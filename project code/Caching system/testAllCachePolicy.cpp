#include <array>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <list>
//用来进行时间操作的头文件
#include <chrono>
#include <vector>
#include <iomanip>
#include <random>
#include <unordered_map>
#include <algorithm>

#include "CachePolicy.h"
#include "LRU_k.h"
#include "LFU_v2.h"
#include "./ArcCache/ArcCache.h"
using namespace std;

class Timer {
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}
    
    double elapsed() {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_).count();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};


void printResults(const std::string& testName, int capacity, 
                 const std::vector<int>& get_operations, 
                 const std::vector<int>& hits) {
    std::cout << "=== " << testName << " 结果汇总 ===" << std::endl;
    std::cout << "缓存大小: " << capacity << std::endl;
    
    // 假设对应的算法名称已在测试函数中定义
    std::vector<std::string> names;
    if (hits.size() == 3) {
        names = {"LRU", "LFU", "ARC"};
    } else if (hits.size() == 4) {
        names = {"LRU", "LFU", "ARC", "LRU-K"};
    } else if (hits.size() == 5) {
        names = {"LRU", "LFU", "ARC", "LRU-K", "LFU-Aging"};
    }
    
    for (size_t i = 0; i < hits.size(); ++i) {
        double hitRate = 100.0 * hits[i] / get_operations[i];
        std::cout << (i < names.size() ? names[i] : "Algorithm " + std::to_string(i+1)) 
                  << " - 命中率: " << std::fixed << std::setprecision(2) 
                  << hitRate << "% ";
        // 添加具体命中次数和总操作次数
        std::cout << "(" << hits[i] << "/" << get_operations[i] << ")" << std::endl;
        // 添加柱状图输出
        std::cout << "[";
        for (int i = 0; i < static_cast<int>(hitRate/2); ++i)
        std::cout << "=";
        std::cout << "] " << hitRate << "%\n";
        }
    
        std::cout << std::endl;  // 添加空行，使输出更清晰
}


void test_hot_data_access(){
    cout<<"\n= = = test case1: hot data access test = = ="<<endl;
    const int CAPACITY = 20;
    const int TOTAL_OPERATIONS = 500000;
    const int HOT_KEYS = 20;
    const int COLD_KEYS = 5000;

    cache_set::LRU_1<int,string> lru(CAPACITY);
    cache_set::LFU_V2<int,string> lfu(CAPACITY);
    cache_set::ArcCache<int,string> arc(CAPACITY);
    cache_set::LRU_k<int,string> Lru_k(CAPACITY,HOT_KEYS+COLD_KEYS,2);
    cache_set::LFU_V2<int,string> lfu_aging(CAPACITY,20000);
    
    random_device rd;
    mt19937 gen(rd());

    array<cache_set::cache_policy_set<int,string>*,5> caches = {&lru,&lfu,&arc,&Lru_k,&lfu_aging};
    vector<int> hits(5,0);
    vector<int> get_operations(5,0);
    std::vector<std::string> names = {"LRU", "LFU", "ARC", "LRU-K", "LFU-Aging"};
    // 为所有的缓存对象进行相同的操作序列测试
    for (int i = 0; i < caches.size(); ++i) {
        // 先预热缓存，插入一些数据
        for (int key = 0; key < HOT_KEYS; ++key) {
            std::string value = "value" + std::to_string(key);
            caches[i]->put(key, value);
        }
        // 交替进行put和get操作，模拟真实场景
        for (int op = 0; op < TOTAL_OPERATIONS; ++op) {
            // 大多数缓存系统中读操作比写操作频繁
            // 所以设置30%概率进行写操作
            bool isPut = (gen() % 100 < 30); 
            int key;
            
            // 70%概率访问热点数据，30%概率访问冷数据
            if (gen() % 100 < 70) {
                key = gen() % HOT_KEYS; // 热点数据
            } else {
                key = HOT_KEYS + (gen() % COLD_KEYS); // 冷数据
            }
            
            if (isPut) {
                // 执行put操作
                std::string value = "value" + std::to_string(key) + "_v" + std::to_string(op % 100);
                caches[i]->put(key, value);
            } else {
                // 执行get操作并记录命中情况
                std::string result;
                get_operations[i]++;
                if (caches[i]->get(key, result)) {
                    hits[i]++;
                }
            }
        }
    }

    // 打印测试结果
    printResults("热点数据访问测试", CAPACITY, get_operations, hits);
}

void test_loopPatern(){
    cout<<"\n= = = test case2:loop screen  test = = ="<<endl;
    const int CAPACITY = 50;
    const int TOTAL_OPERATIONS = 200000;
    const int LOOP_SIZE = 500;

    cache_set::LRU_1<int,string> lru(CAPACITY);
    cache_set::LFU_V2<int,string> lfu(CAPACITY);
    cache_set::ArcCache<int,string> arc(CAPACITY);
    cache_set::LRU_k<int,string> lruk(CAPACITY,LOOP_SIZE*2,2);
    cache_set::LFU_V2<int,string> lfuaging(CAPACITY,3000);
    
    array<cache_set::cache_policy_set<int,string>*,5> caches = {&lru,&lfu,&arc,&lruk,&lfuaging};
    vector<int> hits(5,0);
    vector<int> get_operations(5,0);
    vector<string> names = {"LRU","LFU","ARC","LRU-K","LFU-Aging"};

    random_device rd;
    mt19937 gen(rd());

    for(int i= 0;i<caches.size();i++){
        for(int key =0;key<LOOP_SIZE/5;key++){
            string value = "loop" + to_string(key);
            caches[i] -> put(key,value);
        }
    
    int current_pos = 0;

    for(int op = 0;op<TOTAL_OPERATIONS;op++){
        bool isPut = (gen()%100 < 20);
        int key;

        if(op % 100 < 60){
            key = current_pos;
            current_pos = (current_pos +1)%LOOP_SIZE;
        }else if(op % 100 < 90){
            key = gen() % LOOP_SIZE;
        }else{
            key = LOOP_SIZE + (gen() % LOOP_SIZE);
        }

        if(isPut){
            string value = "loop" + to_string(key) + "_v" + to_string(op % 100);
            caches[i] -> put(key,value);
        }else{
            string result;
            get_operations[i]++;
            if(caches[i]->get(key,result)){
                hits[i]++;
                }
            }
        }
    }
    printResults("循环扫描模式",CAPACITY,get_operations,hits);
}

void testWorkLoadShift(){
    std::cout<<"\n===test case3:work load shift with high frequency test ==="<<std::endl;

    const int CAPACITY = 30;
    const int TOTAL_OPERATIONS = 80000;
    const int PHASE_LASTING = TOTAL_OPERATIONS / 5;

    cache_set::LRU_1<int,string> lru(CAPACITY);
    cache_set::LFU_V2<int,string> lfu(CAPACITY);
    cache_set::ArcCache<int,string> arc(CAPACITY);
    cache_set::LRU_k<int,string> lruk(CAPACITY,500,2);
    cache_set::LFU_V2<int,string> lfuAging(CAPACITY,10000);

    random_device rd;
    mt19937 gen(rd());
    array<cache_set::cache_policy_set<int,string>*,5> caches;
    vector<int> hits(5,0);
    vector<int> get_operations(5,0);
    vector<string> names = {"LRU","LFU","ARC","LFU-K","LFUAging"};

    for(int i = 0;i<caches.size();i++){
        for(int key = 0;key<30;key++){
            string value = "init" + to_string(key);
            caches[i]->put(key,value);
        }

        for(int op = 0;op<TOTAL_OPERATIONS;op++){
            int phase = op/PHASE_LASTING;

            int putProbability;
            switch(phase){
            case 0:putProbability = 15;break;
            case 1:putProbability = 30;break;
            case 2:putProbability = 10;break;
            case 3:putProbability = 25;break;
            case 4:putProbability = 20;break;
            default:putProbability = 20;
            }
        bool isPut = (gen() % 100 < putProbability);
        
        int key;
        if(op < PHASE_LASTING){
            key = gen()%5;
        }else if(op < PHASE_LASTING *2){
            key = gen() % 400;
        }else if(op < PHASE_LASTING *3){
            key = (op - PHASE_LASTING *2) % 100;
        }else if(op < PHASE_LASTING *4){
            int locality = (op/800) % 5;
            key = locality *15 + (gen()% 15);
        }else{
            int r = gen() % 100;
            if(r < 40){
                key = gen() % 5;
            }else if(r<70){
                key = 5 + (gen() % 45);
            }else{
               key = 50 + (gen() % 350); 
            }
        }

        if(isPut){
            string value = "value" + to_string(key) + "_p" + to_string(phase);
            caches[i] -> put(key,value);
        }else{
            string result;
            get_operations[i]++;
            if(caches[i]->get(key,result)){
                hits[i]++;
            }
        }
        }
    }
    printResults("工作负载剧烈变化测试",CAPACITY,get_operations,hits);
}

int main()
{   test_hot_data_access();
    test_loopPatern();
    testWorkLoadShift();
    return 0;
}

