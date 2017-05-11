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
We found in our preliminary research that inserts and deletes are the most commonly performed operations. Thus for our tests with various percentages of deletes and inserts & lookups happening with the same likelihood, our results were:
![30% Delete Plot](30p_del_plot.png)
![20% Delete Plot](20p_del_plot.png)
![10% Delete Plot](10p_del_plot.png)
In all three of these plots, the fine grained and memory leak version peak at 16 threads because that is the total number of available execution contexts on the GHC machines. Then for more threads beyond this point, the memory leak lock free version performs slightly better than the fine grained version because for the locking version if the thread holding the bucket's lock gets scheduled out then no progress can be made on operations on that bucket. Although the DCAS and hazard pointer had less overall speedup due to the overhead of memory management, the above results clearly show that these results scale better with more threads. 

Then evaluated on a workload with an uniform amount of all three operations, we achieved:
![Uniform Plot](Uniform_plot.png)
This plot clearly indicates that in the the uniform case, DCAS and hazard pointers scale well and perform better than the fine grained and memory leak versions at higher thread counts.

To further explore the effects of the load factor on the performance, we found:
![Uniform Plot](load_factor_plot.png)
In this plot it is evident that both the fine grained and memory leak version indicate a general increase in speedup with a larger load factor although the memory leak version has a greater speedup (due to allowing multiple operations on the same bucket simultaneously). The DCAS performance stays relatively the same while the hazard pointer implementation gets worse with larger load factors.
