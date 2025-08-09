#pragma once
#include <memory>
#include <map>
#include <unordered_map>
#include <mutex>
#include <list>
#include "ArcCacheNode.h"
namespace cache_set{
template<typename Key,typename Value>
class ArcLfuSection
{
public:
    using NodeType = ArcCacheNode<Key,Value>;
    using Node_ptr = std::shared_ptr<NodeType>;
    using Fre_map = std::map<unsigned,std::list<Node_ptr>>;
    using Node_map = std::unordered_map<Key,Node_ptr>;

    ArcLfuSection(size_t capacity,unsigned threshold):
    _capacity(capacity),
    _capacity_ghost(capacity),
    _threshold(threshold),
    //创建时频率初始化为0
    _min_freq(0){
        initialize();
    }

    bool put(const Key key,const Value value){
        if(_capacity == 0) return true;
        std::lock_guard<std::mutex> lock(_mutex);
        //如果能找到
        if(lfu_list.find(key) != lfu_list.end()){
           //更新一下值并返回
            return updateExistingNode(lfu_list[key],value);
        }
        //没找到就创建一个新的节点插入并返回
        return addNewNode(key,value);
    }

    bool get(Key key,Value & value){
        std::lock_guard<std::mutex> lock(_mutex);
        //如果能找到
        if(lfu_list.find(key) != lfu_list.end()){
            //获取值
            value = lfu_list[key] -> get_value();
            //更新访问频率与访问队列
            updateNodeFrequency(lfu_list[key]);
            return true;
        }
        return false;
    }
    //查看此键值是否存在于内存中
    bool contain(Key key){
        return lfu_list.find(key) != lfu_list.end();
    }
    
    bool check_ghost(Key key){
        //获取对应键在幽灵缓存中的迭代器
        auto it = lfu_list_ghost.find(key);
        //如果存在于幽灵缓存中
        if(it != lfu_list_ghost.end()){
            //将它从幽灵缓存中移除
            removeFromghost(it->second);
            //哈希映射也删除
            lfu_list_ghost.erase(key);
            return true;
        }
        return false;
    }

    void increase_capacity(){
        //如果幽灵缓存非空，主缓存容量自增
        /* if(!lfu_list_ghost.empty()){ */
            _capacity++;
        /* } */
        return ;
    }

    bool decrease_capacity(){
        //主缓存容量为0，直接返回
        if(_capacity <= 0) return false;
        //主缓存容量满，则踢出最低频列表中的最久未访问数据
        if(_capacity == lfu_list.size()){
            evict_minFreqNode();
        }
        _capacity--;
        return true;
    }

    ~ArcLfuSection() {}
private:
    void initialize(){
        //初始化幽灵头尾节点
        _dummyhead_ghost = std::make_shared<NodeType>();
        _dummytail_ghost = std::make_shared<NodeType>();

        _dummyhead_ghost -> _next = _dummytail_ghost;
        _dummytail_ghost -> _pre = _dummyhead_ghost;
    }

    bool updateExistingNode(Node_ptr node,const Value & value){
        node -> set_value(value);
        updateNodeFrequency(node);
        return true;
    }

    bool addNewNode(const Key & key,const Value & value){
        //如果当前的主缓存已经满了，那就把最小访问频率链表里的最久远数据踢掉
        if(lfu_list.size() == _capacity){
            evict_minFreqNode();
        }
        //创造新的节点添加到主缓存
        Node_ptr node = std::make_shared<NodeType>(key,value);
        //添加到对应的哈希表中
        lfu_list[key] = node;
        //将新加入的数据加入到频次为1的链表中，若此链表不存在，则创建它
        if(freq_map.find(1) == freq_map.end()){
            freq_map[1] = std::list<Node_ptr>();
        }
        //将数据加入链表尾部
        freq_map[1].push_back(node);
        //将最小频率设为1
        _min_freq = 1;
        return true;
    }

    bool updateNodeFrequency(Node_ptr node){
        //将此节点从原本的频率链表中移除
        removeFromfre_list(node);
        //节点的访问次数自增
        node -> increase_access_cnt();
        //将此节点加入到新的频率链表中
        add2fre_list(node);
        //更新当前的最小频率
        _min_freq = min(_min_freq,node -> get_access_cnt());
        return true;
    }

    bool add2fre_list(Node_ptr node){
        //获得对应节点的访问值
        Key key = node -> get_access_cnt();
        //要是此节点对应的访问频率链表不存在
        if(freq_map.find(key) == freq_map.end()){
            //创建它
            freq_map[key] = std::list<Node_ptr>();
        }
        freq_map[key].push_back(node);
        return true;
    }

    bool removeFromfre_list(Node_ptr node){
        Key key = node -> get_access_cnt();
        //从访问值映射的哈希表中获取对应频率的链表
        auto & old_list = freq_map[key];
        //将节点从此链表删除
        old_list.remove(node);
        //如果删除后导致链表空了，那么链表也回收
        if(old_list.empty()){
            freq_map.erase(key);
        }
        return true;
    }

    bool evict_minFreqNode(){
        //如果频率哈希表空，说明不存在链表，直接返回
        if(freq_map.empty()) return false;
        //获取最小频率链表
        std::list<Node_ptr>  & temp = freq_map[_min_freq];
        //如果获取有问题或者链表空，返回
        if(temp.empty()) return false;
        //否则将对应的链表中的首个数据（最久未被访问的数据），伪淘汰（加入到幽灵缓存中）
        add2ghost(temp.front());
        //从主缓存哈希表中移除
        lfu_list.erase(temp.front() -> get_key());
        //从对应频率链表中移除
        temp.pop_front();
        //如果对应缓存链表由于此次移除导致空并且频率映射哈希表非空的话，更新最小值为当前频率映射的首个键
        if(temp.empty() && !freq_map.empty()){
            _min_freq = freq_map.begin() -> first;
        }
        return true;
    }

    bool add2ghost(Node_ptr node){
        if(lfu_list_ghost.size() == _capacity_ghost){
            evictFromghost();
        }
        node -> _next = _dummyhead_ghost -> _next;
        node -> _pre.lock() = _dummyhead_ghost;
        _dummyhead_ghost -> _next -> _pre.lock() = node;
        _dummyhead_ghost -> _next = node;
        lfu_list_ghost[node -> get_key()] = node;
        return true;
    }

    bool removeFromghost(Node_ptr node){
        //对于weak_ptr的托管对象的有效表示要使用expired()函数
        if(!node -> _pre.expired() && node -> _next){
            auto pre = node -> _pre.lock();
            pre -> _next = node ->_next;
            node ->_next->_pre = node -> _pre;
            node -> _next = nullptr;
        }
        return true;
    }

    bool evictFromghost(){
        if(lfu_list_ghost.empty()) return false;
        Node_ptr node = _dummytail_ghost -> _pre.lock();
        //节点无误且链表非空
        if(!node || node == _dummyhead_ghost) return false;
            removeFromghost(node);
            lfu_list_ghost.erase(node -> get_key());
            return true;
    }
private:
    size_t _capacity;
    size_t _capacity_ghost;

    std::mutex _mutex;
    unsigned _threshold;
    unsigned _min_freq;   

    Node_ptr _dummyhead_ghost;
    Node_ptr _dummytail_ghost;
    
    Node_map lfu_list; //主缓存
    Node_map lfu_list_ghost; //幽灵缓存
    Fre_map  freq_map;  //频率-频率链表映射的哈希表
};
}// namespace = cache_set;

