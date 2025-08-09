#include "CachePolicy.h"
#include <thread>
#include <cmath>

using namespace std;

namespace cache_set{

template<typename Key,typename Value> class LRU_1;

template<typename Key,typename Value>
class LRU_Node{
public:
    LRU_Node(Key key,Value val)
    :_key(key),
     _val(val),
     _access_cnt(1){}

     void set_key(Key key){
        _key = key;
        return ;
     }
     
    Key get_key() const {
        return _key;
    }

    void set_val(Value val){
        _val = val;
        return ;
    }
    Value get_val() const {
        return _val;
    }

    size_t get_access_cnt() const {
        return _access_cnt;
    }

    void increase_access_cnt(){
        _access_cnt++;
        return ;
    }
    friend class LRU_1<Key,Value>;
private:
    //key-value pair
    Key _key;
    Value _val;
    //access counts
    size_t _access_cnt;
    // mark here,question exsits.
    weak_ptr<LRU_Node<Key,Value>> _pre;
    shared_ptr<LRU_Node<Key,Value>> _next;

};


template<typename Key,typename Value>
class LRU_1 : public cache_policy_set<Key,Value>{
public:
    using LruNodeType = LRU_Node<Key,Value>;
    using node_ptr = shared_ptr<LruNodeType>;
    using node_map = unordered_map<Key,node_ptr>;

    LRU_1(size_t capacity)
    :_capacity(capacity){
        initialize_list();
    }

    ~LRU_1() override = default;

    void put(const Key key,const Value val) override {
        //lock on
        lock_guard<mutex> lock(_mutex);
        // find it and update its pos and value
        if(Lru_list.find(key)!=Lru_list.end()){
            updateExistingNode(Lru_list[key],val);
            move2mostRecent(Lru_list[key]);
        }else{//not find
            //if full ,evict the leastest one so that left one unit for new node
            if( Lru_list.size() == _capacity ){
                evict_leastRecent();
            }
            add_new_node(key,val);
        }
        return ;
    }

    Value get(Key key) override {
        //euqals Value value = 0;
        Value value{};
        get(key,value);
        return value;
    }
    bool get(Key key,Value & val) override {
        lock_guard<mutex> lock(_mutex);
        // a flag to express get or not get
        bool isfind = false;
        if(Lru_list.find(key)!=Lru_list.end()){
            move2mostRecent(Lru_list[key]);
            val = Lru_list[key]->get_val();
            isfind = true;
        }
        return isfind;
    }

    void remove(Key key){
        lock_guard<mutex> lock(_mutex);
        if(Lru_list.find(key) != Lru_list.end()){
            //remove from doubled_list
            removeNode(Lru_list[key]);
            //remove from hashmap
            Lru_list.erase(key);
        }
        return ;
    }
private:
    void initialize_list(){
        //create head pointer and tail pointer
        _dummyhead = make_shared<LruNodeType>(Key(),Value());
        _dummytail = make_shared<LruNodeType>(Key(),Value());
        //makes head ptr and tail ptr point to each other(single direction)
        _dummyhead -> _next = _dummytail;
        _dummytail -> _pre = _dummyhead;
    }

    void updateExistingNode(node_ptr node,Value  value){
        node -> set_val(value);
        move2mostRecent(node);
    }
    void add_new_node(const Key & key,const Value & val){
        node_ptr newNode = make_shared<LruNodeType>(key,val);
        /*if(Lru_list.size() == _capacity){
            evict_leastRecent();
        }*/
       // add into doubled_list
        insertNode(newNode);
        //add into hashmap
        Lru_list[key] = newNode;
    }
    void move2mostRecent(node_ptr node){
        removeNode(node);
        insertNode(node);
    }
    void insertNode(node_ptr node){
        
        node -> _next = _dummytail;
        _dummytail -> _pre.lock() -> _next = node;
        node -> _pre = _dummytail -> _pre;
        _dummytail -> _pre = node; 

        Lru_list[node->_key] = node;
    }
    void removeNode(node_ptr node){
        node -> _next -> _pre = node -> _pre;
        node -> _pre.lock() -> _next = node -> _next;
        node -> _next = nullptr;
    }
    void evict_leastRecent(){
        node_ptr  to_be_pop = _dummyhead -> _next;
        removeNode(to_be_pop);
        Lru_list.erase(to_be_pop->_key);
    }

private:
    node_map Lru_list;
    size_t _capacity;
    mutex _mutex;
    node_ptr _dummyhead;
    node_ptr _dummytail;
};


template<typename Key,typename Value>
class LRU_k : public LRU_1<Key,Value>{
public:
    LRU_k(size_t capacity,size_t history_capacity,int k):
    LRU_1<Key,Value>(capacity),
    // initialize a unique_ptr to manage LRU_1 object (allocate sizeof(LRU_1<Key,size_t> * history_capacity))
    _history_list(make_unique<LRU_1<Key,size_t>>(history_capacity)),
    _k(k){}

    Value get(Key key) override {
        //initialize var "value" to 0
        Value value{};
        //to know whether the data in main memory or not
        bool in_main_mem = LRU_1<Key,Value>::get(key,value);

        size_t history_access_cnt = _history_list -> get(key);
        history_access_cnt++;
        _history_list -> put(key,history_access_cnt);

        if(in_main_mem){
            return value;
        }
        //over threshold k
        if(history_access_cnt >= _k){
            //check whether the key exsits in history_list or not
            if(_history_val_map.find(key)!=_history_val_map.end()){
                //check
                Value stored_value = _history_val_map[key];
                _history_list->remove(key);
                _history_val_map.erase(key);
                 LRU_1<Key,Value>::put(key,stored_value);
                 return stored_value;
            }
        }
        //if do not exist ,return default value
        return value;
    }
#if 0
    bool get(Key key,Value & value) override {
        bool in_main_cache = LRU_1<key,Value>::get(key,value);

        if(in_main_cache){
            return true;
        }
        size_t history_access_cnt = _history_list -> get(key);
        history_access_cnt++;
        _history_list -> put(key,history_access_cnt);

        if(history_access_cnt >= k){
            Value stored_value = _history_val_map[key] -> _val;
            _history_list -> remove(key);
            _history_val_map.earse(key);
            LRU_1<Key,Value>::put(key,stored_value);
            return 
        }
    }
#endif
    void put(Key key,Value value){
        Value exsitingValue{};
        bool in_main_cache = LRU_1<Key,Value>::get(key,exsitingValue);

        if(in_main_cache){
            //if exsiting in main memory,we don't care access cnts
            LRU_1<Key,Value>::put(key,value);
            return ;
        }
        size_t history_access_cnt = _history_list -> get(key);
        history_access_cnt++;
        _history_list -> put(key,history_access_cnt);

        _history_val_map[key] = value;
        //if access cnts over threshold ,then remove this key from history list and add it into cache_list
        if(history_access_cnt >= _k){
            _history_list -> remove(key);
            _history_val_map.erase(key);
            LRU_1<Key,Value>::put(key,value);
        }
        return ;
    }


private:
    //k is threshold 
    int _k;
    //
    unique_ptr<LRU_1<Key,size_t>> _history_list;
    //value is history_counts.
    unordered_map<Key,Value> _history_val_map;
};

template <typename Key,typename Value>
class HashLru{
public:
    HashLru(size_t capacity,int slice_nums):
    _capacity(capacity),
    //std::thread::hardwear_concurrency is a function that generate slice nums by analize your nums of cores automatically
    _slice_nums(slice_nums > 0? slice_nums : std::thread::hardware_concurrency()){
        //to caculate each slice size,ceil() will up to integer,use cast to make the expresion execute with double type ,avoiding prececion lose
        size_t slice_size = std::ceil(_capacity / static_cast<double> (_slice_nums ));
        for(int i = 0;i < _slice_nums; i++){
            //insert lru slice into vector
            lru_slice_cache.emplace_back(new LRU_1<Key,Value>(slice_size));
        }
    }
    void put(Key key,Value value){
        size_t slice_index = hash(key) % _slice_nums;
        lru_slice_cache[slice_index] -> put(key,value);
    }

    bool get(Key key,Value value){
        size_t slice_index = hash(key) % _slice_nums;
        lru_slice_cache[slice_index] -> get(key,value);
        return true;
    }

    Value get(Key key){
        Value value{};
        get(key,value);
        return value;
    }
private:
    size_t hash(Key key){
        std::hash<Key> hashfunc;
        return hashfunc(key);
    }
private:
    size_t _capacity;
    int    _slice_nums;
    vector<unique_ptr<LRU_1<Key,Value>>> lru_slice_cache;  
};
}//namespace = cache_set
