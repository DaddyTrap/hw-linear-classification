#ifndef SAMPLE_H
#define SAMPLE_H

#include <unordered_map>

struct Sample {
  char label;
  std::unordered_map<int, double> data;
  // char features[201];
  double operator[](int feature_index) const {
    try {
      return data.at(feature_index);
    } catch (const std::out_of_range &e) {
      return 0.0;
    }
  }
};

#endif
