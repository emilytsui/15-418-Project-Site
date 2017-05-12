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

const char *args1[] = {"tests/correctness1.txt",
                      "tests/correctness2.txt"};
std::vector<std::string> corrtestfiles(args1, args1 + sizeof(args1)/sizeof(args1[0]));

const char *args2[] = {"tests/uniform_all_test.txt",
                      "tests/chunked_all.txt",
                      "tests/30p_del_all.txt",
                      "tests/25p_del_all.txt",
                      "tests/20p_del_all.txt",
                      "tests/15p_del_all.txt",
                      "tests/10p_del_all.txt",
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
                dcasTable->insert(instr.second.first, instr.second.second);
                break;
            case del:
                // printf("Thread %d delete: %d\n", id, instr.second.first);
                dcasTable->remove(instr.second.first);
                break;
            case lookup:
                // printf("Thread %d lookup: %d\n", id, instr.second.first);
                dcasTable->find(instr.second.first);
                break;
            default:
                break;
        }
    }
    // double dt = (std::clock() - startTime) / (double) CLOCKS_PER_SEC;
    // printf("Before thread return %f\n", dt * 1000.f);
    // pthread_exit(NULL);
}

double seqRun(SeqHashTable<int, int>* htable)
{
    auto startTime = std::chrono::high_resolution_clock::now();
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
    std::chrono::duration<double, std::milli> fp_ms = (std::chrono::high_resolution_clock::now() - startTime);
    double dt = fp_ms.count();
    printf("Sequential Test complete in %f ms!\n", dt);
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
    for (uint i = 0; i < corrtestfiles.size(); i++) {
        printf("Correctness Testing file: %s for DCAS lock-free hash table\n", corrtestfiles[i].c_str());
        parseText(corrtestfiles[i].c_str());
        baseline = new SeqHashTable<int, int>(10000, &hash);
        seqRun(baseline);
        for (uint j = 1; j <= 16; j *= 2)
        {
            dcasTable = new DCASHashTable<int, int>(10000, &hash);
            numThreads = j;
            for (uint id = 0; id < j; id++)
            {
                pthread_create(&threads[id], NULL, dcasRun, &ids[id]);
            }
            for (uint id = 0; id < j; id++)
            {
                pthread_join(threads[id], NULL);
            }
            testDCASCorrectness(baseline, dcasTable);
            delete(dcasTable);
        }
        delete(baseline);
    }
    printf("Correctness Tests Complete!\n");

    double baseTime;
    for (uint i = 0; i < perftestfiles.size(); i++) {
        printf("\nPerformance Testing file: %s on DCAS lock-free hash table\n", perftestfiles[i].c_str());
        parseText(perftestfiles[i].c_str());
        baseline = new SeqHashTable<int, int>(10000, &hash);
        baseTime = seqRun(baseline);
        for (uint j = 1; j <= 64; j *= 2)
        {
            double bestTime = std::numeric_limits<double>::max();
            for (int i = 0; i < 5; i++)
            {
                dcasTable = new DCASHashTable<int, int>(10000, &hash);
                numThreads = j;
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
        }
        delete(baseline);
    }

    std::string filename = "tests/load_factor_test.txt";
    for (int load_fac = 1; load_fac <= 20; load_fac = load_fac == 1 ? 5 : load_fac + 5) {
        printf("\nPerformance Testing file with load factor %d: %s on fine-grained lock-based hash table\n", load_fac, filename.c_str());
        parseText(filename.c_str());
        baseline = new SeqHashTable<int, int>(200000/load_fac, &hash);
        baseTime = seqRun(baseline);
        for (uint j = 1; j <= 64; j *= 2)
        {
            double bestTime = std::numeric_limits<double>::max();
            for (int i = 0; i < 5; i++)
            {
                dcasTable = new DCASHashTable<int, int>(10000, &hash);
                numThreads = j;
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
        }
        delete(baseline);
    }

    for (int load_fac = 250; load_fac <= 1000; load_fac *= 2) {
        printf("\nPerformance Testing file with load factor %d: %s on fine-grained lock-based hash table\n", load_fac, filename.c_str());
        parseText(filename.c_str());
        baseline = new SeqHashTable<int, int>(200000/load_fac, &hash);
        baseTime = seqRun(baseline);
        for (uint j = 1; j <= 64; j *= 2)
        {
            double bestTime = std::numeric_limits<double>::max();
            for (int i = 0; i < 5; i++)
            {
                dcasTable = new DCASHashTable<int, int>(10000, &hash);
                numThreads = j;
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
        }
        delete(baseline);
    }
}