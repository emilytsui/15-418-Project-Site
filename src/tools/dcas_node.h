#include <atomic>

template <typename K, typename V>
class DNode {
private:

    K key;
    V data;

public:

    struct pair {
        uint tag;
        DNode* next;
    };

    std::atomic<pair> nextTag;

    DNode(K ke, V val, DNode* n = NULL, uint t = 0) {
        key = ke;
        data = val;
        // nextTag = new pair();
        pair temp;
        temp.next = n;
        temp.tag = t;
        nextTag.store(temp);
    }

    DNode(DNode* n = NULL) {
        key = 0;
        data = 0;
        // nextTag = new pair();
        pair temp;
        temp.next = NULL;
        temp.tag = 0;
        nextTag.store(temp);
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
        return nextTag.load().next;
    }

    DNode* set_next(DNode* n) {
        pair temp = nextTag.load();
        temp.next = n;
        nextTag.store(temp);
        return this;
    }

    uint get_tag() {
        return nextTag.load().tag;
    }

    DNode* set_tag(uint t) {
        pair temp = nextTag.load();
        temp.tag = t;
        nextTag.store(temp);
        return this;
    }

};