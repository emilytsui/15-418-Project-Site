#include <vector>

#include "tools/linked_list_node.h"

template <typename K, typename V>
class DelOptHashTable {
private:

    void mark(LLNode<K,V>* node) {
        node->set_next(node->get_next() | 0x1);
    }

    void unmark(LLNode<K,V>* node) {
        node->set_next(node->get_next() & (-1 << 1));
    }

    bool is_marked(LLNode<K,V>* node) {
        return ((*(long*) node->get_next()) & 0x1);
    }

    LLNode<K,V>* internal_find(LLNode<K,V>* head, K key) {
        LLNode<K,V>* prev = head;
        LLNode<K,V>* curr = head->get_next();
        while (true)
        {
            if (curr == NULL)
            {
                return NULL;
            }
            LLNode<K,V>* next = curr->get_next();
            if (prev->get_next() != curr)
            {
                return internal_find(head, key);
            }
            if (!is_marked(curr))
            {
                if (curr->get_key() == key)
                {
                    return curr;
                }
            }
            prev = curr;
            curr = next;
        }
    }

public:
    int table_size;
    int (*hash_fn) (K);
    std::vector< LLNode<K,V>* > table;

    DelOptHashTable(int num_buckets, int (*hash) (K)) {
        table_size = num_buckets;
        hash_fn = hash;
        table = std::vector< LLNode<K,V>* >(num_buckets, new LLNode<K, V>());
    }

    bool insert(K key, V val) {
        int hashIndex = hash_fn(key) % table_size;
        LLNode<K,V>* head = table[hashIndex];
        LLNode<K,V>* prev = head;
        LLNode<K,V>* node = new LLNode<K,V>(key, val);
        while(true) {
            if (internal_find(head, key) != NULL) {
                return false;
            }
            LLNode<K,V>* curr = head->get_next();
            while (curr != NULL)
            {
                if (is_marked(curr))
                {
                    if (__sync_bool_compare_and_swap(&(prev->get_next()->get_key()), curr->get_key(), key) &&
                        __sync_bool_compare_and_swap(&(prev->get_next()->get_data()), curr->get_data(), val))
                    {
                        unmark(curr);
                        return true;
                    }
                }
                prev = curr;
                curr = curr->get_next();
            }
            node->set_next(curr);
            if (__sync_bool_compare_and_swap(&table[hashIndex], curr, node))
            {
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
                return NULL;
            }
            if (!__sync_bool_compare_and_swap(&internal_find(head, key), curr, mark(curr))) {
                return curr;
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
