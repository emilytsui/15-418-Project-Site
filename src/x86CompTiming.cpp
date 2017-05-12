#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <string>
#include <cassert>
#include <pthread.h>
#include <ctime>
#include <chrono>

#include "tools/cycle_timer.h"
#include "seq_hash_table.h"
#include "dcas_hash_table.h"

enum Instr {
    insert,
    del,
    lookup
};

static int numThreads;
static std::vector<std::pair<Instr, std::pair<int, int> > > input;
SeqHashTable<int, int>* baseline;
DCASHashTable<int, int>* dcasTable;
static std::vector<double> seqTimes;
static std::vector<double> multiTimes;
static std::vector<double> seqInstr;
static std::vector<double> multiInstr;

const char *args2[] = {
                      // "tests/uniform_all_test.txt",
                      // "tests/chunked_all.txt",
                      // "tests/30p_del_all.txt",
                      // "tests/25p_del_all.txt",
                      // "tests/20p_del_all.txt",
                      // "tests/15p_del_all.txt",
                      // "tests/10p_del_all.txt",
                      "tests/5p_del_5p_ins.txt",
                      "tests/10p_del_10p_ins.txt"};
std::vector<std::string> perftestfiles(args2, args2 + sizeof(args2)/sizeof(args2[0]));

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

void* dcasRun(void *arg) {
    // printf("In delete Optimal\n");
    // double startTime = std::clock();
    double insTime = 0;
    double delTime = 0;
    double lookupTime = 0;
    double numInserts = 0;
    double numDeletes = 0;
    double numLookups = 0;
    int id = *(int*)arg;
    int instrPerThread = input.size() / numThreads;
    int start = instrPerThread * id;
    int end = (start + instrPerThread < input.size()) ? (start + instrPerThread) : input.size();
    for (int i = start; i < end; i++)
    {
        std::pair<Instr, std::pair<int, int> > instr = input[i];
        std::chrono::duration<double, std::micro> t_ms;
        std::chrono::high_resolution_clock::time_point t1;
        switch(instr.first)
        {
            case insert:
                // printf("Thread %d insert: %d\n", id, instr.second.first);
                t1 = std::chrono::high_resolution_clock::now();
                dcasTable->insert(instr.second.first, instr.second.second);
                t_ms = (std::chrono::high_resolution_clock::now() - t1);
                insTime += t_ms.count();
                numInserts++;
                break;
            case del:
                // printf("Thread %d delete: %d\n", id, instr.second.first);
                t1 = std::chrono::high_resolution_clock::now();
                dcasTable->remove(instr.second.first);
                t_ms = (std::chrono::high_resolution_clock::now() - t1);
                delTime += t_ms.count();
                numDeletes++;
                break;
            case lookup:
                // printf("Thread %d lookup: %d\n", id, instr.second.first);
                t1 = std::chrono::high_resolution_clock::now();
                dcasTable->find(instr.second.first);
                t_ms = (std::chrono::high_resolution_clock::now() - t1);
                lookupTime += t_ms.count();
                numLookups++;
                break;
            default:
                break;
        }
    }
    // double dt = (std::clock() - startTime) / (double) CLOCKS_PER_SEC;
    // printf("Before thread return %f\n", dt * 1000.f);
    // pthread_exit(NULL);
    multiTimes[id*3] = insTime;
    multiTimes[id*3 + 1] = lookupTime;
    multiTimes[id*3 + 2] = delTime;
    multiInstr[id*3] = numInserts;
    multiInstr[id*3 + 1] = numLookups;
    multiInstr[id*3 + 2] = numDeletes;
}

double seqRun(SeqHashTable<int, int>* htable)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    double insTime = 0;
    double delTime = 0;
    double lookupTime = 0;
    double numInserts = 0;
    double numDeletes = 0;
    double numLookups = 0;
    for (int i = 0; i < input.size(); i++)
    {
        std::pair<Instr, std::pair<int, int> > instr = input[i];
        std::chrono::high_resolution_clock::time_point t1;
        std::chrono::duration<double, std::micro> t_ms;
        switch(instr.first)
        {
            case insert:
                t1 = std::chrono::high_resolution_clock::now();
                htable->insert(instr.second.first, instr.second.second);
                t_ms = (std::chrono::high_resolution_clock::now() - t1);
                insTime += t_ms.count();
                numInserts++;
                break;
            case del:
                t1 = std::chrono::high_resolution_clock::now();
                htable->remove(instr.second.first);
                t_ms = (std::chrono::high_resolution_clock::now() - t1);
                delTime += t_ms.count();
                numDeletes++;
                break;
            case lookup:
                t1 = std::chrono::high_resolution_clock::now();
                htable->find(instr.second.first);
                t_ms = (std::chrono::high_resolution_clock::now() - t1);
                lookupTime += t_ms.count();
                numLookups++;
                break;
            default:
                break;
        }
    }
    std::chrono::duration<double, std::milli> fp_ms = (std::chrono::high_resolution_clock::now() - startTime);
    double dt = fp_ms.count();
    printf("Sequential Test complete in %f ms!\n", dt);
    seqTimes[0] = insTime;
    seqTimes[1] = lookupTime;
    seqTimes[2] = delTime;
    seqInstr[0] = numInserts;
    seqInstr[1] = numLookups;
    seqInstr[2] = numDeletes;
    return dt;
}

void testDCASCorrectness(SeqHashTable<int, int>* baseline, DCASHashTable<int, int>* htable) {
    // printf("In del optimal correctness check\n");
    for (int i = 0; i < baseline->table_size; i++)
    {
        LLNode<int, int>* curr = baseline->table[i];
        while(curr != NULL)
        {
            DNode<int, int>* res = htable->find(curr->get_key());
            if(res == NULL || res->get_data() != curr->get_data())
            {
                printf("Incorrect: Lock-free Hash Table doesn't contain (%d, %d)\n", curr->get_key(), curr->get_data());
            }
            curr = curr->get_next();
        }
    }
    for (int j = 0; j < htable->table_size; j++)
    {
        DNode<int, int>* curr = htable->table[j]->get_next();
        int count = 0;
        while(curr != NULL)
        {
            LLNode<int, int>* res = baseline->find(curr->get_key());
            if(res == NULL || res->get_data() != curr->get_data())
            {
                printf("Incorrect: Lock-free Hash Table contains additional elem (%d, %d)\n", curr->get_key(), curr->get_data());
                printf("Element: %d in chain %d\n", count, j);
            }
            curr = curr->get_next();
            count++;
        }
    }
    // printf("Completed delete optimal correctness\n");
}

int main() {

    pthread_t threads[64];
    int ids[64];
    for (uint z = 0; z < 64; z++)
    {
        ids[z] = z;
    }

    double baseTime;
    seqTimes.resize(3);
    seqInstr.resize(3);
    for (uint i = 0; i < perftestfiles.size(); i++) {
        printf("\nPerformance Testing file: %s on DCAS lock-free hash table\n", perftestfiles[i].c_str());
        parseText(perftestfiles[i].c_str());
        baseline = new SeqHashTable<int, int>(10000, &hash);
        baseTime = seqRun(baseline);
        printf("Seq: Average Insert Time %f\n", seqTimes[0] / seqInstr[0]);
        printf("Seq: Average Lookup Time %f\n", seqTimes[1] / seqInstr[1]);
        printf("Seq: Average Delete Time %f\n", seqTimes[2] / seqInstr[2]);
        printf("Seq: Average Operation Time %f\n", (seqTimes[0] + seqTimes[1] + seqTimes[2]) / (seqInstr[0] + seqInstr[1] + seqInstr[2]));
        for (uint j = 1; j <= 64; j *= 2)
        {
            double bestTime = std::numeric_limits<double>::max();
            for (int i = 0; i < 5; i++)
            {
                dcasTable = new DCASHashTable<int, int>(10000, &hash);
                numThreads = j;
                multiTimes.resize(3*numThreads);
                multiInstr.resize(3*numThreads);
                auto startTime = std::chrono::high_resolution_clock::now();
                // printf("Start time: %f\n", startTime);
                for (uint id = 0; id < j; id++)
                {
                    pthread_create(&threads[id], NULL, dcasRun, &ids[id]);
                }
                for (uint id = 0; id < j; id++)
                {
                    pthread_join(threads[id], NULL);
                }
                std::chrono::duration<double, std::milli> fp_ms = (std::chrono::high_resolution_clock::now() - startTime);
                double dt = fp_ms.count();
                if (dt < bestTime)
                {
                    bestTime = dt;
                }
                delete(dcasTable);
            }
            printf("%d Thread DCAS lock-free Test completed in %f ms!\n", numThreads, bestTime);
            printf("%d Thread Speedup: %f\n", j, (baseTime / bestTime));
            double insertTime = 0;
            double deleteTime = 0; 
            double lookupTime = 0;
            double totalIns = 0;
            double totalDel = 0;
            double totalLookup = 0;
            for (int k = 0; k < numThreads; k++)
            {
                insertTime += multiTimes[k*3];
                totalIns += multiInstr[k*3];
                lookupTime += multiTimes[k*3 + 1];
                totalLookup += multiInstr[k*3 + 1];
                deleteTime += multiTimes[k*3 + 2];
                totalDel += multiInstr[k*3 + 2];
            }
            printf("%d Thread: Average Insert Time %f\n", j, insertTime / totalIns);
            printf("%d Thread: Average Lookup Time %f\n", j, lookupTime / totalLookup);
            printf("%d Thread: Average Delete Time %f\n", j, deleteTime / totalDel);
            printf("%d Thread: Average Operation Time %f\n", j, (insertTime + lookupTime + deleteTime) / (totalIns + totalLookup + totalDel));
        }
        delete(baseline);
    }

    std::string filename = "tests/load_factor_test.txt";
    for (int load_fac = 1; load_fac <= 20; load_fac = load_fac == 1 ? 5 : load_fac + 5) {
        printf("\nPerformance Testing file with load factor %d: %s on fine-grained lock-based hash table\n", load_fac, filename.c_str());
        parseText(filename.c_str());
        baseline = new SeqHashTable<int, int>(200000/load_fac, &hash);
        baseTime = seqRun(baseline);
        printf("Seq: Average Insert Time %f\n", seqTimes[0] / seqInstr[0]);
        printf("Seq: Average Lookup Time %f\n", seqTimes[1] / seqInstr[1]);
        printf("Seq: Average Delete Time %f\n", seqTimes[2] / seqInstr[2]);
        printf("Seq: Average Operation Time %f\n", (seqTimes[0] + seqTimes[1] + seqTimes[2]) / (seqInstr[0] + seqInstr[1] + seqInstr[2]));
        for (uint j = 1; j <= 64; j *= 2)
        {
            double bestTime = std::numeric_limits<double>::max();
            for (int i = 0; i < 5; i++)
            {
                dcasTable = new DCASHashTable<int, int>(10000, &hash);
                numThreads = j;
                multiTimes.resize(3*numThreads);
                multiInstr.resize(3*numThreads);
                auto startTime = std::chrono::high_resolution_clock::now();
                // printf("Start time: %f\n", startTime);
                for (uint id = 0; id < j; id++)
                {
                    pthread_create(&threads[id], NULL, dcasRun, &ids[id]);
                }
                for (uint id = 0; id < j; id++)
                {
                    pthread_join(threads[id], NULL);
                }
                std::chrono::duration<double, std::milli> fp_ms = (std::chrono::high_resolution_clock::now() - startTime);
                double dt = fp_ms.count();
                if (dt < bestTime)
                {
                    bestTime = dt;
                }
                delete(dcasTable);
            }
            printf("%d Thread DCAS lock-free Test completed in %f ms!\n", numThreads, bestTime);
            printf("%d Thread Speedup: %f\n", j, (baseTime / bestTime));
            double insertTime = 0;
            double deleteTime = 0; 
            double lookupTime = 0;
            double totalIns = 0;
            double totalDel = 0;
            double totalLookup = 0;
            for (int k = 0; k < numThreads; k++)
            {
                insertTime += multiTimes[k*3];
                totalIns += multiInstr[k*3];
                lookupTime += multiTimes[k*3 + 1];
                totalLookup += multiInstr[k*3 + 1];
                deleteTime += multiTimes[k*3 + 2];
                totalDel += multiInstr[k*3 + 2];
            }
            printf("%d Thread: Average Insert Time %f\n", j, insertTime / totalIns);
            printf("%d Thread: Average Lookup Time %f\n", j, lookupTime / totalLookup);
            printf("%d Thread: Average Delete Time %f\n", j, deleteTime / totalDel);
            printf("%d Thread: Average Operation Time %f\n", j, (insertTime + lookupTime + deleteTime) / (totalIns + totalLookup + totalDel));
        }
        delete(baseline);
    }

    // for (int load_fac = 250; load_fac <= 1000; load_fac *= 2) {
    //     printf("\nPerformance Testing file with load factor %d: %s on fine-grained lock-based hash table\n", load_fac, filename.c_str());
    //     parseText(filename.c_str());
    //     baseline = new SeqHashTable<int, int>(200000/load_fac, &hash);
    //     baseTime = seqRun(baseline);
    //     for (uint j = 1; j <= 64; j *= 2)
    //     {
    //         double bestTime = std::numeric_limits<double>::max();
    //         for (int i = 0; i < 5; i++)
    //         {
    //             dcasTable = new DCASHashTable<int, int>(10000, &hash);
    //             numThreads = j;
    //             auto startTime = std::chrono::high_resolution_clock::now();
    //             // printf("Start time: %f\n", startTime);
    //             for (uint id = 0; id < j; id++)
    //             {
    //                 pthread_create(&threads[id], NULL, dcasRun, &ids[id]);
    //             }
    //             for (uint id = 0; id < j; id++)
    //             {
    //                 pthread_join(threads[id], NULL);
    //             }
    //             std::chrono::duration<double, std::milli> fp_ms = (std::chrono::high_resolution_clock::now() - startTime);
    //             double dt = fp_ms.count();
    //             if (dt < bestTime)
    //             {
    //                 bestTime = dt;
    //             }
    //             delete(dcasTable);
    //         }
    //         printf("%d Thread DCAS lock-free Test completed in %f ms!\n", numThreads, bestTime);
    //         printf("%d Thread Speedup: %f\n", j, (baseTime / bestTime));
    //     }
    //     delete(baseline);
    // }
}