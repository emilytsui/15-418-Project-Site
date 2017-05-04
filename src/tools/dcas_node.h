template <typename K, typename V>
class DNode {
private:

    K key;
    V data;

public:

    struct pair {
        DNode* next;
        uint tag;
    };

    pair* nextTag;

    DNode(K ke, V val, DNode* n = NULL, uint t = 0) {
        key = ke;
        data = val;
        nextTag = new pair();
        nextTag->next = n;
        nextTag->tag = t;
    }

    DNode(DNode* n = NULL) {
        key = 0;
        data = 0;
        nextTag = new pair();
        nextTag->next = NULL;
        nextTag->tag = 0;
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
        return nextTag->next;
    }

    DNode* set_next(DNode* n) {
        nextTag->next = n;
        return this;
    }

    uint get_tag() {
        return nextTag->tag;
    }

    DNode* set_tag(uint t) {
        nextTag->tag = t;
        return this;
    }

};