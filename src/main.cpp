#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <string>
#include <cassert>

#include "seq_hash_table.h"

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
  printf("Test Complete! (No Errors Detected)\n");
}

int main() {
    std::vector<std::pair<Instr, std::pair<int, int> > > input = parseText("tests/uniform_all_test.txt");
    seqTest(input);
    // coarseLockTest(input);
    // fineLockTest(input);
    // insertOptimalTest(input);
    // deleteOptimalTest(input);
    // dynamicTest(input);
}