#include <vector>

#include "linked_list_node.h"

template <typename K, typename V>
class SeqHashTable {
private:

public:
    int table_size;
    int (*hash_fn) (K);
    std::vector< LLNode<K,V>* > table;

    SeqHashTable(int num_buckets, int (*hash) (K)) {
        table_size = num_buckets;
        hash_fn = hash;
        table = std::vector< LLNode<K,V>* >(num_buckets, NULL);
    }

    void insert(K key, V val) {
        LLNode<K,V>* node = new LLNode<K,V>(key, val);
        int hashIndex = hash_fn(key) % table_size;
        node->set_next(table[hashIndex]);
        table[hashIndex] = node;
    }

    LLNode<K,V>* remove(K key) {
        int hashIndex = hash_fn(key) % table_size;
        LLNode<K,V>* result = NULL;
        LLNode<K,V>* curr = table[hashIndex];
        LLNode<K,V>* prev = NULL;
        while(curr != NULL)
        {
            if (curr->get_key() == key) {
                result = curr;
                if (prev != NULL)
                {
                    prev->set_next(result->get_next());
                }
                else
                {
                    table[hashIndex] = result->get_next();
                }
                return result;
            }
            else {
                prev = curr;
                curr = curr->get_next();
            }
        }
        return result;
    }

    LLNode<K,V>* find(K key) {
        int hashIndex = hash_fn(key) % table_size;
        LLNode<K,V>* curr = table[hashIndex];
        LLNode<K,V>* prev = NULL;
        while(curr != NULL)
        {
            if (curr->get_key() == key) {
                return curr;
            }
            else {
                prev = curr;
                curr = curr->get_next();
            }
        }
        return NULL;
    }
};
