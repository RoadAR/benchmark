#include <roadar/benchmark.hpp>
#include <roadar/tracing.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <sstream>
#include <thread>
#include <iomanip>
#include <chrono>
#include <memory> // unique_ptr

#ifndef _WIN32
#include <sys/time.h>
#endif

#ifndef CAPTURE_LAST_N_TIMES
#define CAPTURE_LAST_N_TIMES 10
#endif


// -------------------------------------------------

namespace roadar {

typedef unsigned long long timestamp_t;
struct MeasurementInfo;
/// К сожалению старая версия  GCC не поддерживает вложенность структур для создания деревьев
/// Не на всех проектах есть возможность обновить GCC
typedef std::unordered_map<std::string, std::unique_ptr<MeasurementInfo>> MeasurementMap;

#ifndef BENCHMARK_DISABLED
  static timestamp_t get_timestamp() {
  //TODO Тут отличие от github версии
  return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
      .count();
}
#endif

/*!
 * \brief Информация о замерах.
 */
struct MeasurementInfo {
  double totalTime = 0;
  double childrenTime = 0;
  unsigned long timesExecuted = 0;
  timestamp_t lastStartTime = 0;
  double lastNTimes[CAPTURE_LAST_N_TIMES] = {};
  unsigned long startNTimesIdx = 0;
  MeasurementMap children;
};

struct MeasurementGroup {
  MeasurementGroup() = default;
//  MeasurementGroup(MeasurementGroup const &val) {
//    map = val.map;
//  };
  MeasurementMap map = {};
  std::thread::id tid;
  std::vector<std::string> measureKey;
  
  MeasurementInfo *getLast(const std::string &addNewKey = "") {
    MeasurementMap *mapRef = &map;
    MeasurementInfo *info = nullptr;
    for (const auto &key : measureKey) {
      info = mapRef->at(key).get();
      mapRef = &(info->children);
    }

    if (!addNewKey.empty()) {
      if ((*mapRef).count(addNewKey) == 0) {
        (*mapRef)[addNewKey] = std::unique_ptr<MeasurementInfo>(new MeasurementInfo());
      }
      info = (*mapRef)[addNewKey].get();
      measureKey.push_back(addNewKey);
    }
    return info;
  }
};

class ErrorMsg {
public:
  ErrorMsg() = default;
  void update(const std::string &msg, const std::string &file, int line) {
    std::lock_guard<std::mutex> lock(mut_);
    if (msg_.empty()) {
      msg_ = msg;
      if (!file.empty()) {
        msg_ += "\nLocation: " + std::string(file) + " > line: " + std::to_string(line);
      }
    }
  }

  bool popError(std::string &outMsg) {
    std::lock_guard<std::mutex> lock(mut_);
    if (msg_.empty()) {
      return false;
    }
    outMsg = msg_;
    msg_ = "";
    return true;
  }
private:
  std::mutex mut_;
  std::string msg_;
};

// without unique_ptr this map fails on Win machine
static std::unordered_map<std::thread::id, std::unique_ptr<MeasurementGroup>> measurementThreadMap;
static std::mutex mut;
static ErrorMsg errorMsg;
static std::unique_ptr<Tracing::Serializer> tracing;

inline MeasurementGroup &getMeasurementGroup() {
  auto tid = std::this_thread::get_id();
  std::lock_guard<std::mutex> lock(mut);
  if (measurementThreadMap.count(tid) == 0) {
    measurementThreadMap.emplace(tid, std::unique_ptr<MeasurementGroup>(new MeasurementGroup()));
    measurementThreadMap[tid]->tid = tid;
  }
  return *measurementThreadMap[tid].get();
}

inline std::string joined(const std::vector<std::string> &array, const std::string &separator = " » ") {
  std::string joinedString;
  for (size_t i = 0; i < array.size(); i++) {
    if (i > 0) joinedString += separator;
    joinedString += array[i];
  }
  return joinedString;
}

bool benchmarkStart(const std::string &identifier, const std::string &file, int line) {
#ifndef BENCHMARK_DISABLED
  auto &group = getMeasurementGroup();
  MeasurementInfo &info = *group.getLast(identifier);
  if (info.lastStartTime > 0) {
    std::string fullPath = joined(group.measureKey);
    errorMsg.update("Benchmark already run for \"" + fullPath + "\" key", file, line);
    return true;
  }
  info.lastStartTime = get_timestamp();
#endif
  return true;
}

void benchmarkStop(const std::string &identifier, const std::string &file, int line) {
#ifndef BENCHMARK_DISABLED
  auto &group = getMeasurementGroup();

  if (group.measureKey.size() == 0 || group.measureKey.back() != identifier) {
    std::string lastKey = group.measureKey.empty() ? "" : group.measureKey.back();
    std::string msg = "benchmarkStop(\"" + identifier + "\") not matched with last key \"" + lastKey + "\"";
    errorMsg.update(msg, file, line);
    return;
  }

  MeasurementInfo &info = *(group.getLast());
  if (info.lastStartTime == 0) {
    std::string fullPath = joined(group.measureKey);
    errorMsg.update("Benchmark for \"" + fullPath + "\" key not started", file, line);
    return;
  }

  group.measureKey.pop_back();

  auto dt = get_timestamp() - info.lastStartTime;
  auto ts = info.lastStartTime;
  auto time = static_cast<double>(dt);
  info.totalTime += time;
  info.timesExecuted++;
  info.lastStartTime = 0;
  info.lastNTimes[info.startNTimesIdx % CAPTURE_LAST_N_TIMES] = time;
  info.startNTimesIdx++;
  if (tracing) {
    tracing->saveTrace({identifier, group.tid, ts, dt, (int)group.measureKey.size()});
  }
#endif
}

void benchmarkReset() {
#ifndef BENCHMARK_DISABLED
  std::lock_guard<std::mutex> lock(mut);
  for (auto &kv : measurementThreadMap) {
    std::vector<MeasurementMap *> cleanupMap = {&(kv.second->map)};
    
    for (size_t idx = 0; idx < cleanupMap.size(); idx++) {
      auto map = cleanupMap[idx];
      for (auto it = map->begin(); it != map->end(); ) {
        if (it->second->lastStartTime == 0) {
          // reset info for non finished benchmarks
          it = map->erase(it);
        } else {
          cleanupMap.push_back(&(it->second->children));
          ++it;
        }
      }
    }
  }

  for (auto it = measurementThreadMap.begin(); it != measurementThreadMap.end(); ) {
    if (it->second->map.empty()) {
      // no measurments for thread, cleanup
      it = measurementThreadMap.erase(it);
    } else {
      ++it;
    }
  }
#endif
}

// Out measurements
/// В этом классе собираем конечные замеры перед переводом в табличное представление
struct MeasurementInfoOut {
  double totalTime = 0;
  double childrenTime = 0;
  double lastTime = 0;
  double currentRunningTime = 0;
  unsigned long timesExecuted = 0;
  std::unordered_map<std::string, std::unique_ptr<MeasurementInfoOut>> children;
  std::vector<std::string> childrenOrder; // нам нужна сортировка по занятому времени
  
  void fill(const MeasurementInfo &info, bool captureLast, bool captureCurrentRunning) {
    totalTime = info.totalTime;
    timesExecuted = info.timesExecuted;
    childrenTime = 0;
    for (const auto &keyVal: info.children) {
      childrenTime += keyVal.second->totalTime;
    }
    
    if (captureLast) {
      double lastTimesTotal = 0;
      unsigned long lastCount = std::min(info.startNTimesIdx, (unsigned long)CAPTURE_LAST_N_TIMES);
      for (unsigned long i = 0; i < lastCount; i++)
        lastTimesTotal += info.lastNTimes[i];

      lastTime = lastCount == 0 ? 0.0 : (lastTimesTotal / (double)lastCount);
    } else {
      lastTime = 0;
    }
    if (captureCurrentRunning && info.lastStartTime > 0) {
      currentRunningTime = get_timestamp() - info.lastStartTime;
    } else {
      currentRunningTime = 0;
    }
    children.clear();
    for (const auto &keyVal: info.children) {
      if (children.count(keyVal.first) == 0) {
        children[keyVal.first] = std::unique_ptr<MeasurementInfoOut>(new MeasurementInfoOut());
      }
      children[keyVal.first]->fill(*keyVal.second, captureLast, captureCurrentRunning);
    }
  }
  
  void merge(const MeasurementInfoOut &other) {
    totalTime += other.totalTime;
    timesExecuted += other.timesExecuted;
    childrenTime += other.childrenTime;
    lastTime += other.lastTime;
    currentRunningTime += other.currentRunningTime;
    for (const auto &key : other.children) {
      children[key.first]->merge(*key.second);
    }
  }
};

static
MeasurementInfoOut unionMeasurements() {
//  объеденяем все замеры в один результат
  MeasurementInfoOut res;
  MeasurementInfoOut tempChild;
  for (auto &threadMeasurements : measurementThreadMap) {
    const auto &measurementsMap = threadMeasurements.second->map;
    for (const auto& keyVal : measurementsMap) {
      if (res.children.count(keyVal.first) > 0) {
        tempChild.fill(*keyVal.second, true, true);
        res.children.at(keyVal.first)->merge(tempChild);
      } else {
        res.children[keyVal.first] = std::unique_ptr<MeasurementInfoOut>(new MeasurementInfoOut());
        res.children.at(keyVal.first)->fill(*keyVal.second, true, true);
      }
    }
  }
  return res;
}

static
void sortChildren(MeasurementInfoOut &info) {
  info.childrenOrder.clear();
  for (auto &keyVal: info.children) {
    info.childrenOrder.push_back(keyVal.first);
  }
  sort(info.childrenOrder.begin(), info.childrenOrder.end(),
       [&info](const std::string &a, const std::string &b) -> bool {
         return info.children.at(a)->totalTime > info.children.at(b)->totalTime;
       });
  
  // recursive
  for (auto &keyVal: info.children) {
    if (!keyVal.second->children.empty()) {
      sortChildren(*keyVal.second);
    }
  }
}

static
void formGrid(const std::vector<std::vector<std::string>> &rows, std::ostream &out) {
  std::vector<int> maxLengths;
  for (const std::vector<std::string> &row : rows) {
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
  for (const std::vector<std::string> &row : rows) {
    for (size_t i = 0; i < row.size(); i++) {
      auto alignment = i == 0 ? std::left : std::right;
      out << std::setw(maxLengths[i] + border) << alignment << row[i];
    }
    out << "\n";
  }
}

template<typename T>
inline std::string formatString(std::stringstream &ss, T val) {
  ss.str("");
  ss << val;
  return ss.str();
}

static void generateTableOutput(const MeasurementInfoOut &root, const Field &withoutFields, std::ostream &out);
static void generateJsonOutput(const MeasurementInfoOut &root, const Field &withoutFields, std::ostream &out);
inline std::string generateError(const std::string &msg, Format format) {
  std::string result;
  switch (format) {
    case Format::table:
      result += "\n=============== Benchmark Error ===============\n";
      result += msg;
      result += "\n===============================================\n";
      break;
    case Format::json:
      result = "{\"error\":\"" + msg + "\"}";
      break;
  }
  return result;
}

struct MeasurementInfoOut;
std::string benchmarkLog(Field withoutFields, Format format, std::ostream *out) {
#ifndef BENCHMARK_DISABLED
  std::string errorMsgString;
  if (errorMsg.popError(errorMsgString)) {
    std::string msg = generateError(errorMsgString, format);
    benchmarkReset();
    if (out) {
      *out << msg;
      return "";
    } else {
      return msg;
    }
  }

  MeasurementInfoOut root;
  {
    std::lock_guard<std::mutex> lock(mut);
    root = unionMeasurements();
    sortChildren(root);
  }
  root.totalTime = 0;
  for (const auto &keyVal : root.children) {
    root.totalTime += keyVal.second->totalTime;
  }
  
  std::stringstream result;
  bool returnEmptyString = true;
  if (out == nullptr) {
    out = &result;
  } else {
    returnEmptyString = false;
  }
  switch (format) {
    case Format::table:
      generateTableOutput(root, withoutFields, *out);
      break;
    case Format::json:
      generateJsonOutput(root, withoutFields, *out);
      break;
  }
  if (returnEmptyString) {
    return "";
  } else {
    return result.str();
  }
#else
  return std::string();
#endif
}

static void generateTableRowsRecursive(const MeasurementInfoOut &root, double totalExecutionTime, int level, const Field &withoutFields, std::vector<std::vector<std::string>> &outRows) {
  
  std::stringstream ss;
  
  for (const auto &key: root.childrenOrder) {
    std::vector<std::string> row;
    row.push_back(std::string(level*2, ' ') + key + ":");
    const auto &info = *root.children.at(key);
    
    ss << std::setprecision(2) << std::fixed;
    if (!static_cast<bool>(withoutFields & Field::total)) {
      row.emplace_back("   total:");
      row.emplace_back(formatString(ss, info.totalTime / 1000.));
    }
    if (!static_cast<bool>(withoutFields & Field::times)) {
      row.emplace_back("   times:");
      row.emplace_back(formatString(ss, info.timesExecuted));
    }
    if (!static_cast<bool>(withoutFields & Field::average)) {
      row.emplace_back("   avg:");
      double avg = info.timesExecuted == 0 ? 0.0 : (info.totalTime / (double)info.timesExecuted);
      row.emplace_back(formatString(ss, avg / 1000.));
    }
    if (!static_cast<bool>(withoutFields & Field::lastAverage)) {
      row.emplace_back("   last avg:");
      row.emplace_back(formatString(ss, info.lastTime / 1000.));
    }
    if (!static_cast<bool>(withoutFields & Field::running)) {
      row.emplace_back("   running:");
      row.emplace_back(formatString(ss, info.currentRunningTime / 1000.));
    }
    
    ss << std::setprecision(1) << std::fixed;
    if (!static_cast<bool>(withoutFields & Field::percent)) {
      row.emplace_back("   percent:");
      double percent = totalExecutionTime == 0 ? 0 : info.totalTime / totalExecutionTime;
      row.emplace_back(formatString(ss, int(percent * 1000) / 10.) + " %");
    }
    if (!static_cast<bool>(withoutFields & Field::percentMissed)) {
      row.emplace_back("   missed:");
      double missed = (totalExecutionTime == 0 || info.childrenTime == 0) ? 0 : std::max(0.0, info.totalTime - info.childrenTime) / totalExecutionTime;
      row.emplace_back(formatString(ss, int(missed * 1000) / 10.) + " %");
    }
    
    outRows.push_back(std::move(row));
    // мне не нравится рекурсия, но пока так; без рекурсии пока не придумал как меньше кода написать
    if (!info.children.empty()) {
      generateTableRowsRecursive(info, totalExecutionTime, level+1, withoutFields, outRows);
    }
  }
}

static void generateTableOutput(const MeasurementInfoOut &root, const Field &withoutFields, std::ostream &out) {
  std::vector<std::vector<std::string>> rows;
  generateTableRowsRecursive(root, root.totalTime, 0, withoutFields, rows);

  out << "\n================== Benchmark ==================\n";
  formGrid(rows, out);
  out << "===============================================\n";
}

static void generateJsonOutput(const MeasurementInfoOut &root, double totalExecutionTime, const Field &withoutFields, std::ostream &out) {
  std::stringstream ss;

  out << "[";
  for (size_t i = 0; i < root.childrenOrder.size(); i++) {
    const auto &name = root.childrenOrder[i];
    const auto &info = *root.children.at(name);
    if (i == 0) {
      out << "{";
    } else {
      out << ",{";
    }

    double totalTime = info.totalTime;
    double childrenTime = info.childrenTime;
    unsigned long timesExecuted = info.timesExecuted;

    double avg = timesExecuted == 0 ? 0.0 : (totalTime / (double) timesExecuted);
    double percent = totalExecutionTime == 0 ? 0 : totalTime / totalExecutionTime;
    double missed = (totalExecutionTime == 0 || childrenTime == 0) ? 0 : std::max(0.0, totalTime - childrenTime) /
                                                                         totalExecutionTime;

    ss << std::setprecision(2) << std::fixed;
    out << "\"name\":\"" << name << "\"";
    if (!static_cast<bool>(withoutFields & Field::total)) {
      out << ",\"total\":" << formatString(ss, totalTime / 1000.);
    }
    if (!static_cast<bool>(withoutFields & Field::times)) {
      out << ",\"times\":" << formatString(ss, timesExecuted);
    }
    if (!static_cast<bool>(withoutFields & Field::average)) {
      out << ",\"avg\":" << formatString(ss, avg / 1000.);
    }
    if (!static_cast<bool>(withoutFields & Field::lastAverage)) {
      out << ",\"last avg\":" << formatString(ss, info.lastTime / 1000.);
    }
    if (!static_cast<bool>(withoutFields & Field::running)) {
      out << ",\"running\":" << formatString(ss, info.currentRunningTime / 1000.);
    }

    ss << std::setprecision(1) << std::fixed;
    if (!static_cast<bool>(withoutFields & Field::percent)) {
      out << ",\"percent\":" << formatString(ss, int(percent * 1000) / 10.);
    }
    if (!static_cast<bool>(withoutFields & Field::percentMissed)) {
      out << ",\"missed\":" << formatString(ss, int(missed * 1000) / 10.);
    }

    if (info.children.empty()) {
      out << "}";
    } else {
      out << ",\"children\":";
      generateJsonOutput(info, totalExecutionTime, withoutFields, out);
    }
  }
  out << "]";
}
static void generateJsonOutput(const MeasurementInfoOut &root, const Field &withoutFields, std::ostream &out) {
  generateJsonOutput(root, root.totalTime, withoutFields, out);
}

void benchmarkStartTracing(const std::string &writeJsonPath, const std::string &file, int line) {
  std::lock_guard<std::mutex> lock(mut);
  std::string err;
  tracing = std::unique_ptr<Tracing::Serializer>(new Tracing::Serializer(writeJsonPath, false, err));
  if (!err.empty()) {
    tracing.reset(nullptr);
    errorMsg.update(err, file, line);
  }
}
void benchmarkStopTracing() {
  std::lock_guard<std::mutex> lock(mut);
  tracing.reset(nullptr);
}

void benchmarkTracingThreadName(const std::string &name) {
  if (tracing) tracing->writeThreadName(std::this_thread::get_id(), name);
}

} // namespace roadar
