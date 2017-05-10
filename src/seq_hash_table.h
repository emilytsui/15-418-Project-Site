#include <vector>

#include "tools/linked_list_node.h"

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

    bool insert(K key, V val) {
        LLNode<K,V>* node = new LLNode<K,V>(key, val);
        int hashIndex = hash_fn(key) % table_size;
        LLNode<K,V>* prev = NULL;
        LLNode<K,V>* curr = table[hashIndex];
        while (curr != NULL && curr->get_key() <= key)
        {
            prev = curr;
            curr = curr->get_next();
        }
        if (prev != NULL && prev->get_key() == key)
        {
            // printf("sequential contained key: %d, inserting key: %d\n", prev->get_key(), key);
            return false;
        }
        node->set_next(curr);
        if (prev != NULL)
        {
            prev->set_next(node);
        }
        else
        {
            table[hashIndex] = node;
        }
        return true;
    }

    bool remove(K key) {
        int hashIndex = hash_fn(key) % table_size;
        LLNode<K,V>* result = NULL;
        LLNode<K,V>* curr = table[hashIndex];
        LLNode<K,V>* prev = NULL;
        while(curr != NULL && curr->get_key() <= key)
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
                delete(result);
                return true;
            }
            else {
                prev = curr;
                curr = curr->get_next();
            }
        }
        return false;
    }

    LLNode<K,V>* find(K key) {
        int hashIndex = hash_fn(key) % table_size;
        LLNode<K,V>* curr = table[hashIndex];
        LLNode<K,V>* prev = NULL;
        while(curr != NULL && curr->get_key() <= key)
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
