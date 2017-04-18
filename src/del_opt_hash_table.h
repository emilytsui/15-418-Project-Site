#include <vector>

#include "tools/linked_list_node.h"

template <typename K, typename V>
class DelOptHashTable {
private:

    LLNode<K,V>* internal_find(LLNode<K,V>* head, K key) {
try_again:
        LLNode<K,V>* prev = head;

    }

    void mark(LLNode<K,V>* node) {

    }

    void unmark(LLNode<K,V>* node) {

    }

    bool is_marked(LLNode<K,V>* node) {

    }

public:
    int table_size;
    int (*hash_fn) (K);
    std::vector< LLNode<K,V>* > table;

    DelOptHashTable(int num_buckets, int (*hash) (K)) {
        table_size = num_buckets;
        hash_fn = hash;
        table = std::vector< LLNode<K,V>* >(num_buckets, NULL);
    }

    void insert(K key, V val) {
        int hashIndex = hash_fn(key) % table_size;
        LLNode<K,V>* head = table[hashIndex];
        LLNode<K,V>* node = new LLNode<K,V>(key, val);
        while(true) {
            if (internal_find(head, key) != NULL) {
                return false;
            }
            LLNode<K,V>* curr = head;
            node->set_next(table[hashIndex]);
            if (__sync_bool_compare_and_swap(&table[hashIndex], curr, node)) {
                return true;
            }
        }
    }

    LLNode<K,V>* remove(K key) {
        int hashIndex = hash_fn(key) % table_size;
        LLNode<K,V>* head = table[hashIndex];
        while(true) {
            LLNode<K,V>* curr = internal_find(head, key);
            if (curr == NULL) {
                return false;
            }
            if (!__sync_bool_compare_and_swap(&internal_find(head, key), curr, mark(curr))) {
                continue;
            }


        }
        // int hashIndex = hash_fn(key) % table_size;
        // LLNode<K,V>* result = NULL;
        // LLNode<K,V>* curr = table[hashIndex];
        // LLNode<K,V>* prev = NULL;
        // while(curr != NULL)
        // {
        //     if (curr->get_key() == key) {
        //         result = curr;
        //         if (prev != NULL)
        //         {
        //             prev->set_next(result->get_next());
        //         }
        //         else
        //         {
        //             table[hashIndex] = result->get_next();
        //         }
        //         return result;
        //     }
        //     else {
        //         prev = curr;
        //         curr = curr->get_next();
        //     }
        // }
        // return result;
    }

    LLNode<K,V>* find(K key) {
        int hashIndex = hash_fn(key) % table_size;
        return internal_find(table[hashIndex], key);
        // LLNode<K,V>* curr = table[hashIndex];
        // LLNode<K,V>* prev = NULL;
        // while(curr != NULL)
        // {
        //     if (curr->get_key() == key) {
        //         return curr;
        //     }
        //     else {
        //         prev = curr;
        //         curr = curr->get_next();
        //     }
        // }
        // return NULL;
    }
};
