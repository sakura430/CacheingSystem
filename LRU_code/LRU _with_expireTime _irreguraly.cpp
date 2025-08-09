#include <iostream>
#include <unordered_map>
#include <time.h>


using namespace std;


//const int ttl = 5;



class LRU_Cache{

public:
   void LRU_Cache_init(int capacity){
    _capacity_num = capacity;
    _L = new LRU_Node(-1,-1,0);
    _R = new LRU_Node(-1,-1,0);
    _L -> _next = _R;
    _R -> _pre = _L;
}
    int get(int key){
        // if find hash[key] is true,remove the element which on old pos and update its pos to the most right pos
        if(hash.find(key)!=hash.end()){
            LRU_Node *temp = hash[key];
            time_t now_time;
            now_time = time(nullptr);
            //if the differ of times over ttl,imeditely pop the timeout one
            if(difftime(temp->_expire_time,now_time)<=0){
            remove(temp);
            return -1;
            }
            //else reset the timer and adjust its pos
            else{
            remove(temp);
            time_t left_time = difftime(temp->_expire_time,now_time);
            insert(temp->_key,temp->_val,left_time);
            return temp->_val;            
        }
    }
        return -1;
    }

    int put(int key,int val,int ttl){
        //if find it,update its pos to the most right pos(the newest pos)and update its val
        if(hash.find(key)!=hash.end()){
            LRU_Node *temp = hash[key];
            remove(temp);
            insert(key,val,ttl);
        }
        //if find failed,check left capacity,if under threshold,just insert on the most right pos
        //else pop the most left one ,then insert 
        else{
            if(hash.size() == _capacity_num){
                time_t now_time;
                now_time = time(nullptr);
                bool is_experi = false;
                auto it = hash.begin();
                for(it;it!=hash.end();it++){
                    if(difftime(it->second->_expire_time,now_time)<=0){
                        is_experi = true;
                        break;
                    }
                }
                if(is_experi){
                    remove(it->second);
                }else{
                    LRU_Node *to_be_delete = _L->_next;
                    remove(to_be_delete);
                }
                insert(key,val,ttl);
            }else{
                insert(key,val,ttl);
            }
        }
    }

private:
    typedef struct Node{
    int _key;
    int _val;
    time_t _expire_time;
    Node *_pre;
    Node *_next;

    Node(int key,int val,time_t expire_time){
        _key = key;
        _val = val;
        _expire_time = expire_time;
        _pre = nullptr;
        _next = nullptr;
    }

}LRU_Node;
   //cache's capacity
    int _capacity_num;
    unordered_map<int,LRU_Node*> hash;

    //double linkedlist's head and tail pointer
    LRU_Node *_L;
    LRU_Node *_R;

    
    void remove(LRU_Node *node){
        //update the node's pre and next,then erase it from hash
        node->_pre->_next = node->_next;
        node->_next->_pre = node->_pre;
        hash.erase(node->_key);
    }

    void insert(int key,int val,time_t ttl){
        //get current time
        time_t now_time;
        now_time = time(nullptr);
        //create a newNode and set its initial args
        LRU_Node *to_be_insert = new LRU_Node(key,val,now_time+ttl);

         //update pointer _R's pre and its next,then insert newNode to hash
        _R->_pre->_next = to_be_insert;
        to_be_insert->_next = _R;
        _R->_pre = to_be_insert;
        to_be_insert->_pre = _R->_pre;
        hash[key] = to_be_insert;
    }
};


int main(){





}
