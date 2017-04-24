# Project Proposal: Lock-Free Hash Table Implementations
## Brenda Thayillam & Emily Tsui
---

[Checkpoint](https://emilytsui.github.io/ParaHash/checkpoint)

---
### Summary
We will be implementing one lock-free hash table optimized for inserts and one lock-free hash table optimized for deletes and potentially allow the ability for dynamic switching between the two implementations based on access patterns of the program the structures are being used in.

### Background
Lock-free hash tables guarantee that multiple threads performing operations on the data structure will have their operation performed within a finite amount of time independent of other operations. Lock-free data structures generally perform better than lock-based implementations because of their non-blocking characteristic that allows parallel execution of threads. To get around locking, they are implemented using Compare-And-Swap operations, which replaces an old value if the current value is what the thread expects it to be.

### Challenge
It's hard to make a good performance lock-free hash table because of issues that can arise once operations cannot be synchronized with locks. This means there need to be mechanisms in place that maintain correct behavior of the data structure but still provide good performance. While it might not be as hard to optimize for either insert or delete, it's hard to achieve optimal performance on all operations.

### Resources
- We will refer to 15-418 lecture slides and other relevant papers to more thoroughly understand parallel performance metrics
- We will be coding from scratch
- We will be referring to the algorithms discussed in this research paper: http://www.research.ibm.com/people/m/michael/spaa-2002.pdf

### Goals and Deliverables
#### Plan to achieve
- Baseline sequential hash table testing harness
- Implementation of fine-grained lock-based hash table
- Implementation of insert-optimized lock-free hash table (based on research paper)
- Implementation of delete-optimized lock-free hash table

#### Hope to achieve
- Dynamic balancer that can determine when to switch between our two implemented lock-free hash tables based on access patterns

#### Deliverables
- An insert-optimized lock-free hash table
- A delete-optimized lock-free hash table
- Evaluation: Correctness based on sequential hash table testing harness
- Evaluation: Performance compared to sequential and fine-grained hash table implementations

### Platform Choice
The code will be written in C++, and we plan to use GCC's implementation of compare and swap. We will be developing on the GHC machines because our code should not be too compute intensive.

### Schedule (Updated 4/24/17)
- April 10 – 16: Read up on and understand algorithms in research paper & create sequential test harness with different insertion and deletion patterns
- April 17 – 23: Complete fine-grained lock-based hash table and begin basic lock-free hash table with memory leaks
- April 24 – 27: Complete basic lock-free hash table
- April 28 - 30: Study for 15-418 exam
- May 1 - 4: Implement double CAS lock-free implementation (Brenda) and the hazard-pointer lock-free implementation (Emily)
- May 4 - 5: Read and understand [RMC](http://www.cs.cmu.edu/~crary/papers/2015/rmc.pdf)
- May 6 - 10: Attempt to implement RMC into our lock-free implementations
- May 11 - 12: Finalize project, create speedup plots, and finish write-up
