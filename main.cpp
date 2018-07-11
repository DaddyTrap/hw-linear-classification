#include "random-forest.h"
#include "data-reader.h"

#include <cstdio>
#include <string>

void ShowHint() {
  printf("Use rf train|test train_data|test_data\n");
}

int main(int argc, char *args[]) {
  if (argc <= 2) {
    ShowHint();
    return 1;
  }
  std::string arg1 = args[1];
  std::string arg2 = args[2];

  if (arg1 == "train") {
    DataReader reader(arg2);
    int threading = -1;
    if (argc > 4) {
      std::string arg3 = args[3];
      std::string arg4 = args[4];
      if (arg3 == "-p") {
        sscanf(arg4.c_str(), "%d", &threading);
      }
    }
    RandomForest rf(201, reader.samples, threading);
    rf.CalcTrees();
    rf.SaveTreesToFile("tree.bin");
  } else if (arg1 == "test") {
    // TODO:
  } else {
    ShowHint();
  }

  return 0;
}
