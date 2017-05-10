#pragma once

// #include <atomic>

template <typename K, typename V>
class HPNode {
private:

    K key;
    V data;

public:
    // std::atomic< HPNode<K,V>* > next;
    HPNode<K,V>* next;

    HPNode(K ke, V val, HPNode* n = NULL) {
        key = ke;
        data = val;
        // next.store(n);
        next = n;
    }

    K get_key() {
        return key;
    }

    HPNode<K,V>* set_key(K k) {
        key = k;
        return this;
    }

    V get_data() {
        return data;
    }

    HPNode<K,V>* set_data(V item) {
        data = item;
        return this;
    }

    HPNode<K,V>* get_next() {
        // return next.load();
        return next;
    }

    HPNode<K,V>* set_next(HPNode* n) {
        // next.store(n);
        next = n;
        return this;
    }

};