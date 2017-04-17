#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <string>
#include <cassert>
#include <pthread.h>

#include "tools/cycle_timer.h"
#include "seq_hash_table.h"
#include "fg_hash_table.h"

enum Instr {
    insert,
    del,
    lookup
};

static int numThreads;
static std::vector<std::pair<Instr, std::pair<int, int> > > input;

const char *args[] = {"tests/correctness1.txt",
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

std::vector<std::pair<Instr, std::pair<int, int> > > parseText(const std::string &filename)
{
    std::ifstream infile;
    infile.open(filename.c_str());
    std::vector<std::pair<Instr, std::pair<int, int> > > instructions;
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
            instructions.push_back(task);
        }
    }
    return instructions;
}

void* fgRun(void *arg)
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

void seqRun(SeqHashTable<int, int>* htable)
{
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
}

void testFgCorrectness(SeqHashTable<int, int>* baseline, FgHashTable<int, int>* htable)
{
    for (int i = 0; i < baseline->table_size; i++)
    {
        LLNode<int, int>* curr = baseline->table[i];
        while(curr != NULL)
        {
            LLNode<int, int>* res = htable->find(curr->get_key());
            if(res == NULL || res->get_data() != curr->get_data())
            {
                printf("Incorrect: Concurrent Hash Table doesn't contain (%d, %d)\n", curr->get_key(), curr->get_data());
            }
        }
    }
    for (int j = 0; j < htable->table_size; j++)
    {
        LLNode<int, int>* curr = htable->table[j];
        while(curr != NULL)
        {
            LLNode<int, int>* res = baseline->find(curr->get_key());
            if(res == NULL || res->get_data() != curr->get_data())
            {
                printf("Incorrect: Concurrent Hash Table contains additional elem (%d, %d)\n", res->get_key(), res->get_data());
            }
        }
    }
}

int main() {

    pthread_t threads[16];
    for (uint i = 0; i < testfiles.size(); i++) {
        printf("Correctness Testing file: %s\n", testfiles[i].c_str());
        input = parseText(testfiles[i]);
        SeqHashTable<int, int>* baseline = new SeqHashTable<int, int>(input.size() / 2, &hash);
        seqRun(baseline);
        for (uint j = 1; j <= 16; j *= 2)
        {
            FgHashTable<int, int>* htable = new FgHashTable<int, int>(input.size() / 2, &hash);
            numThreads = j;
            for (uint id = 0; id < j; id++)
            {
                pthread_create(&threads[id], NULL, fgRun, (void*)htable);
            }
            for (uint id = 0; id < j; id++)
            {
                pthread_join(threads[id], NULL);
            }
            testFgCorrectness(baseline, htable);
            delete(htable);
        }
        delete(baseline);
    }
}