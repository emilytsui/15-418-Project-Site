#include <vector>
#include <pthread.h>

#include "tools/linked_list_node.h"

template <typename K, typename V>
class FgHashTable {
private:
    std::vector<pthread_mutex_t> locks;

public:
    int table_size;
    int (*hash_fn) (K);
    std::vector< LLNode<K,V>* > table;

    FgHashTable(int num_buckets, int (*hash) (K)) {
        table_size = num_buckets;
        hash_fn = hash;
        table = std::vector< LLNode<K,V>* >(num_buckets, NULL);
        locks.resize(num_buckets);
        for (int i = 0; i < num_buckets; i++)
        {
            pthread_mutex_init(&locks[i], NULL);
        }
    }

    void insert(K key, V val) {
        LLNode<K,V>* node = new LLNode<K,V>(key, val);
        int hashIndex = hash_fn(key) % table_size;
        pthread_mutex_lock(&locks[hashIndex]);
        node->set_next(table[hashIndex]);
        table[hashIndex] = node;
        pthread_mutex_unlock(&locks[hashIndex]);
    }

    LLNode<K,V>* remove(K key) {
        int hashIndex = hash_fn(key) % table_size;
        LLNode<K,V>* result = NULL;
        LLNode<K,V>* prev = NULL;
        pthread_mutex_lock(&locks[hashIndex]);
        LLNode<K,V>* curr = table[hashIndex];
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
                pthread_mutex_unlock(&locks[hashIndex]);
                return result;
            }
            else {
                prev = curr;
                curr = curr->get_next();
            }
        }
        pthread_mutex_unlock(&locks[hashIndex]);
        return result;
    }

    LLNode<K,V>* find(K key) {
        int hashIndex = hash_fn(key) % table_size;
        pthread_mutex_lock(&locks[hashIndex]);
        LLNode<K,V>* curr = table[hashIndex];
        LLNode<K,V>* prev = NULL;
        while(curr != NULL)
        {
            if (curr->get_key() == key) {
                pthread_mutex_unlock(&locks[hashIndex]);
                return curr;
            }
            else {
                prev = curr;
                curr = curr->get_next();
            }
        }
        pthread_mutex_unlock(&locks[hashIndex]);
        return NULL;
    }
};
