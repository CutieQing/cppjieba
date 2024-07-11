#pragma once

#include "Custome_DatTrie.hpp"
#include "Unicode.hpp"
#include "limonp/Logging.hpp"
#include "limonp/StringUtil.hpp"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <stdint.h>
#include <string>

namespace custom {

using namespace limonp;

const double MIN_DOUBLE = -3.14e+100;
const double MAX_DOUBLE = 3.14e+100;
const size_t DICT_COLUMN_NUM = 2;

class DictTrie {
public:
  enum UserWordWeightOption {
    WordWeightMin,
    WordWeightMedian,
    WordWeightMax,
  }; // enum UserWordWeightOption

  DictTrie() {}

  ~DictTrie() {}
  void Init(const string &dict_path, const string &user_dict_paths = "",
            string dat_cache_path = "",
            UserWordWeightOption user_word_weight_opt = WordWeightMedian) {
    const auto dict_list = dict_path + "|" + user_dict_paths;
    size_t file_size_sum = 0;
    const string md5 = CalcFileListMD5(dict_list, file_size_sum);

    if (dat_cache_path.empty()) {
      dat_cache_path = dict_path + "." + md5 + "." +
                       to_string(user_word_weight_opt) + ".dat_cache";
    }

    if (dat_.InitAttachDat(dat_cache_path, md5)) {
      LoadUserDict(user_dict_paths,
                   false); // for load user_dict_single_chinese_word_;
      total_dict_size_ = file_size_sum;
      _initialed = true;
      return;
    }

    LoadDefaultDict(dict_path);
    freq_sum_ = CalcFreqSum(static_node_infos_);
    CalculateWeight(static_node_infos_, freq_sum_);
    double min_weight = 0;
    SetStaticWordWeights(user_word_weight_opt, min_weight);
    dat_.SetMinWeight(min_weight);

    LoadUserDict(user_dict_paths);
    const auto build_ret =
        dat_.InitBuildDat(static_node_infos_, dat_cache_path, md5);
    assert(build_ret);
    total_dict_size_ = file_size_sum;
    vector<DatElement>().swap(static_node_infos_);
    _initialed = true;
  }

  int end() const { return -1; }

  int find(const string &key) const {
    if (!_initialed)
      throw std::runtime_error("Dict not initialed");

    double val;
    return dat_.find(key, val);
  }

  double operator[](const string &key) const {
    double val;
    if (dat_.find(key, val) != 0)
      throw std::runtime_error("index out of range");
    return val;
  }

  bool IsUserDictSingleChineseWord(const cppjieba::Rune &word) const {
    return IsIn(user_dict_single_chinese_word_, word);
  }

  double GetMinWeight() const { return dat_.GetMinWeight(); }

  size_t GetTotalDictSize() const { return total_dict_size_; }

  void InserUserDictNode(const string &line, bool saveNodeInfo = true) {
    vector<string> buf;
    DatElement node_info;
    Split(line, buf, " ");

    if (buf.size() == 0) {
      return;
    }

    node_info.word = buf[0];
    node_info.weight = user_word_default_weight_;
    node_info.val = atof(buf[1].c_str());

    if (saveNodeInfo) {
      static_node_infos_.push_back(node_info);
    }

    if (cppjieba::Utf8CharNum(node_info.word) == 1) {
      cppjieba::RuneArray word;

      if (cppjieba::DecodeRunesInString(node_info.word, word)) {
        user_dict_single_chinese_word_.insert(word[0]);
      } else {
        XLOG(ERROR) << "Decode " << node_info.word << " failed.";
      }
    }
  }

  void LoadUserDict(const string &filePaths, bool saveNodeInfo = true) {
    vector<string> files = limonp::Split(filePaths, "|;");

    for (size_t i = 0; i < files.size(); i++) {
      ifstream ifs(files[i].c_str());
      XCHECK(ifs.is_open()) << "open " << files[i] << " failed";
      string line;

      for (; getline(ifs, line);) {
        if (line.size() == 0) {
          continue;
        }

        InserUserDictNode(line, saveNodeInfo);
      }
    }
  }

private:
  void LoadDefaultDict(const string &filePath) {
    ifstream ifs(filePath.c_str());
    XCHECK(ifs.is_open()) << "open " << filePath << " failed.";
    string line;
    vector<string> buf;

    for (; getline(ifs, line);) {
      Split(line, buf, " ");
      XCHECK(buf.size() == DICT_COLUMN_NUM)
          << "split result illegal, line:" << line;
      DatElement node_info;
      node_info.word = buf[0];
      node_info.weight = 1;
      node_info.val = atof(buf[1].c_str());
      static_node_infos_.push_back(node_info);
    }
  }

  static bool WeightCompare(const DatElement &lhs, const DatElement &rhs) {
    return lhs.weight < rhs.weight;
  }

  void SetStaticWordWeights(UserWordWeightOption option, double &min_weight) {
    XCHECK(!static_node_infos_.empty());
    vector<DatElement> x = static_node_infos_;
    sort(x.begin(), x.end(), WeightCompare);
    if (x.empty()) {
      return;
    }
    min_weight = x[0].weight;
    const double max_weight_ = x[x.size() - 1].weight;
    const double median_weight_ = x[x.size() / 2].weight;

    switch (option) {
    case WordWeightMin:
      user_word_default_weight_ = min_weight;
      break;

    case WordWeightMedian:
      user_word_default_weight_ = median_weight_;
      break;

    default:
      user_word_default_weight_ = max_weight_;
      break;
    }
  }

  double CalcFreqSum(const vector<DatElement> &node_infos) const {
    double sum = 0.0;

    for (size_t i = 0; i < node_infos.size(); i++) {
      sum += node_infos[i].weight;
    }

    return sum;
  }

  void CalculateWeight(vector<DatElement> &node_infos, double sum) const {
    for (size_t i = 0; i < node_infos.size(); i++) {
      DatElement &node_info = node_infos[i];
      assert(node_info.weight > 0.0);
      node_info.weight = log(double(node_info.weight) / sum);
    }
  }

private:
  vector<DatElement> static_node_infos_;
  size_t total_dict_size_ = 0;
  DatTrie dat_;

  double freq_sum_;
  double user_word_default_weight_;
  unordered_set<cppjieba::Rune> user_dict_single_chinese_word_;
  bool _initialed = false;
};
} // namespace custom
