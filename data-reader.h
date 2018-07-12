#ifndef DATA_READER_H
#define DATA_READER_H

#include <fstream>
#include <memory>
#include "sample.h"
#include "util.h"

struct DataReader {
  constexpr static int kBufferSize = 2048;

  DataReader(const std::string &filename) {
    printf("Reading data...\n");
    std::ifstream ifs;
    ifs.open(filename);
    if (!ifs.good()) {
      throw std::string("File not opened");
    }
    char buffer[kBufferSize] = {};
    int count = 0;
    while (!ifs.eof()) {
      ifs.getline(buffer, kBufferSize);
      std::vector<std::string> line_split;
      SplitString(line_split, buffer, " ");
      if (line_split.empty()) continue;
      ++count;
      if (count % 100000 == 0) printf("Readed %d records\n", count);
      std::unordered_map<int, double> data;
      for (int i = 1; i < line_split.size(); ++i) {
        // i == 0 is label
        int index = 0;
        double val = 0.0;
        sscanf(line_split[i].c_str(), "%d:%lf", &index, &val);
        data[index] = val;
      }
      constexpr char kZero = '0';
      LabelType label = line_split[0][0] - kZero;
      samples.push_back({ label, std::move(data) });
    }
    printf("Total: %d records\n", count);
  }

  std::vector<Sample> samples;
};

#endif
