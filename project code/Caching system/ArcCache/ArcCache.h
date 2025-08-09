//这个条件就是让重定义视为唯一的
#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include "../CachePolicy.h"
#include "ArcLfuSection.h"
#include "ArcLruSection.h"

namespace cache_set{
template<typename Key,typename Value>
class ArcCache : public cache_policy_set<Key,Value>
{
public:
    explicit ArcCache(size_t capacity_ = 10,size_t threshold = 2):
    _capacity(capacity_),
    _threshold(threshold){
        lru_part = std::make_unique<ArcLruSection<Key,Value>>(_capacity,_threshold);
        lfu_part = std::make_unique<ArcLfuSection<Key,Value>>(_capacity,_threshold);
    }
    ~ArcCache() override = default;
    
    void put(const Key key,const Value value) override{
        //例行检查是否存储空间需要变化
        check_ghost_cache(key);   
        //查看lfu缓存中是否存储了这个键值对
        bool inlfu = lfu_part -> contain(key);
        //无脑存lru
        lru_part -> put(key,value);
        //要是在lfu中确实有，那么就再放一次（能有说明它的访问次数达到了阈值）
        if(inlfu){
            lfu_part -> put(key,value);
        }
    }

    bool get(Key key,Value & value) override {
            //例行检查是否需要调整lfu与lru的容量
            check_ghost_cache(key);
            //转存标志
            bool transform_to_lfu = false;
            //先例行找lru，并且同时获取到阈值标志
            if(lru_part->get(key,value,transform_to_lfu)){
                //访问次数达标，允许放置到lfu中
                if(transform_to_lfu){
                    lfu_part -> put(key,value);
                }
                return true;
            }
            //之前要是找到了就返回了，没找到上lfu碰碰运气
            return lfu_part -> get(key,value);
    }

    Value get(Key key) override {
        Value value{};
        get(key,value);
        return value;
    }
private:
    bool check_ghost_cache(Key key){
    
        bool in_ghost = false;
        //如果lru的幽灵中有，那就缩小lfu容积，增大lru容积
        if(lru_part -> check_ghost(key)){
            if(lfu_part -> decrease_capacity()){
                lru_part -> increase_capacity();
            }
            in_ghost = true;
        }
        //与上面相反过来
        else if(lfu_part -> check_ghost(key)){
            if(lru_part -> decrease_capacity()){
                lfu_part -> increase_capacity();
            }
            in_ghost = true;
        }
        return in_ghost;
    }
private:
    size_t _capacity;
    size_t _threshold;
    
    std::unique_ptr<ArcLruSection<Key,Value>> lru_part;
    std::unique_ptr<ArcLfuSection<Key,Value>> lfu_part;
};
} // namespace = cache_set;

