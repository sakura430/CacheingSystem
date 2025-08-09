#include <vector>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <cmath>
#include <map>


class LFU_Cache{

private:
    struct Node{
        int _key;
        int _value;
        int _access_cnt;
        Node *_pre;
        Node *_next;

        Node(int key,int value,int access_cnt):
        _key(key),
        _value(value),
        _access_cnt(access_cnt),
        _pre(nullptr),
        _next(nullptr){}
    };

    struct fre_Node{
        int _freq;
        Node *_L;
        Node *_R;

        fre_Node(int freq):_freq(freq){
            _L -> _next = _R;
            _R -> _pre  = _L;
        }
    };

    void increase_access_cnt(int key){
        Node_list[key] -> _access_cnt ++;
        return ;
    }

    int get_access_cnt(Node *rhs){
        return rhs->_access_cnt;
    }

    void removeNode(Node *node){
        node -> _next ->_pre = node -> _pre;
        node -> _pre -> _next = node -> _next;
    }

    void insertNode(Node *node){
        node ->_next = Fre_list[node->_access_cnt] ->_R;
        node ->_pre = Fre_list[node->_access_cnt] ->_R ->_pre;
        Fre_list[node->_access_cnt] ->_R ->_pre ->_next = node;
        Fre_list[node->_access_cnt] ->_R ->_pre = node;
    }


    int _capacity;
    int min_frequency;
    std::unordered_map<int,Node*> Node_list;
    std::unordered_map<int,fre_Node*> Fre_list;
public:
    LFU_Cache(int capacity):_capacity(capacity),min_frequency(0){}

    int get(int key){
        if(Node_list.find(key)!=Node_list.end()){
            increase_access_cnt(key);
            remove_from_previous_frelist(Node_list[key]);
            add_into_new_frelist(Node_list[key]);
            if(min_frequency )
            return Node_list[key] -> _value;
        }
        return -1;
    }

    void put(int key,int value){
        if(Node_list.find(key)!=Node_list.end()){
            if(Node_list[key]->_value != value){
                updateExsitingNode(key,value);
                get(key);
                return ;
            }
        }else{
            if(Node_list.size() == _capacity){
                evict_min_fre_and_least();
            }
        }
    }
    void evict_min_fre_and_least(){
        int key = Fre_list[min_frequency] ->_L ->_next ->_key;
        Fre_list[min_frequency]->_L ->_next = Fre_list[min_frequency] ->_L ->_next ->_next;
        Fre_list[min_frequency]->_L->_next ->_pre = Fre_list[min_frequency]->_L;
        if(Fre_list[min_frequency]->_L->_next == Fre_list[min_frequency]->_R){
            Node_list.erase(key);
        }
    }
    void updateExsitingNode(int key,int value){
        Node_list[key] ->_value = value;
    }
    void remove_from_previous_frelist(Node* node){
        removeNode(node);
        if(Fre_list[node->_access_cnt] -> _L ->_next == Fre_list[node->_key]-> _R){
        Fre_list.erase(node -> _access_cnt);
        }
    }
    void add_into_new_frelist(Node *node){
        insertNode(node); 
    }
};
