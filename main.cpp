#include "random-forest.h"
#include "data-reader.h"

#include <cstdio>
#include <string>
#include <unordered_map>

using ArgsTable = std::unordered_map<std::string, std::string>;

void TableValToInt(const ArgsTable &table, const std::string &key, int &val) {
  if (table.count(key) > 0) {
    sscanf(table.at(key).c_str(), "%d", &val);
  }
}

void ShowHint() {
  printf("Use rf train|test train_data|test_data\n");
}

int main(int argc, char *args[]) {
  if (argc <= 2) {
    ShowHint();
    return 1;
  }

  ArgsTable table;

  std::string first, second;
  for (int i = 3; i < argc; ++i) {
    if (i % 2 == 1) {
      first = args[i];
    } else {
      second = args[i];
      table[first] = second;
    }
  }

  // printf("ArgTable:\n");
  // for (auto &pair : table) {
  //   printf("%s: %s\n", pair.first.c_str(), pair.second.c_str());
  // }

  std::string arg1 = args[1];
  std::string arg2 = args[2];

  DataReader reader(arg2);
  int threading = -1;
  TableValToInt(table, "-p", threading);


  if (arg1 == "train") {
    // TreeInfo
    int max_depth = 10;
    int min_samples_split = 2;
    int tree_count = 100;
    int one_sample_size = 1000;
    TableValToInt(table, "-d", max_depth);
    TableValToInt(table, "-min-split", min_samples_split);
    TableValToInt(table, "-c", tree_count);
    TableValToInt(table, "-sample-size", one_sample_size);

    DecisionTreeInfo info;
    info.max_depth = max_depth;
    info.min_samples_split = min_samples_split;
    RandomForest rf(201, reader.samples, threading, info, tree_count, one_sample_size);

    rf.CalcTrees();
    rf.SaveTreesToFile("tree.bin");
  } else if (arg1 == "test") {
    RandomForest rf(201, reader.samples, threading);
    rf.LoadTreesFromFile("tree.bin");
    rf.TestAndSave("test_res.csv");
  } else {
    ShowHint();
  }

  return 0;
}
