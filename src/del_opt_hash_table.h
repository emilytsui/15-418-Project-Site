#include <vector>

#include "tools/linked_list_node.h"

template <typename K, typename V>
class DelOptHashTable {
private:

    LLNode<K,V>* marked(LLNode<K,V>* node) {
        return (LLNode<K,V>*)((unsigned long) node | 0x1);
    }

    LLNode<K,V>* unmarked(LLNode<K,V>* node) {
        return (LLNode<K,V>*)((unsigned long) node & (-1 << 1));
    }

    bool is_marked(LLNode<K,V>* node) {
        return (((unsigned long) node) & 0x1);
    }

    // LLNode<K, V>* noMark(LLNode<K,V>* node)
    // {
    //     return (LLNode<K, V>*)((unsigned long)node & (-1 << 1));
    // }

    // LLNode<K, V>* withMark(LLNode<K,V>* node)
    // {
    //     return (LLNode<K, V>*)((unsigned long)node | 0x1);
    // }

    std::pair<LLNode<K,V>*, LLNode<K,V>*> internal_find(LLNode<K,V>* head, const K key) {
        // printf("key: %d\n", key);
    try_again:
        LLNode<K,V>* prev = head;
        LLNode<K,V>* curr = prev->get_next();
        std::pair<LLNode<K,V>*, LLNode<K,V>*> res;
        while (true)
        {
            if (unmarked(curr) == NULL)
            {
                res.first = prev;
                res.second = unmarked(curr);
                return res;
            }
            LLNode<K,V>* next = (unmarked(curr))->get_next();
            if ((unmarked(prev))->get_next() != curr)
            {
                goto try_again;
            }
            if (!is_marked(curr->next))
            {
                K currKey = unmarked(curr)->get_key();
                // printf("currKey: %d, key: %d, geq: %d\n", currKey, key, currKey >= key);
                if (currKey >= key)
                {
                    res.first = prev;
                    res.second = curr;
                    return res;
                }
                prev = curr;
            }
            else
            {
                if (__sync_bool_compare_and_swap(&(prev->next), curr, unmarked(next))) {
                    // garbage collection - delete(curr)
                }
                else
                {
                    goto try_again;
                }
            }
            curr = next;
        }
    }

public:
    int table_size;
    int (*hash_fn) (K);
    std::vector< LLNode<K,V>* > table;

    DelOptHashTable(const int num_buckets, int (*hash) (K)) {
        table_size = num_buckets;
        hash_fn = hash;
        table = std::vector< LLNode<K,V>* >(num_buckets, NULL);
        // table.resize(num_buckets); // dummy values
        for (int i = 0; i < num_buckets; i++)
        {
            table[i] = new LLNode<K, V>(0, 0);
        }
    }

    bool insert(const K key, const V val) {
        // printf("In Insert!\n");
        int hashIndex = hash_fn(key) % table_size;
        LLNode<K,V>* head = table[hashIndex];
        LLNode<K,V>* node = new LLNode<K,V>(key, val);
        while(true) {
            std::pair<LLNode<K,V>*, LLNode<K,V>*> res = internal_find(head, key);
            LLNode<K,V>* curr = res.second;
            LLNode<K,V>* prev = res.first;
            if (curr != NULL && (curr->get_key() == key)) {
                return false;
            }
            // printf("Finished internal find!\n");
            node->set_next(curr);
            // printf("Prev next: %p\n", prev->next);
            // printf("Curr: %p\n", curr);
            // printf("New node: %p\n", node);
            if (__sync_bool_compare_and_swap(&(prev->next), curr, node))
            {
                return true;
            }
            // printf("End of loop curr is %p!\n", curr);
        }
    }

    bool remove(const K key) {
        // printf("In remove!\n");
        int hashIndex = hash_fn(key) % table_size;
        LLNode<K,V>* head = table[hashIndex];
        LLNode<K,V>* prev;
        LLNode<K,V>* curr;
        while(true) {
            std::pair<LLNode<K,V>*, LLNode<K,V>*> res = internal_find(head, key);
            prev = res.first;
            curr = res.second;
            if (unmarked(curr) == NULL)
            {
                // printf("Didn't find to remove\n");
                return false;
            }
            LLNode<K,V>* next = unmarked(curr)->get_next();
            if (!is_marked(next) &&
                !__sync_bool_compare_and_swap(&(unmarked(curr)->next), next, marked(next)))
            {
                continue;
            }
            if (__sync_bool_compare_and_swap(&(unmarked(prev)->next), curr, next)) {
                // garbage collection - delete(curr)
            }
            else
            {
                internal_find(head, key);
            }
            return true;
            // printf("End of remove loop\n");
        }
    }

    LLNode<K,V>* find(const K key) {
        // printf("In lookup!\n");
        int hashIndex = hash_fn(key) % table_size;
        LLNode<K,V>* curr = unmarked(internal_find(table[hashIndex], key).second);
        if (curr != NULL && curr->get_key() != key)
        {
            curr = NULL;
        }
        return curr;
    }
};
