#include <stdlib.h>

#include "seq_hash_table.h"

#define INPUT_SIZE 2000

int hash(int tag) {
  int temp = tag;
  int hashVal = 7;
  while(temp != 0) {
    hashVal *= 31;
    hashVal += temp % 10;
    temp /= 10;
  }
  return abs(hashVal);
}

int main() {
    SeqHashTable<int,int> table(INPUT_SIZE/2, &hash);
    std::vector<int> keys;
    for (int i = 0; i < INPUT_SIZE; i++) {
        int key = rand();
        int val = rand();
        keys.push_back(key);
        table.insert(key, val);
    }

    for (int i = 0; i < INPUT_SIZE; i++) {
        int keyIndex = rand() % keys.size();
        int key = keys.back();
        keys.pop_back();
        table.remove(key);
    }
}