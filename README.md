# Project Proposal: Parallel Debugging Tool for OpenMP and CUDA
## Brenda Thayillam & Emily Tsui
---


### Summary
We will be creating a parallel debugging tool for C++ programs that use OpenMP or CUDA programs on GPUs, specifially analyzing arithmetic intensity, memory access patterns, percentage peak performance, and potential speedup. We will be using this on the GHC machines, and the user will be able to provide specifications for the machines to simulate it on.

### Background
Arithmetic intensity
False sharing
Percentage peak performance
Potential speedup

### Challenge
In order to make the parallel debugging tool as useful as possible, we need to have a deep understanding of the concepts mentioned above and be able to analyze how different lines of code could affect performance. The memory access pattern will be especially hard to analyze, as we will need a thorough understanding in order to come up with a good method of analysis for it.

### Resources
- We will refer to 15-418 lecture slides and other relevant papers to more thoroughly understand parallel performance metrics
- We will be coding from scratch
- The project will be developed on GHC machines and locally

### Goals and Deliverables
##### Plan to achieve
- Good performance of the debugging tool
- Allows the user to provide specifications for the machine they would like to simulate on
- Analyzes arithmetic intensity of OpenMP and CUDA code
- Analyzes percentage peak performance of OpenMP and CUDA code
- Analyzes problem-constrained scaling of OpenMP and CUDA code
- Analyzes false sharing of OpenMP and CUDA code

##### Hope to achieve
- Analyzes other memory access patterns of OpenMP and CUDA code
- Analyzes workload balance of OpenMP and CUDA code

##### Deliverables
- A C++ program which can analyze parallel performance problem in OpenMP and CUDA code

### Platform Choice
The code will be written in C++ because we plan to use Open MP to gain good performance in our code. We will be developing on the GHC machines because our code should not be too compute intensive.

### Schedule
