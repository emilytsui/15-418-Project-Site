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

    LLNode<K,V>* internal_find(const K key, LLNode<K,V>** retPrev) {
        // printf("key: %d\n", key);
    int hashIndex = hash_fn(key) % table_size;
    LLNode<K,V>* retCurr;
    LLNode<K,V>* retPrevNext;
    try_again:
        while (true)
        {
            LLNode<K,V>* prev = table[hashIndex];
            LLNode<K,V>* curr = prev->get_next();
            do {
                if (!is_marked(curr))
                {
                    (*retPrev) = prev;
                    retPrevNext = curr;
                }
                prev = unmarked(curr);
                if (prev == NULL)
                {
                    break;
                }
                curr = prev->get_next();
            } while (is_marked(curr) || (unmarked(prev)->get_key() < key));
            retCurr = prev;

            // make sure retPrev and retCurr still adjacent
            if (retPrevNext == retCurr)
            {
                if (retCurr != NULL && is_marked(unmarked(retCurr)->get_next()))
                {
                    goto try_again;
                }
                else
                {
                    return retCurr;
                }
            }

            // remove any marked nodes
            if (__sync_bool_compare_and_swap(&(unmarked((*retPrev))->next), retPrevNext, retCurr))
            {
                if (retCurr != NULL && is_marked(unmarked(retCurr)->get_next()))
                {
                    goto try_again;
                }
                else
                {
                    return retCurr;
                }
            }

            // if (unmarked(curr) == NULL)
            // {
            //     res->prev = prev;
            //     res->curr = curr;
            //     return res;
            // }
            // LLNode<K,V>* next = (unmarked(curr))->get_next();
            // if ((unmarked(prev))->get_next() != unmarked(curr))
            // {
            //     goto try_again;
            // }
            // if (!is_marked(next))
            // {
            //     K currKey = unmarked(curr)->get_key();
            //     // printf("currKey: %d, key: %d, geq: %d\n", currKey, key, currKey >= key);
            //     if (currKey >= key)
            //     {
            //         res->prev = prev;
            //         res->curr = curr;
            //         return res;
            //     }
            //     prev = curr;
            // }
            // else
            // {
            //     if (__sync_bool_compare_and_swap(&(prev->next), unmarked(curr), unmarked(next))) {
            //         // garbage collection - delete(curr)
            //     }
            //     else
            //     {
            //         goto try_again;
            //     }
            // }
            // curr = next;
        }
    }

public:
    int table_size;
    int (*hash_fn) (K);
    std::vector< LLNode<K,V>* > table;

    DelOptHashTable(const int num_buckets, int (*hash) (K)) {
        table_size = num_buckets;
        hash_fn = hash;
        table = std::vector< LLNode<K,V>* >(num_buckets, new LLNode<K, V>(0, 0, NULL)); // dummy values
    }

    bool insert(const K key, const V val) {
        // printf("In Insert!\n");
        // int hashIndex = hash_fn(key) % table_size;
        // LLNode<K,V>* head = table[hashIndex];
        LLNode<K,V>* node = new LLNode<K,V>(key, val);
        LLNode<K, V>* curr;
        LLNode<K, V>* prev;
        while(true) {
            curr = unmarked(internal_find(key, &prev));
            prev = unmarked(prev);
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
        // int hashIndex = hash_fn(key) % table_size;
        // LLNode<K,V>* head = table[hashIndex];
        LLNode<K,V>* prev;
        LLNode<K,V>* curr;
        LLNode<K,V>* next;
        LLNode<K,V>* noMarkCurr;
        while(true) {
            curr = internal_find(key, &prev);
            noMarkCurr = unmarked(curr);
            if (noMarkCurr == NULL || noMarkCurr->get_key() != key)
            {
                // printf("Didn't find to remove\n");
                return false;
            }
            next = noMarkCurr->get_next();
            if (!is_marked(next) &&
                !__sync_bool_compare_and_swap(&(noMarkCurr->next), next, marked(next)))
            {
                break;
            }
            // if (__sync_bool_compare_and_swap(&(unmarked(prev)->next), noMarkCurr, unmarked(next))) {
            //     // garbage collection - delete(curr)
            // }
            // else
            // {
            //     internal_find(head, key);
            // }
            // printf("End of remove loop\n");
        }
        if (!__sync_bool_compare_and_swap(&(unmarked(prev)->next), noMarkCurr, unmarked(next)))
        {
            curr = internal_find(key, &prev);
        }
        return true;
    }

    LLNode<K,V>* find(const K key) {
        // printf("In lookup!\n");
        // int hashIndex = hash_fn(key) % table_size;
        LLNode<K,V>* prev;
        LLNode<K,V>* curr;
        curr = unmarked(internal_find(key, &prev));
        if (curr != NULL && curr->get_key() != key)
        {
            curr = NULL;
        }
        return curr;
    }
};
