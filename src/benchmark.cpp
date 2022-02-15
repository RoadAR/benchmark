#include "benchmark.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <sstream>
#include <thread>


#ifndef _WIN32
#include <sys/time.h>
#endif

#ifndef CAPTURE_LAST_N_TIMES
#define CAPTURE_LAST_N_TIMES 10
#endif

#ifndef BENCHMARK_DISABLE_AUTONESTING
#define AUTONESTING
#endif

#ifndef BENCHMARK_DISABLE
#define USE_BENCHMARK
#endif


// -------------------------------------------------

namespace roadar {

#ifdef AUTONESTING
// Мультипоточка всегда включена для автоматического включения замеров к родителю
static const std::string kNestingString = " ";
#endif



using namespace std;
typedef unsigned long long timestamp_t;

static timestamp_t get_timestamp() {
#ifndef _WIN32
  struct timeval now{};
  gettimeofday(&now, nullptr);
  return now.tv_usec + (timestamp_t)now.tv_sec * 1000000;
#else
  return duration_cast<microseconds>(steady_clock::now().time_since_epoch())
        .count();
#endif
}

/*!
 * \brief Информация о замерах.
 */
struct MeasurmentInfo {
  double totalTime = 0;
  unsigned long timesExecuted = 0;
  timestamp_t lastStartTime = 0;
  double lastNTimes[CAPTURE_LAST_N_TIMES] = {};
  unsigned long startNTimesIdx = 0;
};

struct MesurmentGroup {
  MesurmentGroup() = default;
  MesurmentGroup(MesurmentGroup const&val) {
    map = val.map;
  };
  unordered_map<string, MeasurmentInfo> map = {};
#ifdef AUTONESTING
  string parentKey;
#endif
};

inline bool stringHasSuffix(std::string const &value, std::string const &suffix) {
    if (suffix.size() > value.size()) return false;
    return std::equal(suffix.rbegin(), suffix.rend(), value.rbegin());
}

inline bool stringHasPrefix(const std::string &str, const std::string &prefix) {
  auto res = mismatch(prefix.begin(), prefix.end(), str.begin());
  return res.first == prefix.end();
}

static unordered_map<size_t, MesurmentGroup> measurmentThreadMap;
static mutex mut;

inline MesurmentGroup &getMeasurmentGroup() {
  size_t tid = std::hash<std::thread::id>()(std::this_thread::get_id());
  lock_guard<mutex> lock(mut);
  if (measurmentThreadMap.count(tid) == 0) {
      measurmentThreadMap.emplace(tid, MesurmentGroup());
  }
  return measurmentThreadMap[tid];
}

bool benchmarkStart(const string &identifier) {
#ifdef USE_BENCHMARK

  auto &measurmentGroup = getMeasurmentGroup();
  auto &measurmentMap = measurmentGroup.map;
#ifdef AUTONESTING
//  переопределяем identifier, чтобы не делать при ненадобности его копию
//  опасненько, но делаем это аккуртно
  if (measurmentGroup.parentKey.empty()) {
    measurmentGroup.parentKey += identifier;
  } else {
    measurmentGroup.parentKey += kNestingString + identifier;
  }
#define identifier measurmentGroup.parentKey
#endif
  
  if (measurmentMap.count(identifier) == 0)
    measurmentMap[identifier] = {};

  MeasurmentInfo &info = measurmentMap[identifier];
  if (info.lastStartTime)
    return false;

#undef identifier
  info.lastStartTime = get_timestamp();
#endif
  return true;
}

bool benchmarkStop(const string &identifier) {
#ifdef USE_BENCHMARK
  auto &measurmentGroup = getMeasurmentGroup();
  auto &measurmentMap = measurmentGroup.map;
  
#ifdef AUTONESTING
//  переопределяем identifier, чтобы не делать при ненадобности его копию
//  опасненько, но делаем это аккуртно
  assert(stringHasSuffix(measurmentGroup.parentKey, identifier) && "Остановка бенчмарка для другого ключа");
#define identifier measurmentGroup.parentKey
#endif
  
  if (measurmentMap.count(identifier) == 0)
    return false;
  MeasurmentInfo &info = measurmentMap[identifier];
  if (info.lastStartTime == 0)
    return false;

#ifdef AUTONESTING
#undef identifier
  long toIdx = (long)measurmentGroup.parentKey.length() - (long)identifier.length() - (long)kNestingString.length();
  measurmentGroup.parentKey = measurmentGroup.parentKey.substr(0, max((long)0, toIdx));
#endif
  
  double time = static_cast<double>(get_timestamp() - info.lastStartTime);
  info.totalTime += time;
  info.timesExecuted++;
  info.lastStartTime = 0;
  info.lastNTimes[info.startNTimesIdx % CAPTURE_LAST_N_TIMES] = time;
  info.startNTimesIdx++;
#endif
  return true;
}

// Out measurments

double getTotalExecutionTime(const vector<pair<string, MeasurmentInfo>> &measureVector) {
#ifdef USE_BENCHMARK
  int count = static_cast<int>(measureVector.size());
  vector<int> parents(measureVector.size(), -2);

  for (int j = 0; j < count; j++) {
    string prefix = measureVector[j].first;

    for (int i = 0; i < count; i++) {
      if (i == j || !stringHasPrefix(measureVector[i].first, prefix))
        continue;

      parents[i] = j;
    }
  }

  double totalTime = 0;
  for (int i = 0; i < count; i++) {
    if (parents[i] < 0)
      totalTime += measureVector[i].second.totalTime;
  }
  return totalTime;
#else
  return 0;
#endif
}

void sortAsThree(vector<pair<string, MeasurmentInfo>> &measureVector, bool skipPrefix = true) {
#ifdef USE_BENCHMARK
  // make Tree structure
  int count = (int)measureVector.size();

  vector<int> levels(measureVector.size(), 0);
  vector<int> parents(measureVector.size(), -2);
  vector<string> origNames(measureVector.size());

  int maxLevel = 0;
  for (int j = 0; j < count; j++) {
    string prefix = measureVector[j].first;
    origNames[j] = prefix;
#ifdef AUTONESTING
    prefix += kNestingString;
#endif
    
    for (int i = 0; i < count; i++) {
      if (i == j || !stringHasPrefix(measureVector[i].first, prefix))
        continue;

      levels[i]++;
      parents[i] = j;
      if (levels[i] > maxLevel)
        maxLevel = levels[i];
    }
  }

  // make inset
  for (int i = 0; i < count; i++) {
    string name = measureVector[i].first;
    if (skipPrefix && parents[i] >= 0) {
      name = name.substr(origNames[parents[i]].size());
    }

    for (int j = 0; j < levels[i]; j++)
      name = "  " + name;

    measureVector[i].first = name;
  }

  // save exchange order, and then do exchange
  vector<int> order(count);
  for (int i = 0; i < count; i++)
    order[i] = i;

  for (int level = 1; level <= maxLevel; level++) {
    for (int i = count - 1; i >= 0; i--) {
      int origIdx = order[i];
      if (levels[origIdx] != level)
        continue;

      int parentIdx = -1;
      for (int k = 0; k < count; k++) {
        if (parents[origIdx] == order[k])
          parentIdx = k;
      }
      int toIdx = parentIdx + 1;
      if (toIdx > i) toIdx--;
      pair<string, MeasurmentInfo> measure = measureVector[i];
      measureVector.erase(measureVector.begin() + i);
      measureVector.insert(measureVector.begin() + toIdx, measure);

      int orderVal = order[i];
      order.erase(order.begin() + i);
      order.insert(order.begin() + toIdx, orderVal);

      levels[origIdx] = 0;
      i++;
    }
  }
#endif
}

unordered_map<string, MeasurmentInfo> unionMeasurments() {
//  объеденяем все замеры в один результат
  if (measurmentThreadMap.empty()) {
    return {};
  }
  unordered_map<string, MeasurmentInfo> res;
  for (auto &threadMeasurments : measurmentThreadMap) {
    auto &measurmentsMap = threadMeasurments.second;
    if (res.empty()) {
      res = measurmentsMap.map;
    } else {
      for (const auto &measure: measurmentsMap.map) {
        if (res.count(measure.first) == 0) {
          res[measure.first] = measure.second;
        } else {
          auto &destMeasure = res[measure.first];
          destMeasure.totalTime += measure.second.totalTime;
          destMeasure.timesExecuted += measure.second.timesExecuted;
        }
      }
    }
  }
  
  return res;
}

string formGrid(const vector<vector<string>> &rows) {
  vector<int> maxLengths;
  for (const vector<string> &row : rows) {
    for (size_t i = 0; i < row.size(); i++) {
      int s = (int)row[i].size();
      if (i >= maxLengths.size()) {
        maxLengths.push_back(s);
      } else if (maxLengths[i] < s){
        maxLengths[i] = s;
      }
    }
  }

  int border = 1;
  stringstream ss;
  for (const vector<string> &row : rows) {
    for (size_t i = 0; i < row.size(); i++) {
      auto alignment = i == 0 ? left : right;
      ss << setw(maxLengths[i] + border) << alignment << row[i];
    }
    ss << endl;
  }

  return ss.str();
}

template<typename T>
inline string formatString(stringstream &ss, T val) {
  ss.str("");
  ss << val;
  return ss.str();
}

string benchmarkLog() {
#ifdef USE_BENCHMARK
  vector<pair<string, MeasurmentInfo>> measureVector;
  {
    lock_guard<mutex> lock(mut);
    const auto mesurments = unionMeasurments();
    for (const auto &k: mesurments)
      measureVector.emplace_back(k);
  }

  sort(measureVector.begin(), measureVector.end(),
       [](const pair<string, MeasurmentInfo> &a, const pair<string, MeasurmentInfo> &b) -> bool {
         return a.second.totalTime > b.second.totalTime;
       });


  double totalExecutionTime = 0;
  totalExecutionTime = getTotalExecutionTime(measureVector);
  sortAsThree(measureVector);

  vector<vector<string>> rows;
  rows.reserve(measureVector.size());
  stringstream ss;
  for (auto const &k : measureVector) {
    vector<string> row;
    row.push_back(k.first + ":");

    double totalTime = k.second.totalTime;
    unsigned long timesExecuted = k.second.timesExecuted;

    double lastTimesTotal = 0;
    unsigned long avaiableLastMeasurmentCount = k.second.startNTimesIdx < CAPTURE_LAST_N_TIMES
                                           ? k.second.startNTimesIdx
                                           : CAPTURE_LAST_N_TIMES;
    for (long i = 0; i < avaiableLastMeasurmentCount; i++)
      lastTimesTotal += k.second.lastNTimes[i];

    double lastTimes = avaiableLastMeasurmentCount == 0 ? 0.0 : (lastTimesTotal / (double)avaiableLastMeasurmentCount);
    double avg = timesExecuted == 0 ? 0.0 : (totalTime / (double)timesExecuted);
    double percent = totalExecutionTime == 0 ? 0 : totalTime / totalExecutionTime;

    ss << setprecision(2) << fixed;
    row.emplace_back("   total:");
    row.emplace_back(formatString(ss, totalTime / 1000.));
    row.emplace_back("   times:");
    row.emplace_back(formatString(ss, timesExecuted));
    row.emplace_back("   avg:");
    row.emplace_back(formatString(ss, avg / 1000.));
    row.emplace_back("   last avg:");
    row.emplace_back(formatString(ss, lastTimes / 1000.));

    ss << setprecision(1) << fixed;
    row.emplace_back("  percent:");
    row.emplace_back(formatString(ss, int(percent * 1000)/10.) + " %");

    rows.push_back(move(row));
  }
  string result = "\n================== Benchmark ==================\n";
  result += formGrid(rows);
  result += "===============================================\n";
  return result;
#else
  return string();
#endif
}

ScopedBenchmark::ScopedBenchmark(const std::string& identifier): m_identifier(identifier) {
  benchmarkStart(identifier);
};

ScopedBenchmark::~ScopedBenchmark() {
  benchmarkStop(m_identifier);
}

} // namespace RoadAR