#ifndef DECISION_TREE_H
#define DECISION_TREE_H

#include <cassert>
#include <vector>
#include <memory>
#include <functional>
#include "sample.h"
#include "util.h"
#include <random>
#include <algorithm>
#include <queue>

// #define NO_SORT

using SamplePtrVec = std::vector<const Sample*>;
using LabelType = char;

struct DecisionTree {
  DecisionTree(const std::function<double (const SamplePtrVec&)> &func,
    const Logger &logger = Logger(), int id = 0)
    : CalcCoeff(func), logger(logger), id(id) {}

  struct DecisionTreeInfo {
    int features_count;
    int max_features;
    int max_depth = 10;
    int min_samples_split = 2;
    // int min_samples_leaf = 1;
    // int min_weight_fraction_leaf;
    // int max_leaf_nodes;
    // double min_impurity_split = 1e-7;
  };

  void FromInfo(const DecisionTreeInfo &info) {
    features_count = info.features_count;
    max_features = info.max_features;
    max_depth = info.max_depth;
    min_samples_split = info.min_samples_split;
    // min_samples_leaf = info.min_samples_leaf;
    // min_impurity_split = info.min_impurity_split;
  }

  struct SplitRes {
    SamplePtrVec left, right;
  };

  inline SplitRes GetSplit(const SamplePtrVec &samples, int feature_index, double split_value) {
    SamplePtrVec left, right;
    left.reserve(200);
    right.reserve(200);
    for (auto &sample : samples) {
      if ((*sample)[feature_index] < split_value) {
        left.push_back(sample);
      } else {
        right.push_back(sample);
      }
    }
    return { std::move(left), std::move(right) };
  }

  struct BestSplitRes {
    int feature_index;
    double feature_val;
    SamplePtrVec left, right;
  };

  BestSplitRes GetBestSplit(const SamplePtrVec &samples, const std::vector<int> &feature_indexes) {
    TikTok tt("GetBestSplit");
    // tt.Tik();
    BestSplitRes res;
    double min_gini = 1e8;
    // 对每一个(选中的)特征
    for (auto &feature_index : feature_indexes) {
      // 对每一个样本
      TikTok tt("OneFeature");
      // printf("sample size: %lu\n", samples.size());
      // tt.Tik();
      #ifdef NO_SORT
      for (auto &sample : samples) {
        auto split_res = GetSplit(samples, feature_index, (*sample)[feature_index]);
        // 计算该分裂的指标值(Gini不纯度/信息增量)
        auto gini = CalcCoeff(split_res.left) + CalcCoeff(split_res.right);
        if (gini < min_gini) {
          // 如果是当前最小的 Gini，则使用该分裂
          min_gini = gini;
          res.feature_index = feature_index;
          res.feature_val = (*sample)[feature_index];
          res.left = split_res.left;
          res.right = split_res.right;
        }
      }
      #else
      SamplePtrVec sort_samples = samples;
      std::sort(sort_samples.begin(), sort_samples.end(), [feature_index](auto lhs, auto rhs) {
        return (*lhs)[feature_index] < (*rhs)[feature_index];
      });
      // Use sort
      for (int mid = 0; mid < sort_samples.size(); ++mid) {
        SamplePtrVec left;
        for (int i = 0; i < mid; ++i) {
          left.push_back(sort_samples[i]);
        }
        SamplePtrVec right;
        for (int i = mid; i < sort_samples.size(); ++i) {
          right.push_back(sort_samples[i]);
        }
        auto gini = CalcCoeff(left) + CalcCoeff(right);
        // printf("gini: %lf\n", gini);
        if (gini < min_gini) {
          // 如果是当前最小的 Gini，则使用该分裂
          min_gini = gini;
          res.feature_index = feature_index;
          res.feature_val = sort_samples[mid]->operator[](feature_index);
          res.left = left;
          res.right = right;
        }
      }
      #endif
      // tt.Tok();
    }
    // tt.Tok();
    return res;
  }

  struct TreeNode {
    // -1 for not leaf, -2 for nothing, 0/1 for tag
    LabelType label;
    int feature_index;
    double feature_val;
  };

  TreeNode *tree = nullptr;

  LabelType GetLabel(const SamplePtrVec &samples) {
    if (samples.empty()) {
      return -2;
    }
    auto label_0_count = std::count_if(samples.begin(), samples.end(), [](const Sample* sample) {
      return sample->label == 0;
    });
    return label_0_count > (samples.size() - label_0_count) ? 0 : 1;
  }

  void BuildTreeRecursive(const SamplePtrVec &samples, int depth, int building_node_index) {
    // logger.Debug("%d building depth %d", id, depth);
    // 检测是否应该结束建树
    if (samples.empty()) {
      // 如果该分组为空，停止建树，贴上标签
      // 然而由于上层递归已经判断，不应有该情况，因此改为断言
      assert(false && "此处不应该出现该情况，调试进入循环的代码");
      return;
    }

    // 对当前结点
    // 随机选取 max_features 个 feature
    std::vector<int> chosen_feature_index;
    for (int i = 0; i < max_features; ++i) {
      int rand_index = Randomer::RandInt(0, features_count);
      while (std::find(chosen_feature_index.begin(),
        chosen_feature_index.end(),
        rand_index) != chosen_feature_index.end()) {
        rand_index = Randomer::RandInt(0, features_count);
      }
      chosen_feature_index.push_back(rand_index);
    }
    // 寻找该结点使用哪个样本的哪个特征进行分裂
    auto best_split_res = GetBestSplit(samples, chosen_feature_index);
    // 写入分裂信息到该结点
    tree[building_node_index].feature_index = best_split_res.feature_index;
    tree[building_node_index].feature_val = best_split_res.feature_val;
    tree[building_node_index].label = -1;
    // 检测分裂结果，是否继续建树
    // 如果某侧分裂结果为空，强制产生叶结点，以结果中最多的标签作为结点标签
    if (best_split_res.left.empty() || best_split_res.right.empty()) {
      // tree[building_node_index * 2].label = tree[building_node_index * 2 + 1].label
      //   = GetLabel(MergeVectors(best_split_res.left, best_split_res.right));
      tree[building_node_index].label = GetLabel(samples);
      return;
    }
    // 如果深度达到了最大深度，强制产生叶结点
    if (depth + 1 >= max_depth) {  // 下一次的深度恰好到达最大深度时
      tree[building_node_index * 2].label = GetLabel(best_split_res.left);
      tree[building_node_index * 2 + 1].label = GetLabel(best_split_res.right);
      return;
    }
    // 如果左侧分裂结果太少，强制产生叶结点
    if (best_split_res.left.size() <= min_samples_split) {
      tree[building_node_index * 2].label = GetLabel(best_split_res.left);
    } else {
      // 否则，递归建树
      BuildTreeRecursive(best_split_res.left, depth + 1, building_node_index * 2);
    }
    // 如果右侧分裂结果太少，强制产生叶结点
    if (best_split_res.right.size() <= min_samples_split) {
      tree[building_node_index * 2 + 1].label = GetLabel(best_split_res.right);
    } else {
      // 否则，递归建树
      BuildTreeRecursive(best_split_res.right, depth + 1, building_node_index * 2 + 1);
    }
  }

  void AllocNewTree(int size) {
    if (tree) delete[] tree;
    else tree = new TreeNode[size];
    for (int i = 0; i < size; ++i) tree[i].label = -2;
  }

  void BuildTree(const SamplePtrVec &samples) {
    AllocNewTree(pow(2, max_depth));
    BuildTreeRecursive(samples, 1, 1);
  }

  LabelType TestTree(const Sample &sample, int visiting_index = 1) {
    auto &visiting = tree[visiting_index];
    if (visiting.label == -2) return -2;
    if (visiting.label == 0 || visiting.label == 1) {
      return visiting.label;
    }
    if (sample[visiting.feature_index] < visiting.feature_val) {
      // left
      return TestTree(sample, visiting_index * 2);
    } else {
      // right
      return TestTree(sample, visiting_index * 2 + 1);
    }
  }

  void PrintTree() {
    int size = pow(2, max_depth);
    int depth = 1;
    for (int i = 1; i < size; ++i) {
      auto &node = tree[i];
      printf("(i: %d, v: %lf, l: %d)", node.feature_index, node.feature_val, node.label);
      if (i == depth) {
        printf("\n");
        depth = depth * 2 + 1;
      }
    }
  }

  void TryTree(int visiting_index = 1) {
    auto &visiting = tree[visiting_index];
    if (visiting.label == -2) printf("FAQ!\n");
    if (visiting.label == 1 || visiting.label == 2) return;
    if (visiting.label == -1) {
      TryTree(visiting_index * 2);
      TryTree(visiting_index * 2 + 1);
    }
  }

  int max_features;
  int features_count;
  int max_depth;
  int min_samples_split;
  // int min_samples_leaf;
  // double min_impurity_split;

  Logger logger;
  int id;

  std::function<double (const SamplePtrVec&)> CalcCoeff;
};

inline double CalcGini(const SamplePtrVec &samples) {
  auto count = std::count_if(samples.begin(), samples.end(), [](const Sample *sample) -> bool {
    return sample->label == 0;
  });
  auto p1 = count / double(samples.size());
  auto p2 = (samples.size() - count) / double(samples.size());
  return (p1 * (1.0 - p1)) + (p2 * (1.0 - p2));
}

#endif
