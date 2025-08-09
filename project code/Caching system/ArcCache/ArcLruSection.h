#pragma once
#include "ArcCacheNode.h"
#include <cstddef>
#include <memory>
#include <mutex>
#include <unordered_map>


namespace cache_set{
template<typename Key,typename Value>
class ArcLruSection
{
public:
    using NodeType = ArcCacheNode<Key,Value>;
    using Node_ptr = std::shared_ptr<NodeType>;
    using Node_map = std::unordered_map<Key,Node_ptr>;
    explicit ArcLruSection(size_t capacity,size_t threshold):
    _capacity(capacity),
    _capacity_ghost(capacity),
    _threshold(threshold){
        initialize();
    }

    ~ArcLruSection() {}

    void put(const Key & key,const Value & value){
        if(_capacity == 0) return ;
        std::lock_guard<std::mutex> lock(_mutex);
        //从哈希映射中寻找这个键值，检查它是否存在
        if(lru_list.find(key)!=lru_list.end()){
            //存在的话，更新值
            updateExsitingNode(lru_list[key],value);
            move2front(lru_list[key]);
            //增加访问次数并获取访问次数
            unsigned access_cnt = increase_accessCnt(lru_list[key]);
            return ;
        }   
        //没找到的话
         else{
                //添加新数据到lru缓存中
                add_newNode2substance(key,value);
            }
    }

    bool get(Key key,Value & value,bool & should_trans){
        if(_capacity == 0) return false;
        std::lock_guard<std::mutex> lock(_mutex); 
        bool isget = false;
        should_trans = false;
        if(lru_list.find(key)!=lru_list.end()){
            isget = true;
            auto it = lru_list.find(key);
            value = it -> second -> get_value();
            move2front(lru_list[key]);
            unsigned access_cnt = increase_accessCnt(it->second);
            if(access_cnt >= _threshold){
                should_trans = true;
            }
        }
        return isget;
    }
    
    Value get(Key key){
        Value value{};
        get(key,value);
        return value;
    }

    bool check_ghost(Key key){
        //获取在幽灵缓存中key的对应的迭代器
        auto it = lru_list_ghost.find(key);
        //如果存在
        if(it!= lru_list_ghost.end()){
            //将此数据从幽灵缓存中移除
            removeNode(it -> second);
            //将此数据从幽灵哈希映射中移除
            lru_list_ghost.erase(it);
            return true;
        }
        return false;
    }

    bool increase_capacity(){
        _capacity++; 
        return true;
    }

    bool decrease_capacity(){
        if(_capacity <= 0){
            return false;
        }
        if(_capacity == lru_list.size()){
            evict_least_from_substance();
        }
        _capacity--;
        return true;
    }
private:
    void initialize(){
        //初始化lru的虚拟头节点与虚拟尾节点
        _dummyhead = std::make_shared<NodeType>();
        _dummytail = std::make_shared<NodeType>();
        //设置lru为空
        _dummyhead -> _next = _dummytail;
        _dummytail -> _pre = _dummyhead;
        //初始化lru幽灵链表的头尾节点
        _dummyhead_ghost = std::make_shared<NodeType>(Key(),Value());
        _dummytail_ghost = std::make_shared<NodeType>(Key(),Value());
        //初始化幽灵链表为空
        _dummyhead_ghost -> _next = _dummytail_ghost;
        _dummytail_ghost -> _pre  = _dummyhead_ghost;
    }

    bool  updateExsitingNode(Node_ptr node,const Value &value){
        node -> set_value(value);
        move2front(node);
        return true;
    }

    void move2front(Node_ptr node){
        removeNode(node);
        add_front2Substance(node);
    }

    bool add_newNode2substance(const Key &key,const Value & value){
        // 如果当前缓存的数量大于了容量，那么就踢
        if(lru_list.size() >= _capacity){
            //踢出最久未被访问的数据
            evict_least_from_substance();
        }
        //创建新的数据
        Node_ptr newNode = std::make_shared<NodeType>(key,value);
        //加到缓存队头
        add_front2Substance(newNode);
        //加入哈希表映射中
        lru_list[key] = newNode;
        return true;
    }

    bool add_newNode2ghost(Key key){
        //如果幽灵队列满了，踢出最久未被访问的元素
        if(lru_list_ghost.size() >= _capacity_ghost){
            evict_least_from_ghost();
        }
        //将数据的访问次数设置为1，准备加入幽灵缓存
        lru_list[key] -> set_access_cnt(1);
        //将从主缓存中淘汰下来的数据存到幽灵缓存中
        add_front2Ghost(lru_list[key]);
        //更新幽灵缓存的哈希表映射
        lru_list_ghost[key] = lru_list[key];
        return true;
    }

    void evict_least_from_substance(){
        //获得键，方便后续将它从哈希映射中删除
        Node_ptr node = _dummytail -> _pre.lock();
        if(!node || node == _dummyhead) return ;
        //伪淘汰，加入到幽灵缓存中
        add_front2Ghost(node);
        //踢出最久未被访问的数据
        remove_backFromSubstance();
        //清除哈希映射
        lru_list.erase(node->get_key());
    }

    void evict_least_from_ghost(){
        //获得头指针后继节点所指向的key值
        Node_ptr node = _dummytail_ghost -> _pre.lock();
        if(!node || node == _dummyhead_ghost) return ;
        //从幽灵缓存中清除它
        remove_backFromGhost();
        //从幽灵哈希映射中清除
        lru_list_ghost[node -> get_key()].erase();
    }

    void add_front2Substance(Node_ptr node){
        node -> _next = _dummyhead -> _next;
        node -> _pre.lock() = _dummyhead;
        _dummyhead -> _next -> _pre.lock() = node;
        _dummyhead -> _next = node;
    }

    void remove_backFromSubstance(){
        Node_ptr node = _dummytail -> _pre.lock();
        node -> _pre.lock() -> _next = _dummytail;
        _dummytail -> _pre.lock() = node -> _pre.lock();
    }

    void add_front2Ghost(Node_ptr node){
        node -> _next = _dummyhead_ghost -> _next;
        node -> _pre.lock() = _dummyhead_ghost;
        _dummyhead_ghost -> _next -> _pre.lock() = node;
        _dummyhead_ghost -> _next = node;
    }

    void remove_backFromGhost(){
        Node_ptr node = _dummytail_ghost -> _pre.lock();
        node -> _pre.lock() -> _next = _dummytail_ghost;
        _dummytail_ghost -> _pre.lock() = node -> _pre.lock();
        node -> _next = nullptr;
    }

    void removeNode(Node_ptr node){
        if(!node -> _pre.expired() && node -> _next){
        node -> _next -> _pre = node -> _pre;
        node -> _pre.lock() -> _next = node -> _next;
        node -> _next = nullptr;
        }
    }

    int increase_accessCnt(Node_ptr node){
        node -> increase_access_cnt();
        return node -> _access_cnt;
    }
    bool is_empty_substance(){
        if(_dummyhead -> _next == _dummytail && _dummytail -> _pre == _dummyhead){
            return true;
        }
        return false;
    }
    bool is_empty_ghost(){
        if(_dummyhead_ghost -> _next == _dummytail_ghost && _dummytail_ghost -> _pre == _dummyhead_ghost){
            return true;
        }
        return false;
    }
    
private:
    //lru与其幽灵列表的容量
    size_t _capacity;
    size_t _capacity_ghost;

    //lru向lfu转换的阈值
    unsigned _threshold;
    std::mutex    _mutex;
    //lru的虚拟头节点与尾节点
    Node_ptr _dummyhead;
    Node_ptr _dummytail;
    // 幽灵链表的虚拟头尾节点
    Node_ptr _dummyhead_ghost;
    Node_ptr _dummytail_ghost;

    Node_map lru_list;
    Node_map lru_list_ghost;
};

}// namespace = cache_set
