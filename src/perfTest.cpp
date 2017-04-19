#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <string>
#include <cassert>
#include <pthread.h>

#include "tools/cycle_timer.h"
#include "seq_hash_table.h"
#include "fg_hash_table.h"
#include "del_opt_hash_table.h"

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
DelOptHashTable<int, int>* delOptTable;

const char *args[] = {"tests/uniform_all_test.txt",
                      "tests/chunked_all.txt",
                      "tests/30p_del_all.txt",
                      "tests/25p_del_all.txt",
                      "tests/20p_del_all.txt",
                      "tests/15p_del_all.txt",
                      "tests/10p_del_all.txt"};
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

void* delOptRun(void* arg) {
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
                delOptTable->insert(instr.second.first, instr.second.second);
                break;
            case del:
                res = delOptTable->remove(instr.second.first); // Can fail
                break;
            case lookup:
                res = delOptTable->find(instr.second.first); // Can fail
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
        LLNode<int, int>* res;
        switch(instr.first)
        {
            case insert:
                htable->insert(instr.second.first, instr.second.second);
                break;
            case del:
                res = htable->remove(instr.second.first); // Can fail
                break;
            case lookup:
                res = htable->find(instr.second.first); // Can fail
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
        LLNode<int, int>* res;
        switch(instr.first)
        {
            case insert:
                htable->insert(instr.second.first, instr.second.second);
                break;
            case del:
                res = htable->remove(instr.second.first);
                break;
            case lookup:
                res = htable->find(instr.second.first);
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

    pthread_t threads[16];
    int ids[16];
    for (uint z = 0; z < 16; z++)
    {
        ids[z] = z;
    }
    double baseTime;
    for (uint i = 0; i < testfiles.size(); i++) {
        printf("\nPerformance Testing file: %s\n on fine-grained lock-based hash table", testfiles[i].c_str());
        parseText(testfiles[i].c_str());
        baseline = new SeqHashTable<int, int>(input.size() / 1000, &hash);
        baseTime = seqRun(baseline);
        for (uint j = 1; j <= 16; j *= 2)
        {
            htable = new FgHashTable<int, int>(input.size() / 1000, &hash);
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
            printf("%d Thread Fine-Grained Test completed in %f ms!\n", numThreads, (1000.f * dt));
            printf("%d Thread Speedup: %f\n", j, (baseTime / dt));
            delete(htable);
        }
        delete(baseline);
    }
    for (uint i = 0; i < testfiles.size(); i++) {
        printf("\nPerformance Testing file: %s\n on delete-optimal lock-free hash table", testfiles[i].c_str());
        for (uint j = 1; j <= 16; j *= 2)
        {
            delOptTable = new DelOptHashTable<int, int>(input.size() / 1000, &hash);
            numThreads = j;
            double startTime = CycleTimer::currentSeconds();
            for (uint id = 0; id < j; id++)
            {
                pthread_create(&threads[id], NULL, delOptRun, &ids[id]);
            }
            for (uint id = 0; id < j; id++)
            {
                pthread_join(threads[id], NULL);
            }
            double dt = CycleTimer::currentSeconds() - startTime;
            printf("%d Thread Delete-Optimal Lock-Free Test completed in %f ms!\n", numThreads, (1000.f * dt));
            printf("%d Thread Speedup: %f\n", j, (baseTime / dt));
            delete(delOptTable);
        }
    }
}