#ifndef _CACHEPOLICY_H_
#define _CACHEPOLICY_H_
#include <list>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <thread>
namespace cache_set{  
    template<typename Key,typename Value>    
    class cache_policy_set{
      public:  
        virtual ~cache_policy_set(){}

        virtual void put(const Key key,const Value val) = 0;
        
        virtual Value get(Key key) = 0;

        virtual bool get(Key key,Value & val) = 0;

    };
};
#endif
