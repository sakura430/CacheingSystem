#include <memory>
#include <unordered_map>
#include <mutex>
#include "CachePolicy.h"
#include <vector>
using namespace std;

namespace cache_set{
template<typename Key,typename Value>
class Fre_List{
private:
    struct Node{
        Key _key;
        Value _value;
        int _fre_times;
        //here _pre has been initialize to nullptr by its constructor because it is a weak_ptr type
        weak_ptr<Node> _pre;
        shared_ptr<Node> _next;

        Node():_fre_times(1),_next(nullptr){}
        
        Node(Key key,Value value):_key(key),_value(value),_fre_times(1),_next(nullptr){}
    };

    using Node_ptr = shared_ptr<Node>;

    int _freq;
    //virtual headptr and virtual tailptr
    Node_ptr _dummyhead;
    Node_ptr _dummytail;
public:
    explicit Fre_List(int freq):_freq(freq){
        _dummyhead = make_shared<Node>;
        _dummytail = make_shared<Node>;
        _dummyhead -> _next = _dummytail;
        _dummytail -> _pre  = _dummyhead;
    }
    bool is_empty()const{
        return this ->_dummyhead -> _next == this -> _dummytail;
    }
    void add_node(Node_ptr node){
        if(!node||!_dummyhe>>> lruSliceCaches_; // 切片LRU缓存ead||!_dummytail){return ;}
        node -> _next = _dummytail;
        node -> _pre  = _dummytail -> _pre;
        _dummytail -> _pre.lock() -> _next =  node;
        _dummytail -> _pre = node;
        return;
    }
    void remove_node(Node_ptr node){
        if(!node||!_dummyhead||!_dummytail){return;}
        node -> _next.lock() -> _pre = node -> _pre;
        node -> _pre.lock() -> _next = node -> _next; 
        return ;
    }
    //the node is least node
    Node_ptr get_first_node()const{
        return this -> _dummyhead -> _next;
    }

};



template<typename Key,typename Value>
class LFU_V2 : public cache_policy_set<Key,Value>{
public:
    //must use typename before dependent nested types in templates to avoid ambiguity and compile correctly.
    using Node = typename Fre_list<Key,Value>::Node;
    using Node_ptr = shared_ptr<Node>;
    using Node_map = unordered_map<Key,Node_ptr>;

    LFU_V2(int capacity,int max_ave = 10)
    :_capacity(capacity),
    _max_ave_fre(max_ave),
    _fre_min(INT8_MAX),
    _cur_total_fre(0),
    _cur_ave_fre(0){}
    /*It defines a default destructor that safely overrides the base class's virtual destructor,
     ensuring correct destruction of derived objects*/
    ~LFU_V2() override = default;

    void put(Key key,Value value) override {
        if(_capacity == 0) return ;
        //lock on
        lock_guard<mutex> lock(_mutex);
        // if find in node_map,just change the value and change its fre_list
        if(_Node_map.find(key)!= _Node_map.end()){
            Node_map[key] -> _value = value;
            get_internal(Node_map[key],value);
            return ;
        }
        //not find , put it inside 
        put_internal(key,value);
        return ;
    }

    bool get(Key key,Value & value) override {
        //empty table,just return
        if(_Node_map.empty()) return false; 

        lock_guard<mutex> lock(_mutex);
        // a nice usage
        auto it = _Node_map.find(key);
        if(it == Node_map.end()){
            get_internal(_Node_map[key],value);
            return true;
        }
        return false;
    }

    Value get(Key key) override {
        Value value{};
        bool isfind = get(key,value);
        return value;
    }
    //clear 2 maps up
    void purge(){
       _Node_map.clear();
       fre2fre_list.clear();
       return ;
    }

private:
    void put_internal(Key key,Value value);
    void get_internal(Node_ptr node,Value & value);
    void kick_out();
    void removeFromFre_list(Node_ptr node);
    void add2Fre_list(Node_ptr node);
    void add_fre_times();
    void dec_fre_times(int cnt);
    void handle_over_max_ave();
    void updateMinFreq();
private:
    int _capacity;
    int _fre_min;
    int _max_ave_fre;
    int _cur_ave_fre;
    int _cur_total_fre;
    mutex _mutex;
    Node_map _Node_map;
    unordered_map<Key,Fre_List<Key,Value>*> fre2fre_list; 

};
#if 0
template<typename Key,typename Value>
int LFU_V2<Key,Value>::_cur_total_fre = 0;

template<typename Key,typename Value>
int LFU_V2<Key,Value>::_cur_ave_fre = 0;
#endif

template<typename Key,typename Value>
void LFU_V2<Key,Value>::put_internal(Key key,Value value){
    //if full ,kick out the oldest one
    if(_Node_map.size() == _capacity){
        kick_out();
    }
    Node_ptr newNode = make_shared<Node>(key,value);
    _Node_map[key] = newNode;
    add2Fre_list(newNode);
    add_fre_times();
    _fre_min = min(1,_fre_min);
    return ;
}

template<typename Key,typename Value>
void LFU_V2<Key,Value>::get_internal(Node_ptr node,Value & value){
    value = node -> _value;
    removeFromFre_list(node);
    node -> _freq++;
    add2Fre_list(node);
    if(node -> _freq -1 == _fre_min && fre2fre_list.find(node->_freq-1)->is_empty()){
        _fre_min++;
    }
    add_fre_times();
    return ;
}

template<typename Key,typename Value>
void LFU_V2<Key,Value>::kick_out(){
    Node_ptr node = fre2fre_list[_fre_min] -> get_first_node();
    removeFromFre_list(node);
    _Node_map[node->_key].erase();
    dec_fre_times(node->_freq_times);
    return ;
}

template<typename Key,typename Value>
void LFU_V2<Key,Value>::removeFromFre_list(Node_ptr node){
     if(!node) return ;
     auto freq = node -> _freq_times;
    fre2fre_list[freq] -> remove_node(node);
    return ;
}

template<typename Key,typename Value>
void LFU_V2<Key,Value>::add2Fre_list(Node_ptr node){
    if(!node) return ;
    auto freq = node -> _freq_times;
    if(fre2fre_list.find(freq) == fre2fre_list.end() ){
        fre2fre_list[freq] = new Fre_List<Key,Value>(freq);
    }
    fre2fre_list[freq] -> add_node(node);
    return ;
}

template<typename Key,typename Value>
void LFU_V2<Key,Value>::add_fre_times(){
    _cur_total_fre++;
    if(_Node_map.empty()){
        _cur_ave_fre = 0;
    }else{
        _cur_ave_fre = _cur_total_fre / _Node_map.size();
    }
    if(_cur_ave_fre > _max_ave_fre){
        handle_over_max_ave();
    }
    return ;
}

template<typename Key,typename Value>
void LFU_V2<Key,Value>::dec_fre_times(int cnt){
    _cur_total_fre -= cnt;
    if(_Node_map.empty()){
        _cur_ave_fre = 0;
    }else{
        _cur_ave_fre = _cur_total_fre / _Node_map.size();
    }
    return ;
}

template<typename Key,typename Value>
void LFU_V2<Key,Value>::handle_over_max_ave(){
    if(_Node_map.empty()) return ;

    //stat all node from fre2fre_list so that change their list from old fre_list
    for(auto it = _Node_map.begin();it!=_Node_map.end();++it){
        if(!it -> second){
            continue;;
        }
        Node_ptr node = it -> second;
        removeFromFre_list(node);
        node -> _fre_times -= _max_ave_fre / 2;
        if(node -> _fre_times < 1) node -> _fre_times = 1; 
        add2Fre_list(node)
    }
    updateMinFreq();
    return ;
}

template<typename Key,typename Value>
void LFU_V2<Key,Value>::updateMinFreq(){
    _fre_min = INT8_MAX;
    for(const auto & pair : fre2fre_list){
        if(pair.second && !pair.second -> is_empty()){
            _fre_min = min(_fre_min,pair.first);
        }
    }
    if(_fre_min == INT8_MAX) _fre_min = 1;
    return ;
}


template<typename Key,typename Value>
class Hash_LFU_V2{
public:
    Hash_LFU_V2(size_t capacity,int slice_nums):
    _capacity(capacity),
    _slice_nums(slice_nums > 0 ? slice_nums : thread::hardware_concurrency){
        //ceil up to get integer,make _slice_nums cast into double so that ensure precision correct
        size_t slice_size = ceil(capacity / static_cast<double>_slice_nums);
        for(int i=0;i<_slice_nums;i++){
            lfu_slice_cache.emplace_back(new LFU_V2<Key,Value>(slice_size));
        }

    }
    void put(Key key,Value value){
        //use hash to get a hash_value then % slice num so that get a index betwwen 0 and _slice_nums -1
        size_t slice_index = hash(key) % _slice_nums;
        lfu_slice_cache[slice_index] -> put(key,value);
    }
    bool get(Key key,Value & Value){
        size_t slice_index = hash(key) % _slice_nums;
        lfu_slice_cache[slice_index] -> get(key,value);
    }
    Value get(Key key){
        Value value{};
        get(key,value);
        return value;
    }

    void purge(){
        for(auto & lfu_slice_cache_unit : lfu_slice_cache){
            lfu_slice_cache_unit -> purge();
        }
    }

private:
    size_t hash(Key key){
        std::hash<Key> hashfunc;
        return hashfunc(key);
    }
private:
    size_t _capacity;
    int    _slice_nums;
    vector<unique_ptr<LFU_V2<Key,Value>>> lfu_slice_cache;

};
}//namespace = cache_set
