#include <iostream>
#include <unordered_map>
#include <time.h>


using namespace std;

class LRU_Cache{

public:
   void LRU_Cache_init(int capacity){
    _capacity_num = capacity;
    _L = new LRU_Node(-1,-1);
    _R = new LRU_Node(-1,-1);

    _L -> _next = _R;
    _R -> _pre = _L;
}
    int get(int key){
        // if find hash[key] is true,remove the element which on old pos and update its pos to the most right pos
        if(hash.find(key)!=hash.end()){
            LRU_Node *temp = hash[key];
            remove(temp);
            insert(temp->_key,temp->_val);
            return temp->_val;            
        }
        return -1;
    }

    int put(int key,int val){
        //if find it,update its pos to the most right pos(the newest pos)and update its val
        if(hash.find(key)!=hash.end()){
            LRU_Node *temp = hash[key];
            remove(temp);
            insert(key,val);
        }
        //if find failed,check left capacity,if under threshold,just insert on the most right pos
        //else pop the most left one ,then insert 
        else{
            if(hash.size() == _capacity_num){
                LRU_Node *temp = _L->_next;
                remove(temp);
                insert(key,val);
            }else{
                insert(key,val);
            }
        }
    }

private:
    typedef struct Node{
    int _key;
    int _val;
    Node *_pre;
    Node *_next;

    Node(int key,int val){
        _key = key;
        _val = val;
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

    void insert(int key,int val){
        //update pointer _R's pre and its next,then insert newNode to hash
        LRU_Node *to_be_insert = new LRU_Node(key,val);
        _R->_pre->_next = to_be_insert;
        to_be_insert->_next = _R;
        _R->_pre = to_be_insert;
        to_be_insert->_pre = _R->_pre;
        hash[key] = to_be_insert;
    }
};


int main(){





}
