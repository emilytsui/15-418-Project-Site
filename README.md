# Project Proposal: Lock-Free Hash Table Implementations
## Brenda Thayillam & Emily Tsui
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

### Schedule
- April 10 – 16: Read up on and understand algorithms in research paper & create sequential test harness with different insertion and deletion patterns
- April 17 – 23: Complete fine-grained lock-based hash table and delete-optimized lock-free hash table
- April 24 – 30: Complete insert-optimized lock-free hash table
- May 1 – 7: Run performance testing and if time, work on dynamic balancer between the two implementations
- May 8 – 12: Finish up and finalize project
