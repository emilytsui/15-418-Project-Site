#pragma once

template <typename K, typename V>
class LLNode {
private:

    K key;
    V data;

public:
    LLNode<K,V>* next;

    LLNode(K ke, V val, LLNode* n = NULL) {
        key = ke;
        data = val;
        next = n;
    }

    LLNode(LLNode* n = NULL) {
    }

    K get_key() {
        return key;
    }

    LLNode<K,V>* set_key(K k) {
        key = k;
        return this;
    }

    V get_data() {
        return data;
    }

    LLNode<K,V>* set_data(V item) {
        data = item;
        return this;
    }

    LLNode* get_next() {
        return next;
    }

    LLNode* set_next(LLNode* n) {
        next = n;
        return this;
    }

};