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

    /* returns the "prev" of the node that has the key match */
    LLNode<K,V>* internal_find(LLNode<K,V>* head, K key) {
        LLNode<K,V>* prev = noMark(head);
        // printf("Check 1\n");
        LLNode<K,V>* curr = noMark(head->get_next());
        while (true)
        {
            // printf("Prev: %p\n", prev);
            // printf("Curr: %p\n", curr);
            if (curr == NULL)
            {
                return NULL;
            }
            // printf("Check 2\n");
            LLNode<K,V>* next = noMark(curr->get_next());
            // printf("Check 3\n");
            if (noMark(prev->get_next()) != curr)
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
        LLNode<K,V>* prev = noMark(head);
        LLNode<K,V>* node = new LLNode<K,V>(key, val);
        while(true) {
            if (internal_find(head, key) != NULL) {
                return false;
            }
            // printf("Finished internal find!\n");
            LLNode<K,V>* curr = noMark(head->get_next());
            while (curr != NULL)
            {
                if (is_marked(curr))
                {
                    // if (__sync_bool_compare_and_swap(&(prev->get_next()->get_key()), curr->get_key(), key) &&
                    //     __sync_bool_compare_and_swap(&(prev->get_next()->get_data()), curr->get_data(), val))
                    // {
                    if (__sync_bool_compare_and_swap(&(prev->next), curr, curr->set_key(key)->set_data(val))) // Without mark
                    {
                        unmark(curr);
                        return true;
                    }
                    else if (__sync_bool_compare_and_swap(&(prev->next), withMark(curr), withMark(curr->set_key(key)->set_data(val)))) // With mark
                    {
                        unmark(curr);
                        return true;
                    }
                }
                prev = curr;
                curr = noMark(curr->get_next());
            }
            curr = noMark(head->get_next());
            node->set_next(curr);
            if (__sync_bool_compare_and_swap(&(table[hashIndex]->next), curr, node))
            {
                return true;
            }
        }
    }

    LLNode<K,V>* remove(K key) {
        // printf("In remove!\n");
        int hashIndex = hash_fn(key) % table_size;
        LLNode<K,V>* head = noMark(table[hashIndex]);
        while(true) {
            if (internal_find(head, key) == NULL)
            {
                // printf("Didn't find to remove\n");
                return NULL;
            }
            LLNode<K,V>* prev = head;
            LLNode<K,V>* curr = noMark(prev->get_next());
            while (curr != NULL && curr->get_key() != key)
            {
                prev = curr;
                curr = noMark(curr->get_next());
            }
            if (__sync_bool_compare_and_swap(&(prev->next), curr, mark(curr)) || 
                __sync_bool_compare_and_swap(&(prev->next), withMark(curr), withMark(mark(curr)))) { // Fix Me!
                return curr;
            }
            // printf("End of remove loop\n");
        }
    }

    LLNode<K,V>* find(K key) {
        // printf("In lookup!\n");
        int hashIndex = hash_fn(key) % table_size;
        return internal_find(noMark(table[hashIndex]), key);
    }
};
