#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <string>
#include <cassert>
#include <pthread.h>

#include "tools/cycle_timer.h"
#include "seq_hash_table.h"
#include "haz_ptr_hash_table.h"
#include "tools/cds-2.2.0/build/include/cds/init.h"
#include "tools/cds-2.2.0/build/include/cds/gc/hp.h"

enum Instr {
    insert,
    del,
    lookup
};

static int numThreads;
static std::vector<std::pair<Instr, std::pair<int, int> > > input;
SeqHashTable<int, int>* baseline;
HazPtrHashTable<int, int>* htable;

const char *args[] = {"tests/correctness-small.txt",
                      "tests/correctness1.txt",
                      "tests/correctness2.txt"};
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
                htable->insert(instr.second.first, instr.second.second, id);
                break;
            case del:
                // printf("Thread %d delete: %d\n", id, instr.second.first);
                htable->remove(instr.second.first, id);
                break;
            case lookup:
                // printf("Thread %d lookup: %d\n", id, instr.second.first);
                htable->find(instr.second.first, id);
                break;
            default:
                break;
        }
    }
    // Detach thread when terminating
    cds::threading::Manager::detachThread();
    pthread_exit(NULL);
}

void seqRun(SeqHashTable<int, int>* htable)
{
    for (int i = 0; i < input.size(); i++)
    {
        std::pair<Instr, std::pair<int, int> > instr = input[i];
        switch(instr.first)
        {
            case insert:
                htable->insert(instr.second.first, instr.second.second); // Can Fail
                break;
            case del:
                htable->remove(instr.second.first); // Can Fail
                break;
            case lookup:
                htable->find(instr.second.first); // Can Fail
                break;
            default:
                break;
        }
    }
}

void testHazPtrCorrectness(SeqHashTable<int, int>* baseline, HazPtrHashTable<int, int>* htable) {
    for (int i = 0; i < baseline->table_size; i++)
    {
        LLNode<int, int>* curr = baseline->table[i];
        while(curr != NULL)
        {
            HPNode<int, int>* res = htable->find(curr->get_key(), 0);
            if(res == NULL || res->get_data() != curr->get_data())
            {
                printf("Incorrect: Lock-free Hash Table doesn't contain (%d, %d)\n", curr->get_key(), curr->get_data());
            }
            curr = curr->get_next();
        }
    }
    for (int j = 0; j < htable->table_size; j++)
    {
        HPNode<int, int>* curr = htable->table[j]->get_next();
        // printf("Next pointer: %p\n", curr);
        while(curr != NULL)
        {
            LLNode<int, int>* res = baseline->find(curr->get_key());
            // printf("%p\n", curr);
            if(res == NULL || res->get_data() != curr->get_data())
            {
                printf("Incorrect: Lock-free Hash Table contains additional elem (%d, %d)\n", curr->get_key(), curr->get_data());
            }
            curr = curr->get_next();
        }
    }
}

int main() {

    pthread_t threads[16];
    int ids[16];
    for (uint z = 0; z < 16; z++)
    {
        ids[z] = z;
    }
    for (uint i = 0; i < testfiles.size(); i++) {
        parseText(testfiles[i].c_str());
        baseline = new SeqHashTable<int, int>(10000, &hash);
        seqRun(baseline);
        printf("Correctness Testing file: %s for hazard pointer lock-free hash table\n", testfiles[i].c_str());
        for (uint j = 1; j <= 16; j *= 2)
        {
            cds::Initialize();
            {
                cds::gc::HP hpGC(48, 16);
                cds::threading::Manager::attachThread();
                htable = new HazPtrHashTable<int, int>(10000, &hash);
                numThreads = j;
                for (uint id = 0; id < j; id++)
                {
                    pthread_create(&threads[id], NULL, hazPtrRun, &ids[id]);
                }
                for (uint id = 0; id < j; id++)
                {
                    pthread_join(threads[id], NULL);
                }
                testHazPtrCorrectness(baseline, htable);
                delete(htable);
            }
            cds::Terminate();
        }
        delete(baseline);
    }
    printf("Correctness Tests Complete!\n");
}