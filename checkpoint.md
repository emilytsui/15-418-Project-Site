# Project Checkpoint: Lock-Free Hash Table Implementations
## Brenda Thayillam & Emily Tsui

---

### [Proposal](https://emilytsui.github.io/ParaHash) | [Checkpoint](https://emilytsui.github.io/ParaHash/checkpoint) | [Final Writeup](https://emilytsui.github.io/ParaHash/final)

---

### Summary
So far, we have created a testing harness for our hash table implementations. We have 2 testing files, both with tens of thousands of instructions, to test correctness of our implementations. We have 7 testing files, all with one million instructions, in order to test performance of our implementations. All the testing files have variable ratios of insert, delete, and lookup. We generated tests based on ratios of the instructions because different instruction patterns will show different speedups, as seen in the graphs in [this research paper](http://www.research.ibm.com/people/m/michael/spaa-2002.pdf) that we have been referencing.

We have also implemented a sequential hash table, as well as a fine-grained hash table that locks on buckets in the hash table. We've seen good speedup going from the sequential hash table to the fine-grained hash table, as would be expected. We have also started implementing a basic lock-free hash table that prevents the ABA problem by allowing for memory leaks. The algorithm is based on the research paper mentioned above, but we replaced hazard pointers with allowing for memory leaks.

### Goals and Deliverables
We have updated our plan with regard to our project. Instead of attempting to implement a delete-optimal hash table and insert-optimal hash table and then building a dynamic balancer between the two, we will implemented multiple different versions of lock-free hash tables. One will be the memory leak version that we have been working on, and another will use the concept of a double CAS. To get around the problem that we don't have a built in double CAS, we will instead attempt to use normal CAS and compile the code for an x86 machine so that a pointer is 4 bytes. The other implementation we plan to code will be compatible with hazard-pointers, and we will be using the hazard pointers implementation in [this library](https://github.com/khizmax/libcds). Finally, we will attempt to implement [RMC](http://www.cs.cmu.edu/~crary/papers/2015/rmc.pdf) into our lock-free hash table. New goals and deliverables were updated in our [proposal page](https://emilytsui.github.io/ParaHash).

### Parallelism Competition
We will be showing our speedup graphs at the competition.

### Preliminary Results
We do not currently have the lock-free hash table implementation working, so we don't have results yet.

### Issues
We've talked to Yicheng and cleared up many issues that we had run into, such as wondering if we had to implement hazard-pointers or double CAS because there aren't built-in implementations of either of these. However, these issues have been mostly resolved, provided that compiling the code for an x86 machine will allow us to get around the double CAS problem. As for the hazard-pointers, we have found a library that provides us with an implementation. Additionally, learning to use and implement RMC concerns us, which is why it is an extra goal that we hope we can achieve.
