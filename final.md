# Final Writeup: Lock-Free Hash Table Implementations
## Brenda Thayillam & Emily Tsui

---

### [Proposal](https://emilytsui.github.io/ParaHash) | [Checkpoint](https://emilytsui.github.io/ParaHash/checkpoint) | [Final Writeup](https://emilytsui.github.io/ParaHash/final)

---

### Summary
For benchmarks, we implemented a sequential hash table to be run on only one thread and a fine-grained lock hash table that locks on each bucket. For our lock-free implementations, the first version we implemented has no memory management and allows for memory leaks to get around the ABA problem, which we based off [this research paper](http://www.research.ibm.com/people/m/michael/spaa-2002.pdf). We also implemented a version that uses the concept of a double CAS with a version tag, but since a double CAS is not available on the 64-bit machine, we used normal CAS and compiled our code for an x86 machine so that a pointer is 4 bytes. Our final implementation is compatible with hazard pointers provided by the [libcds](https://github.com/khizmax/libcds) library.

### Challenges
The ABA problem is a major challenge for lock-free data structures, so our implementations address it in different ways. Since lock free hash tables compare and swap at the node level, the ABA problem exists with the state of the node seeming to be the same based on address when it is in fact not in the same state. The most basic way to combat the ABA problem was to not free nodes after deleting them, ensuring that the same address couldn't be reused. We had to combine this ABA prevention scheme with a two-step deletion process required for hash table operations that didn't corrupt the overall structure. The two step deletion involved marking each deleted node followed by actually deleting the node form the hash table. Thus, even after addressing the ABA problem in this manner, we ran into some difficulties with marking the `next` pointer of each node; the `next` pointer of a node is marked if the node is about to be deleted, but of course when comparing pointers, we had to unmark the nodes and keep track of where we were marking and unmarking, which caused a lot of correctness problems. After getting the memory leak version of the lock-free hash table working, the hazard pointer implementation was mainly just sifting through the library source code to figure out how to integrate hazard pointers into our code.

One of our memory managing hash table implementations that also prevented the ABA problem used the double-compare-and-swap operation on nodes with tags. In order to get this version working properly, we had to create an effective way of blocking the next pointer and the tag together in memory in order to perform a DCAS operation on the entire chunk of data. We further had to get it compiled to x86 in order to even make the DCAS feasible. This implementation caused difficulties because it still used the two-step deletion process and thus needed to be able to modify the next and tag values individually in addition to together with the DCAS operation. Realizing that the gcc compare-and-swap primitive only accepted long long values, we resorted to using std::atomic::compare_exchange_weak which resulted in a lot of experimentation with the various memory synchronization methods in order to find the optimal.

### Preliminary Results

