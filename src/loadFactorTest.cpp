#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <string>
#include <cassert>
#include <pthread.h>

#include "tools/cycle_timer.h"
#include "seq_hash_table.h"
#include "fg_hash_table.h"
#include "mem_leak_hash_table.h"
#include "haz_ptr_hash_table.h"
#include "tools/cds-2.2.0/build/include/cds/init.h"
#include "tools/cds-2.2.0/build/include/cds/gc/hp.h"

#define MAX_THREADS 64

#define ALL_TESTS

enum Instr {
    insert,
    del,
    lookup
};

static int numThreads;
static std::vector<std::pair<Instr, std::pair<int, int> > > input;
SeqHashTable<int, int>* baseline;
FgHashTable<int, int>* htable;
MemLeakHashTable<int, int>* lockFreeTable;
HazPtrHashTable<int, int>* hazPtrTable;

std::string filename = "tests/load_factor_test.txt";

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

void parseText(const std::string &filename)
{
    std::ifstream infile;
    infile.open(filename.c_str());
    input.resize(0);
    while(!infile.eof())
    {
        std::string str;
        std::pair<Instr, std::pair<int, int> > task;
        getline(infile, str);
        if (str.length() > 0)
        {
            char instr = str[0];
            switch(instr) {
            case 'L':
                task.first = lookup;
                break;
            case 'I':
                task.first = insert;
                break;
            case 'D':
                task.first = del;
                break;
            default:
                break;
            }

            std::pair<int, int> keyval;
            std::string rest = str.substr(2);
            int spcIdx = rest.find_first_of(' ');
            keyval.first = atoi(rest.substr(0, spcIdx).c_str());
            keyval.second = atoi(rest.substr(spcIdx + 1).c_str());
            task.second = keyval;
            input.push_back(task);
        }
    }
}

void* hazPtrRun(void *arg) {
    // Attach the thread to libcds infrastructure
    cds::threading::Manager::attachThread();
    int id = *(int*)arg;
    int instrPerThread = input.size() / numThreads;
    int start = instrPerThread * id;
    int end = (start + instrPerThread < input.size()) ? (start + instrPerThread) : input.size();
    for (int i = start; i < end; i++)
    {
        std::pair<Instr, std::pair<int, int> > instr = input[i];
        switch(instr.first)
        {
            case insert:
                // printf("Thread %d insert: %d\n", id, instr.second.first);
                hazPtrTable->insert(instr.second.first, instr.second.second, id);
                break;
            case del:
                // printf("Thread %d delete: %d\n", id, instr.second.first);
                hazPtrTable->remove(instr.second.first, id);
                break;
            case lookup:
                // printf("Thread %d lookup: %d\n", id, instr.second.first);
                hazPtrTable->find(instr.second.first, id);
                break;
            default:
                break;
        }
    }
    // Detach thread when terminating
    cds::threading::Manager::detachThread();
    pthread_exit(NULL);
}

void* lockFreeRun(void* arg) {
    int id = *(int*)arg;
    int instrPerThread = input.size() / numThreads;
    int start = instrPerThread * id;
    int end = (start + instrPerThread < input.size()) ? (start + instrPerThread) : input.size();
    for (int i = start; i < end; i++)
    {
        std::pair<Instr, std::pair<int, int> > instr = input[i];
        LLNode<int, int>* res;
        switch(instr.first)
        {
            case insert:
                lockFreeTable->insert(instr.second.first, instr.second.second);
                break;
            case del:
                lockFreeTable->remove(instr.second.first); // Can fail
                // printf("Deleted node: %p\n", res);
                break;
            case lookup:
                res = lockFreeTable->find(instr.second.first); // Can fail
                // printf("Lookup returned: %p\n", res);
                break;
            default:
                break;
        }
    }
    pthread_exit(NULL);
}

void* fgRun(void* arg)
{
    int id = *(int*)arg;
    int instrPerThread = input.size() / numThreads;
    int start = instrPerThread * id;
    int end = (start + instrPerThread < input.size()) ? (start + instrPerThread) : input.size();
    for (int i = start; i < end; i++)
    {
        std::pair<Instr, std::pair<int, int> > instr = input[i];
        switch(instr.first)
        {
            case insert:
                htable->insert(instr.second.first, instr.second.second);
                break;
            case del:
                htable->remove(instr.second.first); // Can fail
                break;
            case lookup:
                htable->find(instr.second.first); // Can fail
                break;
            default:
                break;
        }
    }
    pthread_exit(NULL);
}

double seqRun(SeqHashTable<int, int>* htable)
{
    double startTime = CycleTimer::currentSeconds();
    for (int i = 0; i < input.size(); i++)
    {
        std::pair<Instr, std::pair<int, int> > instr = input[i];
        switch(instr.first)
        {
            case insert:
                htable->insert(instr.second.first, instr.second.second);
                break;
            case del:
                htable->remove(instr.second.first);
                break;
            case lookup:
                htable->find(instr.second.first);
                break;
            default:
                break;
        }
    }
    double dt = CycleTimer::currentSeconds() - startTime;
    printf("Sequential Test complete in %f ms!\n", (1000.f * dt));
    return dt;
}

int main() {

    pthread_t threads[MAX_THREADS];
    int ids[MAX_THREADS];
    for (uint z = 0; z < MAX_THREADS; z++)
    {
        ids[z] = z;
    }
    double baseTime;
    for (int load_fac = 250; load_fac <= 1000; load_fac *= 2) {
    // for (int load_fac = 1; load_fac <= 20; load_fac = load_fac == 1 ? 5 : load_fac + 5) {
        printf("\nPerformance Testing file with load factor %d: %s on fine-grained lock-based hash table\n", load_fac, filename.c_str());
        parseText(filename.c_str());
        baseline = new SeqHashTable<int, int>(200000/load_fac, &hash);
        baseTime = seqRun(baseline);
        for (uint j = 1; j <= MAX_THREADS; j *= 2)
        {
            double bestDt = 0;
            for (int a = 0; a < 5; a++) {
                htable = new FgHashTable<int, int>(200000/load_fac, &hash);
                numThreads = j;
                double startTime = CycleTimer::currentSeconds();
                for (uint id = 0; id < j; id++)
                {
                    pthread_create(&threads[id], NULL, fgRun, &ids[id]);
                }
                for (uint id = 0; id < j; id++)
                {
                    pthread_join(threads[id], NULL);
                }
                double dt = CycleTimer::currentSeconds() - startTime;
                bestDt = bestDt == 0 ? dt : std::min(bestDt, dt);
                delete(htable);
            }
            printf("%d Thread Fine-Grained Test completed in %f ms!\n", numThreads, (1000.f * bestDt));
            printf("%d Thread Speedup: %f\n", j, (baseTime / bestDt));
        }
        printf("\nPerformance Testing file with load factor %d: %s on lock-free hash table with memory leaks\n", load_fac, filename.c_str());
        for (uint j = 1; j <= MAX_THREADS; j *= 2)
        {
            double bestDt = 0;
            for (int a = 0; a < 5; a++) {
                lockFreeTable = new MemLeakHashTable<int, int>(200000/load_fac, &hash);
                numThreads = j;
                double startTime = CycleTimer::currentSeconds();
                for (uint id = 0; id < j; id++)
                {
                    pthread_create(&threads[id], NULL, lockFreeRun, &ids[id]);
                }
                for (uint id = 0; id < j; id++)
                {
                    pthread_join(threads[id], NULL);
                }
                double dt = CycleTimer::currentSeconds() - startTime;
                bestDt = bestDt == 0 ? dt : std::min(bestDt, dt);
                delete(lockFreeTable);
            }
            printf("%d Thread Lock-Free with Memory Leaks Test completed in %f ms!\n", numThreads, (1000.f * bestDt));
            printf("%d Thread Speedup: %f\n", j, (baseTime / bestDt));
        }
        printf("\nPerformance Testing file with load factor %d: %s on lock-free hash table with hazard pointers\n", load_fac, filename.c_str());
        for (uint j = 1; j <= 64; j *= 2)
        {
            double bestDt = 0;
            for (int a = 0; a < 5; a++) {
                cds::Initialize();
                {
                    cds::gc::HP hpGC(MAX_THREADS*3, 50);
                    cds::threading::Manager::attachThread();

                    hazPtrTable = new HazPtrHashTable<int, int>(200000/load_fac, &hash);
                    numThreads = j;
                    double startTime = CycleTimer::currentSeconds();
                    for (uint id = 0; id < j; id++)
                    {
                        pthread_create(&threads[id], NULL, hazPtrRun, &ids[id]);
                    }
                    for (uint id = 0; id < j; id++)
                    {
                        pthread_join(threads[id], NULL);
                    }
                    double dt = CycleTimer::currentSeconds() - startTime;
                    bestDt = bestDt == 0 ? dt : std::min(bestDt, dt);
                    delete(hazPtrTable);
                }
                cds::Terminate();
            }
            printf("%d Thread Hazard Pointer Test completed in %f ms!\n", numThreads, (1000.f * bestDt));
            printf("%d Thread Speedup: %f\n", j, (baseTime / bestDt));
        }
        delete(baseline);
    }
}