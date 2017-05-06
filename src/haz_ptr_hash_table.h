#include <vector>

#include "tools/haz_ptr_node.h"
#include "tools/cds-2.2.0/build/include/cds/init.h"
#include "tools/cds-2.2.0/build/include/cds/gc/hp.h"

static CDS_CONSTEXPR size_t const hazardPtrCount = 16;

template <typename T>
struct disposer {
    void operator ()( T * p ) {
        delete p;
    }
};

template <typename K, typename V>
class HazPtrHashTable {
private:

    std::vector< HPNode<K,V>** > hazardPtrs;

    HPNode<K,V>* marked(HPNode<K,V>* node) {
        return (HPNode<K,V>*)((unsigned long) node | 0x1);
    }

    HPNode<K,V>* unmarked(HPNode<K,V>* node) {
        return (HPNode<K,V>*)((unsigned long) node & (-1 << 1));
    }

    bool is_marked(HPNode<K,V>* node) {
        return (((unsigned long) node) & 0x1);
    }

    std::pair<HPNode<K,V>*, HPNode<K,V>*> internal_find(HPNode<K,V>*& head, const K& key, const int& id) {
        // printf("key: %d\n", key);
    try_again:
        HPNode<K,V>* prev = head;
        HPNode<K,V>* curr = prev->get_next();
        std::pair<HPNode<K,V>*, HPNode<K,V>*> res;
        // &hazardPtrs[3*id+1] = curr;
        guards.protect(3*id+1, prev->next);
        if ((unmarked(prev))->get_next() != curr)
        {
            goto try_again;
        }
        while (true)
        {
            if (unmarked(curr) == NULL)
            {
                res.first = prev;
                res.second = unmarked(curr);
                return res;
            }
            HPNode<K,V>* next = (unmarked(curr))->get_next();
            std::atomic< HPNode<K,V>* > nextAtomic;
            nextAtomic.store(next);
            // *&hazardPtrs[3*id] = next;
            guards.protect(3*id, (unmarked(curr))->next);
            if ((unmarked(curr))->get_next() != next) {
                goto try_again;
            }
            if ((unmarked(prev))->get_next() != curr)
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
                guards.protect(3*id+2, prev->next);
                prev = curr;
                // *&hazardPtrs[3*id+2] = curr;
            }
            else
            {
                // if (__sync_bool_compare_and_swap(&(prev->next), curr.load(), unmarked(next))) {
                if ((prev->next).compare_exchange_weak(curr, unmarked(next))) {
                    // garbage collection - delete(curr)
                    cds::gc::HP::retire< disposer< HPNode<K,V> > >(curr);
                }
                else
                {
                    goto try_again;
                }
            }
            guards.protect(3*id+1, unmarked(curr)->next);
            curr = next;
            // *&hazardPtrs[3*id+1] = next;
        }
    }

public:
    int table_size;
    int (*hash_fn) (K);
    std::vector< HPNode<K,V>* > table;
    cds::gc::HP::GuardArray< hazardPtrCount * 3 > guards;

    HazPtrHashTable(const int num_buckets, int (*hash) (K)) {
        table_size = num_buckets;
        hash_fn = hash;
        table = std::vector< HPNode<K,V>* >(num_buckets, NULL);
        // table.resize(num_buckets); // dummy values
        for (int i = 0; i < num_buckets; i++)
        {
            table[i] = new HPNode<K, V>(0, 0);
        }
    }

    bool insert(const K& key, const V& val, const int& id) {
        // printf("In Insert!\n");
        int hashIndex = hash_fn(key) % table_size;
        HPNode<K,V>* head = table[hashIndex];
        HPNode<K,V>* node = new HPNode<K,V>(key, val);
        bool result;
        while(true) {
            std::pair<HPNode<K,V>*, HPNode<K,V>*> res = internal_find(head, key, id);
            HPNode<K,V>* curr = res.second;
            HPNode<K,V>* prev = res.first;
            if (curr != NULL && (curr->get_key() == key)) {
                result = false;
                break;
            }
            // printf("Finished internal find!\n");
            node->set_next(curr);
            // printf("Prev next: %p\n", prev->next);
            // printf("Curr: %p\n", curr);
            // printf("New node: %p\n", node);
            // if (__sync_bool_compare_and_swap(&((prev->next).load()), curr, node))
            if ((prev->next).compare_exchange_weak(curr, node))
            {
                result = true;
                break;
            }
        }
        guards.clear(3*id);
        guards.clear(3*id+1);
        guards.clear(3*id+2);
        return result;
    }

    bool remove(const K& key, const int& id) {
        // printf("In remove!\n");
        int hashIndex = hash_fn(key) % table_size;
        HPNode<K,V>* head = table[hashIndex];
        HPNode<K,V>* prev;
        HPNode<K,V>* curr;
        bool result;
        while(true) {
            std::pair<HPNode<K,V>*, HPNode<K,V>*> res = internal_find(head, key, id);
            prev = res.first;
            curr = res.second;
            if (unmarked(curr) == NULL)
            {
                result = false;
                break;
            }
            HPNode<K,V>* next = unmarked(curr)->get_next();
            // HPNode<K,V>* next = unmarked(curr)->get_next();
            if (!is_marked(next) &&
                !(unmarked(curr)->next).compare_exchange_weak(next, marked(next)))
                // !__sync_bool_compare_and_swap(&(unmarked(curr)->next), next, marked(next)))
            {
                continue;
            }
            // if (__sync_bool_compare_and_swap(&(unmarked(prev)->next), curr, next)) {
            if ((unmarked(prev)->next).compare_exchange_weak(curr, next)) {
                // garbage collection - delete(curr)
                cds::gc::HP::retire< disposer< HPNode<K,V> > >(curr);
            }
            else
            {
                internal_find(head, key, id);
            }
            result = true;
            break;
            // printf("End of remove loop\n");
        }
        guards.clear(3*id);
        guards.clear(3*id+1);
        guards.clear(3*id+2);
        return result;
    }

    HPNode<K,V>* find(const K& key, const int& id) {
        // printf("In lookup!\n");
        int hashIndex = hash_fn(key) % table_size;
        HPNode<K,V>* curr = unmarked(internal_find(table[hashIndex], key, id).second);
        if (curr != NULL && curr->get_key() != key)
        {
            curr = NULL;
        }
        guards.clear(3*id);
        guards.clear(3*id+1);
        guards.clear(3*id+2);
        return curr;
    }
};
