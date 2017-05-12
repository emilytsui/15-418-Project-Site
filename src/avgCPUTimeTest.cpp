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
std::vector<double> seqOpTimes;
std::vector<double> fgOpTimes;
std::vector<double> memOpTimes;
std::vector<double> hazOpTimes;
std::vector<double> seqOpCounts;
std::vector<double> fgOpCounts;
std::vector<double> memOpCounts;
std::vector<double> hazOpCounts;

const char *args[] = {"tests/load_factor_test.txt",
                      "tests/5p_del_5p_ins.txt",
                      "tests/10p_del_10p_ins.txt"};
std::vector<std::string> testfiles(args, args + sizeof(args)/sizeof(args[0]));

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

double getSum(int index, std::vector<double> vec) {
    double total = 0.0;
    for (uint i = index; i < vec.size(); i+= 3) {
        total += vec[i];
    }
    return total;
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
    double startTime;
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
                startTime = CycleTimer::currentSeconds();
                // printf("Thread %d insert: %d\n", id, instr.second.first);
                hazPtrTable->insert(instr.second.first, instr.second.second, id);
                hazOpTimes[id*3+0] += CycleTimer::currentSeconds() - startTime;
                hazOpCounts[id*3+0] += 1;
                break;
            case del:
                startTime = CycleTimer::currentSeconds();
                // printf("Thread %d delete: %d\n", id, instr.second.first);
                hazPtrTable->remove(instr.second.first, id);
                hazOpTimes[id*3+1] += CycleTimer::currentSeconds() - startTime;
                hazOpCounts[id*3+1] += 1;
                break;
            case lookup:
                startTime = CycleTimer::currentSeconds();
                // printf("Thread %d lookup: %d\n", id, instr.second.first);
                hazPtrTable->find(instr.second.first, id);
                hazOpTimes[id*3+2] += CycleTimer::currentSeconds() - startTime;
                hazOpCounts[id*3+2] += 1;
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
    double startTime;
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
                startTime = CycleTimer::currentSeconds();
                lockFreeTable->insert(instr.second.first, instr.second.second);
                memOpTimes[id*3+0] += CycleTimer::currentSeconds() - startTime;
                memOpCounts[id*3+0] += 1;
                break;
            case del:
                startTime = CycleTimer::currentSeconds();
                lockFreeTable->remove(instr.second.first); // Can fail
                memOpTimes[id*3+1] += CycleTimer::currentSeconds() - startTime;
                memOpCounts[id*3+1] += 1;
                // printf("Deleted node: %p\n", res);
                break;
            case lookup:
                startTime = CycleTimer::currentSeconds();
                res = lockFreeTable->find(instr.second.first); // Can fail
                memOpTimes[id*3+2] += CycleTimer::currentSeconds() - startTime;
                memOpCounts[id*3+2] += 1;
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
    double startTime;
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
                startTime = CycleTimer::currentSeconds();
                htable->insert(instr.second.first, instr.second.second);
                fgOpTimes[id*3+0] += CycleTimer::currentSeconds() - startTime;
                fgOpCounts[id*3+0] += 1;
                break;
            case del:
                startTime = CycleTimer::currentSeconds();
                htable->remove(instr.second.first); // Can fail
                fgOpTimes[id*3+1] += CycleTimer::currentSeconds() - startTime;
                fgOpCounts[id*3+1] += 1;
                break;
            case lookup:
                startTime = CycleTimer::currentSeconds();
                htable->find(instr.second.first); // Can fail
                fgOpTimes[id*3+2] += CycleTimer::currentSeconds() - startTime;
                fgOpCounts[id*3+2] += 1;
                break;
            default:
                break;
        }
    }
    pthread_exit(NULL);
}

double seqRun(SeqHashTable<int, int>* htable)
{
    double startTime;
    for (int i = 0; i < input.size(); i++)
    {
        std::pair<Instr, std::pair<int, int> > instr = input[i];
        switch(instr.first)
        {
            case insert:
                startTime = CycleTimer::currentSeconds();
                htable->insert(instr.second.first, instr.second.second);
                seqOpTimes[0] += CycleTimer::currentSeconds() - startTime;
                seqOpCounts[0] += 1;
                break;
            case del:
                startTime = CycleTimer::currentSeconds();
                htable->remove(instr.second.first);
                seqOpTimes[1] += CycleTimer::currentSeconds() - startTime;
                seqOpCounts[1] += 1;
                break;
            case lookup:
                startTime = CycleTimer::currentSeconds();
                htable->find(instr.second.first);
                seqOpTimes[2] += CycleTimer::currentSeconds() - startTime;
                seqOpCounts[2] += 1;
                break;
            default:
                break;
        }
    }
    double dt = CycleTimer::currentSeconds() - startTime;
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
    for (uint i = 0; i < testfiles.size(); i++) {
        for (int load_fac = 1; load_fac <= 10; load_fac = load_fac == 1 ? 5 : load_fac + 5) {
            int bucketSize;
            if (i == 0) bucketSize = 200000/load_fac;
            if (i == 1) bucketSize = 50000/load_fac;
            if (i == 2) bucketSize = 10000/load_fac;
        // for (int load_fac = 1; load_fac <= 20; load_fac = load_fac == 1 ? 5 : load_fac + 5) {

            parseText(testfiles[i].c_str());
            baseline = new SeqHashTable<int, int>(bucketSize, &hash);
            seqOpTimes = std::vector<double>(3, 0.0);
            seqOpCounts = std::vector<double>(3, 0);
            baseTime = seqRun(baseline);
            double insertTime = (getSum(0, seqOpTimes) *1000000.f) / getSum(0, seqOpCounts);
            double deleteTime = (getSum(1, seqOpTimes) *1000000.f) / getSum(1, seqOpCounts);
            double lookupTime = (getSum(2, seqOpTimes) *1000000.f) / getSum(2, seqOpCounts);
            printf("\nAvg CPU time with load factor %d: %s on sequential hash table\n", load_fac, testfiles[i].c_str());
            printf("Average insert time: %f microseconds\n", insertTime);
            printf("Average delete time: %f microseconds\n", deleteTime);
            printf("Average lookup time: %f microseconds\n", lookupTime);

            printf("\nAvg CPU time with load factor %d: %s on fine-grained lock-based hash table\n", load_fac, testfiles[i].c_str());
            for (uint j = 1; j <= MAX_THREADS; j *= 2)
            {
                fgOpTimes = std::vector<double>(j*3, 0.0);
                fgOpCounts = std::vector<double>(j*3, 0);
                htable = new FgHashTable<int, int>(bucketSize, &hash);
                numThreads = j;
                for (uint id = 0; id < j; id++)
                {
                    pthread_create(&threads[id], NULL, fgRun, &ids[id]);
                }
                for (uint id = 0; id < j; id++)
                {
                    pthread_join(threads[id], NULL);
                }
                delete(htable);
                double insertTime = (getSum(0, fgOpTimes) *1000000.f) / getSum(0, fgOpCounts);
                double deleteTime = (getSum(1, fgOpTimes) *1000000.f) / getSum(1, fgOpCounts);
                double lookupTime = (getSum(2, fgOpTimes) *1000000.f) / getSum(2, fgOpCounts);
                printf("%d Thread Fine-Grained Test\n", numThreads);
                printf("Average insert time: %f microseconds\n", insertTime);
                printf("Average delete time: %f microseconds\n", deleteTime);
                printf("Average lookup time: %f microseconds\n", lookupTime);
            }
            printf("\nAvg CPU time with load factor %d: %s on lock-free hash table with memory leaks\n", load_fac, testfiles[i].c_str());
            for (uint j = 1; j <= MAX_THREADS; j *= 2)
            {
                memOpTimes = std::vector<double>(j*3, 0.0);
                memOpCounts = std::vector<double>(j*3, 0);
                lockFreeTable = new MemLeakHashTable<int, int>(bucketSize, &hash);
                numThreads = j;
                for (uint id = 0; id < j; id++)
                {
                    pthread_create(&threads[id], NULL, lockFreeRun, &ids[id]);
                }
                for (uint id = 0; id < j; id++)
                {
                    pthread_join(threads[id], NULL);
                }
                delete(lockFreeTable);
                double insertTime = (getSum(0, memOpTimes) *1000000.f) / getSum(0, memOpCounts);
                double deleteTime = (getSum(1, memOpTimes) *1000000.f) / getSum(1, memOpCounts);
                double lookupTime = (getSum(2, memOpTimes) *1000000.f) / getSum(2, memOpCounts);
                printf("%d Thread Lock-Free Test with Memory Leaks\n", numThreads);
                printf("Average insert time: %f microseconds\n", insertTime);
                printf("Average delete time: %f microseconds\n", deleteTime);
                printf("Average lookup time: %f microseconds\n", lookupTime);
            }
            printf("\nAvg CPU time with load factor %d: %s on lock-free hash table with hazard pointers\n", load_fac, testfiles[i].c_str());
            for (uint j = 1; j <= 64; j *= 2)
            {
                hazOpTimes = std::vector<double>(j*3, 0.0);
                hazOpCounts = std::vector<double>(j*3, 0);
                cds::Initialize();
                {
                    cds::gc::HP hpGC(MAX_THREADS*3, 50);
                    cds::threading::Manager::attachThread();

                    hazPtrTable = new HazPtrHashTable<int, int>(bucketSize, &hash);
                    numThreads = j;
                    for (uint id = 0; id < j; id++)
                    {
                        pthread_create(&threads[id], NULL, hazPtrRun, &ids[id]);
                    }
                    for (uint id = 0; id < j; id++)
                    {
                        pthread_join(threads[id], NULL);
                    }
                    delete(hazPtrTable);
                }
                cds::Terminate();
                double insertTime = (getSum(0, hazOpTimes) *1000000.f) / getSum(0, hazOpCounts);
                double deleteTime = (getSum(1, hazOpTimes) *1000000.f) / getSum(1, hazOpCounts);
                double lookupTime = (getSum(2, hazOpTimes) *1000000.f) / getSum(2, hazOpCounts);
                printf("%d Thread Lock-Free Test with Hazard Pointers\n", numThreads);
                printf("Average insert time: %f microseconds\n", insertTime);
                printf("Average delete time: %f microseconds\n", deleteTime);
                printf("Average lookup time: %f microseconds\n", lookupTime);
            }
            delete(baseline);
        }
    }
}