#pragma once

template <typename K, typename V>
class LLNode {
private:

    K key;
    V data;
    LLNode<K,V>* next;

public:

    LLNode(K ke, V val, LLNode* n = NULL) {
        key = ke;
        data = val;
        next = n;
    }

    K get_key() {
        return key;
    }

    V get_data() {
        return data;
    }

    void set_data(V item) {
        data = item;
    }

    LLNode* get_next() {
        return next;
    }

    void set_next(LLNode* n) {
        next = n;
    }

};