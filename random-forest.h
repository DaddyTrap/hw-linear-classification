#ifndef RANDOM_FOREST_H
#define RANDOM_FOREST_H

#include "decision-tree.h"
#include <string>
#include <fstream>
#include <exception>
#include <thread>
#include "simple-threadpool.h"

using DecisionTreeInfo = DecisionTree::DecisionTreeInfo;
using TreeNode = DecisionTree::TreeNode;

struct RandomForest {
  RandomForest(int features_count, const std::vector<Sample> &samples, int threading = 0,
    const Logger &logger = Logger())
    : samples(samples), threading(threading), features_count(features_count),
      logger(logger) {
    decision_tree_info.features_count = features_count;
    decision_tree_info.max_features = sqrt(features_count);
  }

  void SaveTreesToFile(const std::string filename) {
    logger.Info("Saving trees to file...");
    std::ofstream ofs;
    int one_tree_size = sizeof(TreeNode) * pow(2, decision_tree_info.max_depth);
    ofs.open(filename, std::ios_base::binary);
    if (ofs.is_open()) {
      for (auto &tree : trees) {
        ofs.write(reinterpret_cast<const char*>(tree.tree.get()), one_tree_size);
      }
    } else {
      throw std::string("Something wrong in opening file");
    }
    logger.Info("Saving trees done.");
  }

  void LoadTreesFromFile(const std::string filename) {
    logger.Info("Loading trees from file...");
    std::ifstream ifs;
    int one_tree_size = sizeof(TreeNode) * pow(2, decision_tree_info.max_depth);
    ifs.open(filename, std::ios_base::binary);
    if (ifs.is_open()) {
      while (!ifs.eof()) {
        DecisionTree d_tree(CaclGini);
        d_tree.FromInfo(decision_tree_info);
        char *buffer = new char[one_tree_size];
        ifs.read(buffer, one_tree_size);
        d_tree.tree.reset(reinterpret_cast<TreeNode*>(buffer));
        trees.push_back(std::move(d_tree));
      }
    } else {
      throw std::string("Something wrong in opening file");
    }
    logger.Info("Loading trees done.");
  }

  DecisionTree CalcOneTree(int id) {
    DecisionTree tree(CaclGini, Logger(), id);
    tree.FromInfo(decision_tree_info);
    // 随机采样
    SamplePtrVec vec;
    for (int i = 0; i < one_sample_size; ++i) {
      int rand_index = Randomer::RandInt(0, samples.size());
      vec.push_back(&samples[rand_index]);
    }
    tree.BuildTree(vec);
    return tree;
  }

  void CalcTrees() {
    if (threading == 0) {
      // 无并行
      logger.Info("Use no parallel mode");
      // 循环 tree_count 次，生成 tree_count 棵决策树
      for (int i = 0; i < tree_count; ++i) {
        trees.push_back(CalcOneTree(i));
      }
      return;
    }
    // 并行
    int thread_count = threading;
    if (threading < 0) {
      logger.Info("No thread_count specified, check cpu cores...");
      thread_count = std::thread::hardware_concurrency();
    }
    logger.Info("Use %d threads to calculate", thread_count);
    SimpleThreadPool pool(thread_count);
    for (int i = 0; i < tree_count; ++i) {
      logger.Info("Adding %d-th job...", i);
      pool.AddJob([this, i]() {
        auto tree = CalcOneTree(i);

        trees_mutex.lock();
        this->trees.push_back(std::move(tree));
        trees_mutex.unlock();

        this->logger.Info("The %d-th job finished", i);
      });
    }
  }

  // 0 for no threading, neg number for using all the cpus, pos number for specifying a certain number
  int threading = 0;
  int tree_count = 100;
  int features_count;
  int one_sample_size = 1000;
  DecisionTreeInfo decision_tree_info = DecisionTreeInfo();
  const std::vector<Sample> &samples;

  std::vector<DecisionTree> trees;
  Logger logger;
  std::mutex trees_mutex;
};

#endif
