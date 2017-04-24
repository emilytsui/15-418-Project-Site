#include <vector>

#include "tools/linked_list_node.h"

template <typename K, typename V>
class DelOptHashTable {
private:

    LLNode<K,V>* mark(LLNode<K,V>* node) {
        return node->set_next((LLNode<K,V>*)((unsigned long) node->get_next() | 0x1));
    }

    LLNode<K,V>* unmark(LLNode<K,V>* node) {
        return node->set_next((LLNode<K,V>*)((unsigned long) node->get_next() & (-1 << 1)));
    }

    bool is_marked(LLNode<K,V>* node) {
        return (((unsigned long)node->get_next()) & 0x1);
    }

    LLNode<K, V>* noMark(LLNode<K,V>* node)
    {
        return (LLNode<K, V>*)((unsigned long)node & (-1 << 1));
    }

    LLNode<K, V>* withMark(LLNode<K,V>* node)
    {
        return (LLNode<K, V>*)((unsigned long)node | 0x1);
    }

    std::pair<LLNode<K,V>*, LLNode<K,V>*> internal_find(LLNode<K,V>* head, K key) {
        LLNode<K,V>* prev = noMark(head);
        // printf("Check 1\n");
        LLNode<K,V>* curr = noMark(prev->get_next());
        std::pair<LLNode<K,V>*, LLNode<K,V>*> res;
        while (true)
        {
            // printf("Prev: %p\n", prev);
            // printf("Curr: %p\n", curr);
            if (curr == NULL)
            {
                res.first = prev;
                res.second = curr;
                return res;
            }
            // printf("Check 2\n");
            // printf("Curr: %p\n", curr);
            LLNode<K,V>* next = noMark(curr->get_next());
            // printf("Check 3\n");
            // printf("Prev: %p\n", prev);
            if (noMark(prev->get_next()) != noMark(curr))
            {
                return internal_find(head, key);
            }
            // printf("Check change!\n");
            if (!is_marked(curr))
            {
                // printf("Case 1\n");
                K currKey = curr->get_key();
                if (currKey >= key)
                {
                    res.first = prev;
                    res.second = curr;
                    return res;
                }
                prev = curr;
                curr = next;
            }
            else
            {
                // printf("Case 2\n");
                if (__sync_bool_compare_and_swap(&(prev->next), noMark(curr), noMark(curr->next))) {
                    // garbage collection - delete(curr)
                    res.first = prev;
                    res.second = curr->next;
                    return res;
                }
                else
                {
                    // printf("Case 2.2\n");
                    return internal_find(head, key);
                }
            }
            // printf("Check 4\n");
        }
    }

public:
    int table_size;
    int (*hash_fn) (K);
    std::vector< LLNode<K,V>* > table;

    DelOptHashTable(int num_buckets, int (*hash) (K)) {
        table_size = num_buckets;
        hash_fn = hash;
        table = std::vector< LLNode<K,V>* >(num_buckets, new LLNode<K, V>(0, 0, NULL)); // dummy values
    }

    bool insert(K key, V val) {
        // printf("In Insert!\n");  
        int hashIndex = hash_fn(key) % table_size;
        LLNode<K,V>* head = noMark(table[hashIndex]);
        LLNode<K,V>* node = new LLNode<K,V>(key, val);
        while(true) {
            std::pair<LLNode<K,V>*, LLNode<K,V>*> res = internal_find(head, key);
            LLNode<K,V>* curr = noMark(res.second);
            LLNode<K,V>* prev = noMark(res.first);
            if (curr != NULL && curr->get_key() == key) {
                return false;
            }
            // printf("Finished internal find!\n");
            node->set_next(curr);
            // printf("Prev next: %p\n", prev->next);
            // printf("Curr: %p\n", curr);
            // printf("New node: %p\n", node);
            if (__sync_bool_compare_and_swap(&(prev->next), noMark(curr), noMark(node)))
            {
                return true;
            }
            // printf("End of loop curr is %p!\n", curr);
        }
    }

    bool remove(K key) {
        // printf("In remove!\n");
        int hashIndex = hash_fn(key) % table_size;
        LLNode<K,V>* head = noMark(table[hashIndex]);
        LLNode<K,V>* prev;
        LLNode<K,V>* curr;
        while(true) {
            std::pair<LLNode<K,V>*, LLNode<K,V>*> res = internal_find(head, key);
            prev = noMark(res.first);
            curr = noMark(res.second);
            if (curr == NULL)
            {
                // printf("Didn't find to remove\n");
                return false;
            }
            if (!__sync_bool_compare_and_swap(&(curr->next), noMark(curr->next), withMark(curr->next)))
            {
                continue;
            }
            if (__sync_bool_compare_and_swap(&(prev->next), noMark(curr), noMark(curr->next))) {
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

    LLNode<K,V>* find(K key) {
        // printf("In lookup!\n");
        int hashIndex = hash_fn(key) % table_size;
        LLNode<K,V>* curr = internal_find(noMark(table[hashIndex]), key).second;
        if (curr != NULL && curr->get_key() != key)
        {
            curr = NULL;
        }
        return curr;
    }
};
