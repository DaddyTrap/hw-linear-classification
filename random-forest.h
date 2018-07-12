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
    const DecisionTreeInfo info = DecisionTreeInfo(), int tree_count = 100,
    int one_sample_size = 1000, const Logger &logger = Logger())
    : samples(samples), threading(threading), features_count(features_count),
      logger(logger), decision_tree_info(info), tree_count(tree_count),
      one_sample_size(one_sample_size) {
    decision_tree_info.features_count = features_count;
    decision_tree_info.max_features = sqrt(features_count);

    // Output info
    std::string infos;
    infos += "\n\tmax-depth: " + std::to_string(decision_tree_info.max_depth) + '\n'
           + "\tmin-split: " + std::to_string(decision_tree_info.min_samples_split) + '\n'
           + "\ttree-count: " + std::to_string(tree_count) + '\n'
           + "\tsample-size: " + std::to_string(one_sample_size) + '\n';
    this->logger.Debug(infos.c_str());
  }

  void SaveTreesToFile(const std::string filename) {
    logger.Info("Saving trees to file...");
    std::ofstream ofs;
    int one_tree_size = sizeof(TreeNode) * pow(2, decision_tree_info.max_depth);
    ofs.open(filename, std::ios_base::binary);
    if (ofs.is_open()) {
      for (auto &tree : trees) {
        ofs.write(reinterpret_cast<const char*>(tree.tree), one_tree_size);
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
        DecisionTree d_tree(CalcGini);
        d_tree.FromInfo(decision_tree_info);
        char *buffer = new char[one_tree_size];
        ifs.read(buffer, one_tree_size);
        d_tree.tree = (reinterpret_cast<TreeNode*>(buffer));
        trees.push_back(std::move(d_tree));
      }
    } else {
      throw std::string("Something wrong in opening file");
    }
    logger.Info("Loading trees done.");
  }

  DecisionTree CalcOneTree(int id) {
    TikTok tt("CalcOneTree id: " + std::to_string(id));
    tt.Tik();
    DecisionTree tree(CalcGini, Logger(), id);
    tree.FromInfo(decision_tree_info);
    // 随机采样
    SamplePtrVec vec;
    std::vector<int> rand_indexes;
    for (int i = 0; i < one_sample_size; ++i) {
      int rand_index = Randomer::RandInt(0, samples.size());
      while (std::find(rand_indexes.begin(), rand_indexes.end(), rand_index)
        != rand_indexes.end()) {
        rand_index = Randomer::RandInt(0, samples.size());
      }
      rand_indexes.push_back(rand_index);
    }
    for (auto index : rand_indexes) {
      vec.push_back(&samples[index]);
    }
    tree.BuildTree(vec);
    tt.Tok();
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

  LabelType TestOne(const Sample &sample, DecisionTree &tree) {
    auto type = tree.TestTree(sample);
    return type;
  }

  void TestAndSave(const std::string &filename) {
    Test();
    SaveTest(filename);
  }

  void SaveTest(const std::string &filename) {
    logger.Info("Saving test result...");
    std::ofstream ofs(filename);
    std::ofstream dr_ofs("_decision_res.txt");
    ofs << "id,label\n";
    for (int i = 0; i < decision_res.size(); ++i) {
      auto &res = decision_res[i];
      double rate = double(res.first) / (res.first + res.second);
      dr_ofs << i << " " << res.first << " " << res.second << std::endl;
      ofs << i << "," << rate << std::endl;
    }
    logger.Info("Saving test result done.");
  }

  inline void AddDecisionWithType(LabelType type, int index) {
    if (type == 0) {
      decision_res[index].first++;
    } else if (type == 1) {
      decision_res[index].second++;
    } else {
      logger.Debug("Type %d is abnormal", type);
    }
  }

  void Test() {
    decision_res.resize(samples.size());
    if (threading == 0) {
      // 无并行
      logger.Info("Use no parallel mode");
      for (auto &tree : trees) {
        TikTok tt("One Tree to all samples");
        tt.Tik();
        for (int i = 0; i < samples.size(); ++i) {
          auto &sample = samples[i];

          auto type = TestOne(samples[i], tree);
          AddDecisionWithType(type, i);
        }
        tt.Tok();
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
    for (int i = 0; i < trees.size(); ++i) {
      auto &tree = trees[i];
      TikTok tt("One Tree to all samples");
      tt.Tik();
      logger.Info("Processing %d-th tree", i);
      for (int j = 0; j < samples.size(); ++j) {
        pool.AddJob([this, j, &tree]() {
          auto &sample = this->samples[j];
          auto type = TestOne(sample, tree);

          // decision_res_mutex.lock();
          AddDecisionWithType(type, j);
          // decision_res_mutex.unlock();
        });
      }
      tt.Tok();
    }
  }

  // 0 for no threading, neg number for using all the cpus, pos number for specifying a certain number
  int threading = 0;
  int tree_count = 100;
  int features_count;
  int one_sample_size = 1000;
  DecisionTreeInfo decision_tree_info = DecisionTreeInfo();
  const std::vector<Sample> &samples;

  std::vector<std::pair<int, int>> decision_res;

  std::vector<DecisionTree> trees;
  Logger logger;
  std::mutex trees_mutex, decision_res_mutex;
};

#endif
