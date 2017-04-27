#pragma once

template <typename K, typename V>
class DNode {
private:

    K key;
    V data;

public:
    std::pair<DNode*, uint> nextTag;

    DNode(K ke, V val, DNode* n = NULL, uint t = 0) {
        key = ke;
        data = val;
        nextTag.first = n;
        nextTag.second = t;
    }

    DNode(DNode* n = NULL) {
        key = 0;
        data = 0;
        nextTag.first = NULL;
        nextTag.second = 0;
    }

    K get_key() {
        return key;
    }

    DNode<K,V>* set_key(K k) {
        key = k;
        return this;
    }

    V get_data() {
        return data;
    }

    DNode<K,V>* set_data(V item) {
        data = item;
        return this;
    }

    DNode* get_next() {
        return nextTag.first;
    }

    DNode* set_next(DNode* n) {
        nextTag.first = n;
        return this;
    }

    uint get_tag() {
        return nextTag.second;
    }

    DNode* set_tag(uint t) {
        nextTag.second = t;
        return this;
    }

    std::pair<DNode*, uint> get_nextTag() {
        return nextTag;
    }

};