#pragma once
#include <cstddef>
#include <memory>

namespace cache_set{
template <typename Key,typename Value>
class ArcCacheNode{
private:
    Key _key;
    Value _value;
    unsigned _access_cnt;
    std::weak_ptr<ArcCacheNode>   _pre;
    std::shared_ptr<ArcCacheNode> _next; 
public:
    ArcCacheNode(Key key,Value value):
    _key(key),
    _value(value),
    _access_cnt(1),
    _next(nullptr){}
    
    ArcCacheNode():_access_cnt(1),_next(nullptr){}

    template<typename K,typename V> friend class ArcLruSection;
    template<typename K,typename V> friend class ArcLfuSection;
    
    Key get_key() const { return _key; }
    Value get_value() const { return _value;}
    bool set_key(Key key){ 
        _key = key;
        return true;
    }
    bool set_value(Value value){
        _value = value;
        return true;
    }
    bool set_access_cnt(unsigned access_cnt){
        _access_cnt = access_cnt;
        return true;
    }
    void increase_access_cnt(){
        _access_cnt++;
    }   
    unsigned get_access_cnt(){
        return _access_cnt;
    }
};

} // namespace = cache_set

