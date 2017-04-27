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
        std::pair<DNode<K,V>*, uint> curr = prev->get_nextTag();
        uint ctag = curr.second;
        std::pair<DNode<K,V>*, DNode<K,V>*> res;
        while (true)
        {
            if (unmarked(curr.first) == NULL)
            {
                res.first = prev;
                res.second = unmarked(curr.first);
                return res;
            }
            std::pair<DNode<K,V>*, uint> next = (unmarked(curr.first))->get_nextTag();
            if ((unmarked(prev))->get_nextTag() != curr)
            {
                goto try_again;
            }
            if (!is_marked(curr.first->get_next()))
            {
                K currKey = unmarked(curr)->get_key();
                // printf("currKey: %d, key: %d, geq: %d\n", currKey, key, currKey >= key);
                if (currKey >= key)
                {
                    res.first = prev;
                    res.second = curr.first;
                    return res;
                }
                prev = curr.first;
                ptag = curr.second;
            }
            else
            {
                std::pair<DNode<K,V>*, uint> old;
                old.first = curr.first;
                old.second = ptag;
                std::pair<DNode<K,V>*, uint> newVal;
                newVal.first = unmarked(next.first);
                newVal.second = ptag + 1;
                if (__sync_bool_compare_and_swap(&(prev->get_nextTag()), old, newVal)) {
                    ctag = ptag + 1;
                }
                else
                {
                    goto try_again;
                }
            }
            next.second = ctag;
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
            std::pair<DNode<K,V>*, uint> old;
            old.first = curr;
            old.second = ptag;
            std::pair<DNode<K,V>*, uint> newVal;
            newVal.first = node;
            newVal.second = ptag + 1;
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
            std::pair<DNode<K,V>*, uint> old1;
            old1.first = next;
            old1.second = ctag;
            std::pair<DNode<K,V>*, uint> newVal1;
            newVal1.first = marked(next);
            newVal1.second = ctag + 1;
            if (!is_marked(next) &&
                !__sync_bool_compare_and_swap(&(unmarked(curr)->nextTag), old1, newVal1))
            {
                continue;
            }

            std::pair<DNode<K,V>*, uint> old2;
            old2.first = curr;
            old2.second = ptag;
            std::pair<DNode<K,V>*, uint> newVal2;
            newVal2.first = next;
            newVal2.second = ptag + 1;
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
