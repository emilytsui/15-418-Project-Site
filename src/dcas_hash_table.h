// Must be compiled for 32-bit machine

#include <vector>
#include "tools/dcas_node.h"

template <typename K, typename V>
class DCASHashTable {
private:

    DNode<K,V>* marked(DNode<K,V>* node) {
        return (DNode<K,V>*)((uint) node | 0x1);
    }

    DNode<K,V>* unmarked(DNode<K,V>* node) {
        return (DNode<K,V>*)((uint) node & (-1 << 1));
    }

    bool is_marked(DNode<K,V>* node) {
        return (((uint) node) & 0x1);
    }

    std::pair<DNode<K,V>*, DNode<K,V>*> internal_find(DNode<K,V>* head, const K key) {
        // printf("key: %d\n", key);
    try_again:
        DNode<K,V>* prev = head;
        uint ptag = prev->get_tag();
        DNode<K, V>* curr = prev->get_next();
        uint ctag = unmarked(curr)->get_tag();
        std::pair<DNode<K,V>*, DNode<K,V>*> res;
        while (true)
        {
            if (unmarked(curr) == NULL)
            {
                res.first = prev;
                res.second = unmarked(curr);
                return res;
            }
            DNode<K, V>* next = (unmarked(curr))->get_next();
            if ((unmarked(prev))->get_next() != curr || unmarked(prev)->get_tag() != ptag)
            {
                goto try_again;
            }
            if (!is_marked(curr->get_next()))
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
                ptag = curr->get_tag();
            }
            else
            {
                typename DNode<K, V>::pair* old;
                old->next = unmarked(curr);
                old->tag = ptag;
                typename DNode<K, V>::pair* newVal;
                newVal->next = unmarked(next);
                newVal->tag = ptag + 1;
                if (__sync_bool_compare_and_swap(&(prev->nextTag), old, newVal)) {
                    ctag = ptag + 1;
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
    std::vector< DNode<K,V>* > table;

    DCASHashTable(const int num_buckets, int (*hash) (K)) {
        table_size = num_buckets;
        hash_fn = hash;
        table = std::vector< DNode<K,V>* >(num_buckets, NULL);
        // table.resize(num_buckets); // dummy values
        for (int i = 0; i < num_buckets; i++)
        {
            DNode<K,V>* node = new DNode<K, V>(0, 0, NULL, 0);
            table[i] = node;
        }
    }

    bool insert(const K key, const V val) {
        // printf("In Insert!\n");
        int hashIndex = hash_fn(key) % table_size;
        DNode<K,V>* head = table[hashIndex];
        DNode<K,V>* node = new DNode<K, V>(key, val, NULL, 0);

        while(true) {
            std::pair<DNode<K,V>*, DNode<K,V>*> res = internal_find(head, key);
            DNode<K,V>* curr = res.second;
            DNode<K,V>* prev = res.first;
            uint ptag = prev->get_tag();
            if (curr != NULL && (curr->get_key() == key)) {
                return false;
            }
            // printf("Finished internal find!\n");
            node->set_next(curr);
            // printf("Prev next: %p\n", prev->next);
            // printf("Curr: %p\n", curr);
            // printf("New node: %p\n", node);
            typename DNode<K, V>::pair* old;
            old->next = curr;
            old->tag = ptag;
            typename DNode<K, V>::pair* newVal;
            newVal->next = node;
            newVal->tag = ptag + 1;
            if (__sync_bool_compare_and_swap(&(prev->nextTag), old, newVal))
            {
                return true;
            }
            // printf("End of loop curr is %p!\n", curr);
        }
    }

    bool remove(const K key) {
        // printf("In remove!\n");
        int hashIndex = hash_fn(key) % table_size;
        DNode<K,V>* head = table[hashIndex];
        DNode<K,V>* prev;
        DNode<K,V>* curr;
        while(true) {
            std::pair<DNode<K,V>*, DNode<K,V>*> res = internal_find(head, key);
            prev = res.first;
            curr = res.second;
            if (unmarked(curr) == NULL)
            {
                // printf("Didn't find to remove\n");
                return false;
            }
            uint ctag = curr->get_tag();
            uint ptag = prev->get_tag();
            DNode<K,V>* next = unmarked(curr)->get_next();
            typename DNode<K, V>::pair* old1;
            old1->next = next;
            old1->tag = ctag;
            typename DNode<K, V>::pair* newVal1;
            newVal1->next = marked(next);
            newVal1->tag = ctag + 1;
            if (!is_marked(next) &&
                !__sync_bool_compare_and_swap(&(unmarked(curr)->nextTag), old1, newVal1))
            {
                continue;
            }

            typename DNode<K, V>::pair* old2;
            old2->next = curr;
            old2->tag = ptag;
            typename DNode<K, V>::pair* newVal2;
            newVal2->next = next;
            newVal2->tag = ptag + 1;
            if (__sync_bool_compare_and_swap(&(unmarked(prev)->nextTag), old2, newVal2)) {
                delete(curr);
            }
            else
            {
                internal_find(head, key);
            }
            return true;
            // printf("End of remove loop\n");
        }
    }

    DNode<K,V>* find(const K key) {
        // printf("In lookup!\n");
        int hashIndex = hash_fn(key) % table_size;
        DNode<K,V>* curr = unmarked(internal_find(table[hashIndex], key).second);
        if (curr != NULL && curr->get_key() != key)
        {
            curr = NULL;
        }
        return curr;
    }
};
