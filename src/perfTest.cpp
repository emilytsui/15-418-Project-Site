#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <string>
#include <cassert>
#include <pthread.h>

#include "tools/cycle_timer.h"
#include "seq_hash_table.h"
#include "fg_hash_table.h"

#define ALL_TESTS

enum Instr {
    insert,
    del,
    lookup
};

static int numThreads;
static std::vector<std::pair<Instr, std::pair<int, int> > > input;

const char *args[] = {"tests/uniform_all_test.txt",
                      "tests/uniform_test_InsDel.txt",
                      "tests/chunked_test_InsDel.txt",
                      "tests/50p_del_test_InsDel.txt",
                      "tests/25p_del_test_InsDel.txt",
                      "tests/10p_del_test_InsDel.txt"};
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

void* fgRun(void* arg)
{
    FgHashTable<int, int>* htable = (FgHashTable<int, int>*) arg;
    int instrPerThread = input.size() / numThreads;
    int start = instrPerThread * pthread_self();
    int end = start + instrPerThread;
    for (int i = start; i < end; i++)
    {
        std::pair<Instr, std::pair<int, int> > instr = input[i];
        switch(instr.first)
        {
            case insert:
                htable->insert(instr.second.first, instr.second.second);
                break;
            case del:
                assert(htable->remove(instr.second.first)->get_data() == instr.second.second);
                break;
            case lookup:
                assert(htable->find(instr.second.first)->get_data() == instr.second.second);
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
                assert(htable->remove(instr.second.first)->get_data() == instr.second.second);
                break;
            case lookup:
                assert(htable->find(instr.second.first)->get_data() == instr.second.second);
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
    for (uint i = 0; i < testfiles.size(); i++) {
        printf("Performance Testing file: %s\n", testfiles[i].c_str());
        SeqHashTable<int, int>* baseline = new SeqHashTable<int, int>(input.size() / 2, &hash);
        int baseTime = seqRun(baseline);
        for (uint j = 1; j <= 16; j *= 2)
        {
            FgHashTable<int, int>* htable = new FgHashTable<int, int>(input.size() / 2, &hash);
            numThreads = j;
            double startTime = CycleTimer::currentSeconds();
            for (uint id = 0; id < j; id++)
            {
                pthread_create(&threads[id], NULL, fgRun, (void*)htable);
            }
            for (uint id = 0; id < j; id++)
            {
                pthread_join(threads[id], NULL);
            }
            double dt = CycleTimer::currentSeconds() - startTime;
            printf("Fine-Grain Test complete in %f ms!\n", (1000.f * dt));
            printf("%d Thread Speedup: %d\n", j, (baseTime / dt));
            delete(htable);
        }
        delete(baseline);
    }
}