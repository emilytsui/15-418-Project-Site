#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <string>
#include <cassert>

#include "tools/cycle_timer.h"
#include "seq_hash_table.h"

#define ALL_TESTS

const char *args[] = {"tests/uniform_all_test.txt",
                      "tests/uniform_test_InsDel.txt",
                      "tests/chunked_test_InsDel.txt",
                      "tests/50p_del_test_InsDel.txt",
                      "tests/25p_del_test_InsDel.txt",
                      "tests/10p_del_test_InsDel.txt"};
std::vector<std::string> testfiles(args, args + sizeof(args)/sizeof(args[0]));

enum Instr {
    insert,
    del,
    lookup
};

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

void seqTest(std::vector<std::pair<Instr, std::pair<int, int> > > input)
{
    SeqHashTable<int,int> table(input.size() / 2, &hash);
    double startTime = CycleTimer::currentSeconds();
    for (int i = 0; i < input.size(); i++)
    {
        std::pair<Instr, std::pair<int, int> > instr = input[i];
        switch(instr.first)
        {
            case insert:
                table.insert(instr.second.first, instr.second.second);
                break;
            case del:
                assert(table.remove(instr.second.first)->get_data() == instr.second.second);
                break;
            case lookup:
                assert(table.find(instr.second.first)->get_data() == instr.second.second);
                break;
            default:
                break;
        }
    }
    double dt = CycleTimer::currentSeconds() - startTime;
    printf("Test complete in %f ms! (No Errors Detected)\n", (1000.f * dt));
}

int main() {

    for (uint i = 0; i < testfiles.size(); i++) {
        printf("Testing file: %s\n", testfiles[i].c_str());
        std::vector<std::pair<Instr, std::pair<int, int> > > input = parseText(testfiles[i]);
        seqTest(input);
    }

    // coarseLockTest(input);
    // fineLockTest(input);
    // insertOptimalTest(input);
    // deleteOptimalTest(input);
    // dynamicTest(input);
}