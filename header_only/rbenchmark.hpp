/*!
* \file
* \brief Модуль бенчмарк-логов.
*/

#pragma once

#include <string>

#define R_FUNC static

#ifndef BENCHMARK_DISABLED
#define R_BENCHMARK_START(_identifier_) roadar::benchmarkStart(_identifier_, __FILE__, __LINE__)
#define R_BENCHMARK_STOP(_identifier_) roadar::benchmarkStop(_identifier_, __FILE__, __LINE__)
#define R_BENCHMARK(_identifier_)                                  \
 for (bool _r_bench_bool = R_BENCHMARK_START(_identifier_); _r_bench_bool; _r_bench_bool = false, R_BENCHMARK_STOP(_identifier_))

// вспомогательные директивы для создания переменной с номером строчки
#define R_HIDDEN_SCOPED_L__(_identifier_, line) roadar::ScopedBenchmark r_bench##line(_identifier_)
#define R_HIDDEN_SCOPED_L_(_identifier_, line) R_HIDDEN_SCOPED_L__(_identifier_, line)

#define R_BENCHMARK_SCOPED(_identifier_) roadar::ScopedBenchmark r_bench(_identifier_)
#define R_BENCHMARK_SCOPED_RESET(_identifier_) r_bench.reset(_identifier_)
#define R_BENCHMARK_SCOPED_L(_identifier_) R_HIDDEN_SCOPED_L_(_identifier_, __LINE__)

#define R_BENCHMARK_LOG(_without_fields_) roadar::benchmarkLog(_without_fields_)
#define R_BENCHMARK_RESET() roadar::benchmarkReset()

// To view result of tracing use https://ui.perfetto.dev/
#define R_TRACING_START(_file_name_) roadar::benchmarkStartTracing(_file_name_, __FILE__, __LINE__)
#define R_TRACING_STOP() roadar::benchmarkStopTracing()
#define R_TRACING_THREAD_NAME(_thread_name_) roadar::benchmarkTracingThreadName(_thread_name_)

#else
#define R_BENCHMARK_START(_identifier_)
#define R_BENCHMARK_STOP(_identifier_)
#define R_BENCHMARK(_identifier_)
#define R_BENCHMARK_SCOPED(_identifier_)
#define R_BENCHMARK_SCOPED_RESET(_identifier_)
#define R_BENCHMARK_SCOPED_L(_identifier_)
#define R_BENCHMARK_LOG(_without_fields_) "Benchmark disabled"
#define R_BENCHMARK_RESET()
#define R_TRACING_START(_file_name_)
#define R_TRACING_STOP()
#define R_TRACING_THREAD_NAME(_name_)
#endif

//!
#define R_BENCHMARK_ENUM_FLAG_OPERATORS(T)                                                                                                                                            \
   inline T operator~ (T a) { return static_cast<T>( ~static_cast<std::underlying_type<T>::type>(a) ); }                                                                       \
   inline T operator| (T a, T b) { return static_cast<T>( static_cast<std::underlying_type<T>::type>(a) | static_cast<std::underlying_type<T>::type>(b) ); }                   \
   inline T operator& (T a, T b) { return static_cast<T>( static_cast<std::underlying_type<T>::type>(a) & static_cast<std::underlying_type<T>::type>(b) ); }                   \
   inline T operator^ (T a, T b) { return static_cast<T>( static_cast<std::underlying_type<T>::type>(a) ^ static_cast<std::underlying_type<T>::type>(b) ); }                   \
   inline T& operator|= (T& a, T b) { return reinterpret_cast<T&>( reinterpret_cast<std::underlying_type<T>::type&>(a) |= static_cast<std::underlying_type<T>::type>(b) ); }   \
   inline T& operator&= (T& a, T b) { return reinterpret_cast<T&>( reinterpret_cast<std::underlying_type<T>::type&>(a) &= static_cast<std::underlying_type<T>::type>(b) ); }   \
   inline T& operator^= (T& a, T b) { return reinterpret_cast<T&>( reinterpret_cast<std::underlying_type<T>::type&>(a) ^= static_cast<std::underlying_type<T>::type>(b) ); }


namespace roadar {

/*!
* \brief Начало бенчмарка.
* \param[in] identifier Идентификатор.
* \return Всегда возвращает `true`
*/
  R_FUNC
  bool benchmarkStart(const std::string &identifier, const std::string &file = "", int line = 0);

/*!
* \brief Конец бенчмарка.
* \param[in] identifier Идентификатор.
*/
  R_FUNC
  void benchmarkStop(const std::string &identifier, const std::string &file = "", int line = 0);

  enum class Field {
    none          = 0,
    total         = 1<<0,   // 0x01
    times         = 1<<1,   // 0x02
    average       = 1<<2,   // 0x04
    lastAverage   = 1<<3,   // 0x08
    percent       = 1<<4,   // 0x10
    percentMissed = 1<<5    // 0x20
  };
  R_BENCHMARK_ENUM_FLAG_OPERATORS(Field)

/*!
* \brief Бенчмарк-лог.
* \return Текст лога.
*/
  R_FUNC
  std::string benchmarkLog(Field withoutFields = Field::none);

/*!
* \brief Очищает все завершенные замеры
*/
  R_FUNC
  void benchmarkReset();


  class ScopedBenchmark {
  public:
    explicit ScopedBenchmark(const std::string& identifier): m_identifier(identifier) {
#ifndef BENCHMARK_DISABLED
      benchmarkStart(identifier);
#endif
    }
    void reset(const std::string& newIdentifier) {
#ifndef BENCHMARK_DISABLED
      benchmarkStop(m_identifier);
      m_identifier = newIdentifier;
      benchmarkStart(m_identifier);
#endif
    }
    ~ScopedBenchmark() {
#ifndef BENCHMARK_DISABLED
      benchmarkStop(m_identifier);
#endif
    }
  private:
    std::string m_identifier;
  };

//
/*!
* \brief Use this function when you want to start capture tracing.
 * To view result of tracing use https://ui.perfetto.dev/
* \param[in] writeJsonPath Save results of
*
*/
  R_FUNC
  void benchmarkStartTracing(const std::string &writeJsonPath, const std::string &file = "", int line = 0);
  R_FUNC
  void benchmarkStopTracing();
  R_FUNC
  void benchmarkTracingThreadName(const std::string &name);
} // namespace RoadarNumbers
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
static const std::string kNestingString = " » ";



using namespace std;
typedef unsigned long long timestamp_t;

#ifndef BENCHMARK_DISABLED
  static timestamp_t get_timestamp() {
  //TODO Тут отличие от github версии
  return chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now().time_since_epoch())
      .count();
}
#endif

namespace Tracing {
  struct TraceInfo {
    std::string name;
    uint32_t tid; // thread id
    timestamp_t startTime;
    timestamp_t duration;
    int stackDepth;
  };

  class Serializer {
  public:
    Serializer(const Serializer&) = delete;
    Serializer(Serializer&&) = delete;

    Serializer(const std::string& filepath, bool flushOnMeasure, std::string &outErrMsg)
            : flushOnMeasure_(flushOnMeasure) {
      std::lock_guard<mutex> lock(mut_);
      outStream_.open(filepath);

      if (outStream_.is_open()) {
        writeHeader();
      } else {
        outErrMsg = "RBenchmark::Tracing::Serializer could not open results file:\n" + filepath;
      }
    }
    ~Serializer() {
      end();
    }

    void end() {
      std::lock_guard<mutex> lock(mut_);
      if (!outStream_.is_open()) return;
      sort(data_.begin(), data_.end(), [](const TraceInfo &a, const TraceInfo &b) -> bool {
        if (a.startTime == b.startTime) {
          if (a.duration == b.duration) {
            return a.stackDepth < b.stackDepth;
          } else {
            return a.duration > b.duration;
          }
        } else {
          return a.startTime < b.startTime;
        }
      });
      for (const auto &d :data_) {
        write(d, true);
      }
      data_.clear();
      writeFooter();
      outStream_.close();
    }


    void saveTrace(TraceInfo info) {
      std::lock_guard<mutex> lock(mut_);
      data_.push_back(move(info));
    }

    void write(const TraceInfo& info, bool threadSafe = false) {
      std::stringstream json;

      json << std::setprecision(3) << std::fixed;
      json << ",{";
      json << "\"cat\":\"function\",";
      json << "\"dur\":" << (info.duration) << ',';
      json << "\"name\":\"" << info.name << "\",";
      json << "\"ph\":\"X\",";
      json << "\"pid\":0,";
      json << "\"tid\":" << info.tid << ",";
      json << "\"ts\":" << info.startTime;
      json << "}";

      if (threadSafe) {
        write(json.str(), flushOnMeasure_);
      } else {
        std::lock_guard<mutex> lock(mut_);
        write(json.str(), flushOnMeasure_);
      }
    }

    void writeThreadName(uint32_t tid, const std::string &name) {
      std::stringstream json;

      json << std::setprecision(3) << std::fixed;
      json << ",{";
      json << "\"cat\":\"function\",";
      json << "\"name\":\"thread_name\",";
      json << "\"ph\":\"M\",";
      json << "\"pid\":0,";
      json << "\"tid\":" << tid << ",";
      json << "\"args\":{\"name\":\"" << name << "\"}";
      json << "}";

      std::lock_guard<mutex> lock(mut_);
      write(json.str(), flushOnMeasure_);
    }

  private:
    std::mutex mut_;
    std::ofstream outStream_;
    bool flushOnMeasure_;
    std::vector<TraceInfo> data_;

    void writeHeader() {
      outStream_ << R"({"otherData": {},"traceEvents":[{})";
      outStream_.flush();
    }

    void writeFooter() {
      outStream_ << "]}";
      outStream_.flush();
    }

    void write(const std::string &str, bool flush) {
      if (!outStream_.is_open()) return;
      outStream_ << str;
      if(flush) outStream_.flush();
    }
  };
}

/*!
 * \brief Информация о замерах.
 */
struct MeasurmentInfo {
  double totalTime = 0;
  double childrenTime = 0;
  unsigned long timesExecuted = 0;
  timestamp_t lastStartTime = 0;
  double lastNTimes[CAPTURE_LAST_N_TIMES] = {};
  unsigned long startNTimesIdx = 0;
};

struct MeasurementGroup {
  MeasurementGroup() = default;
  MeasurementGroup(MeasurementGroup const &val) {
    map = val.map;
  };
  unordered_map<string, MeasurmentInfo> map = {};
  uint32_t tid = 0;
  string parentKey;
};

class ErrorMsg {
public:
  ErrorMsg() = default;
  void update(const std::string &msg, const string &file, int line) {
    lock_guard<mutex> lock(mut_);
    if (msg_.empty()) {
      msg_ = msg;
      if (!file.empty()) {
        msg_ += "\nLocation: " + string(file) + " > line: " + to_string(line);
      }
    }
  }

  bool popError(std::string &outMsg) {
    lock_guard<mutex> lock(mut_);
    if (msg_.empty()) {
      return false;
    }
    outMsg = msg_;
    msg_ = "";
    return true;
  }
private:
  mutex mut_;
  string msg_;
};

inline bool stringHasSuffix(std::string const &value, std::string const &suffix) {
  if (suffix.size() > value.size()) return false;
  return std::equal(suffix.rbegin(), suffix.rend(), value.rbegin());
}

inline bool stringHasPrefix(const std::string &str, const std::string &prefix) {
  auto res = mismatch(prefix.begin(), prefix.end(), str.begin());
  return res.first == prefix.end();
}

// without shared_ptr this map fails on Win machine
static unordered_map<uint32_t, shared_ptr<MeasurementGroup>> measurmentThreadMap;
static mutex mut;
static ErrorMsg errorMsg;
static std::unique_ptr<Tracing::Serializer> tracing;

inline uint32_t get_tid() {
  return (uint32_t)std::hash<std::thread::id>()(std::this_thread::get_id());
}
inline MeasurementGroup &getMeasurmentGroup() {
  uint32_t tid = get_tid();
  lock_guard<mutex> lock(mut);
  if (measurmentThreadMap.count(tid) == 0) {
    measurmentThreadMap.emplace(tid, make_shared<MeasurementGroup>());
    measurmentThreadMap[tid]->tid = tid;
  }
  return *measurmentThreadMap[tid].get();
}

bool benchmarkStart(const string &identifier, const string &file, int line) {
#ifndef BENCHMARK_DISABLED

  auto &measurmentGroup = getMeasurmentGroup();
  auto &measurmentMap = measurmentGroup.map;
  if (measurmentGroup.parentKey.empty()) {
    measurmentGroup.parentKey += identifier;
  } else {
    measurmentGroup.parentKey += kNestingString + identifier;
  }

  MeasurmentInfo &info = measurmentMap[measurmentGroup.parentKey];
  if (info.lastStartTime > 0) {
    errorMsg.update("Benchmark already run for \"" + measurmentGroup.parentKey + "\" key", file, line);
    return true;
  }

  info.lastStartTime = get_timestamp();
#endif
  return true;
}

R_FUNC
void failWrongNestingKey(const std::string &fullPath, const std::string &idetifier, const string &file, int line) {
  auto keyFromIdx = fullPath.find_last_of(kNestingString);
  std::string lastKeyPath;
  if (keyFromIdx == string::npos) {
    lastKeyPath = fullPath;
  } else {
    lastKeyPath = fullPath.substr(keyFromIdx+1);
  }
  string msg = "benchmarkStop(\"" + idetifier + "\") not matched with last key \"" + lastKeyPath + "\"";
  errorMsg.update(msg, file, line);
}

void benchmarkStop(const string &identifier, const string &file, int line) {
#ifndef BENCHMARK_DISABLED
  auto &measurmentGroup = getMeasurmentGroup();
  auto &measurmentMap = measurmentGroup.map;

  if (!stringHasSuffix(measurmentGroup.parentKey, identifier)) {
    failWrongNestingKey(measurmentGroup.parentKey, identifier, file, line);
    return;
  }

  if (measurmentMap.count(measurmentGroup.parentKey) == 0) {
    errorMsg.update("Internal error. Missing \"" + measurmentGroup.parentKey + "\" key", file, line);
    return;
  }
  MeasurmentInfo &info = measurmentMap[measurmentGroup.parentKey];
  if (info.lastStartTime == 0) {
    errorMsg.update("Benchmark for \"" + measurmentGroup.parentKey + "\" key not started", file, line);
    return;
  }

  long toIdx = (long)measurmentGroup.parentKey.length() - (long)identifier.length() - (long)kNestingString.length();
  measurmentGroup.parentKey = measurmentGroup.parentKey.substr(0, max((long)0, toIdx));

  auto dt = get_timestamp() - info.lastStartTime;
  auto ts = info.lastStartTime;
  auto time = static_cast<double>(dt);
  info.totalTime += time;
  info.timesExecuted++;
  info.lastStartTime = 0;
  info.lastNTimes[info.startNTimesIdx % CAPTURE_LAST_N_TIMES] = time;
  info.startNTimesIdx++;
  if (tracing) {
    tracing->saveTrace({identifier, measurmentGroup.tid, ts, dt, (int)measurmentGroup.parentKey.length()});
  }
#endif
}

void benchmarkReset() {
#ifndef BENCHMARK_DISABLED
  lock_guard<mutex> lock(mut);
  for (auto &kv : measurmentThreadMap) {
    auto &map = kv.second->map;
    for (auto it = map.begin(); it != map.end(); ) {
      if (it->second.lastStartTime == 0) {
        // reset info for non finished benchmarks
        auto lastStartTime = it->second.lastStartTime;
        it->second = MeasurmentInfo();
        it->second.lastStartTime = lastStartTime;
        it = map.erase(it);
      } else {
        // remove all finished benchmarks
        ++it;
      }
    }
  }

  for (auto it = measurmentThreadMap.begin(); it != measurmentThreadMap.end(); ) {
    if (it->second->map.empty()) {
      // no measurments for thread, cleanup
      it = measurmentThreadMap.erase(it);
    } else {
      ++it;
    }
  }
#endif
}

// Out measurments

R_FUNC
double getTotalExecutionTime(const vector<pair<string, MeasurmentInfo>> &measureVector) {
#ifndef BENCHMARK_DISABLED
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

R_FUNC
void sortAsThree(vector<pair<string, MeasurmentInfo>> &measureVector, bool skipPrefix = true) {
#ifndef BENCHMARK_DISABLED
  // make Tree structure
  int count = (int)measureVector.size();

  vector<int> levels(measureVector.size(), 0);
  vector<int> parents(measureVector.size(), -2);
  vector<string> origNames(measureVector.size());

  int maxLevel = 0;
  for (int j = 0; j < count; j++) {
    string prefix = measureVector[j].first;
    origNames[j] = prefix;
    prefix += kNestingString;

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
      size_t from = origNames[parents[i]].size();
      from += kNestingString.size();
      name = name.substr(from);
      measureVector[parents[i]].second.childrenTime += measureVector[i].second.totalTime;
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

R_FUNC
unordered_map<string, MeasurmentInfo> unionMeasurments() {
//  объеденяем все замеры в один результат
  if (measurmentThreadMap.empty()) {
    return {};
  }
  unordered_map<string, MeasurmentInfo> res;
  for (auto &threadMeasurments : measurmentThreadMap) {
    const auto &measurmentsMap = threadMeasurments.second->map;
    if (res.empty()) {
      res = measurmentsMap;
    } else {
      for (const auto &measure: measurmentsMap) {
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

R_FUNC
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

string benchmarkLog(Field withoutFields) {
#ifndef BENCHMARK_DISABLED
  string errorMsgString;
  if (errorMsg.popError(errorMsgString)) {
    string msg = "\n=============== Benchmark Error ===============\n";
    msg += errorMsgString;
    msg += "\n===============================================\n";
    benchmarkReset();
    return msg;
  }

  vector<pair<string, MeasurmentInfo>> measureVector;
  {
    lock_guard<mutex> lock(mut);
    const auto measurements = unionMeasurments();
    for (const auto &k: measurements)
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
    double childrenTime = k.second.childrenTime;
    unsigned long timesExecuted = k.second.timesExecuted;

    double lastTimesTotal = 0;
    unsigned long avaiableLastMeasurmentCount = k.second.startNTimesIdx < CAPTURE_LAST_N_TIMES
                                                    ? k.second.startNTimesIdx
                                                    : CAPTURE_LAST_N_TIMES;
    for (unsigned long i = 0; i < avaiableLastMeasurmentCount; i++)
      lastTimesTotal += k.second.lastNTimes[i];

    double lastTimes = avaiableLastMeasurmentCount == 0 ? 0.0 : (lastTimesTotal / (double)avaiableLastMeasurmentCount);
    double avg = timesExecuted == 0 ? 0.0 : (totalTime / (double)timesExecuted);
    double percent = totalExecutionTime == 0 ? 0 : totalTime / totalExecutionTime;
    double missed = (totalExecutionTime == 0 || childrenTime == 0) ? 0 : max(0.0, totalTime - childrenTime) / totalExecutionTime;

    ss << setprecision(2) << fixed;
    if (!static_cast<bool>(withoutFields & Field::total)) {
      row.emplace_back("   total:");
      row.emplace_back(formatString(ss, totalTime / 1000.));
    }
    if (!static_cast<bool>(withoutFields & Field::times)) {
      row.emplace_back("   times:");
      row.emplace_back(formatString(ss, timesExecuted));
    }
    if (!static_cast<bool>(withoutFields & Field::average)) {
      row.emplace_back("   avg:");
      row.emplace_back(formatString(ss, avg / 1000.));
    }
    if (!static_cast<bool>(withoutFields & Field::lastAverage)) {
      row.emplace_back("   last avg:");
      row.emplace_back(formatString(ss, lastTimes / 1000.));
    }

    ss << setprecision(1) << fixed;
    if (!static_cast<bool>(withoutFields & Field::percent)) {
      row.emplace_back("   percent:");
      row.emplace_back(formatString(ss, int(percent * 1000) / 10.) + " %");
    }
    if (!static_cast<bool>(withoutFields & Field::percentMissed)) {
      row.emplace_back("   missed:");
      row.emplace_back(formatString(ss, int(missed * 1000) / 10.) + " %");
    }

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

void benchmarkStartTracing(const std::string &writeJsonPath, const std::string &file, int line) {
  lock_guard<mutex> lock(mut);
  string err;
  tracing = unique_ptr<Tracing::Serializer>(new Tracing::Serializer(writeJsonPath, false, err));
  if (!err.empty()) {
    tracing.reset(nullptr);
    errorMsg.update(err, file, line);
  }
}
void benchmarkStopTracing() {
  lock_guard<mutex> lock(mut);
  tracing.reset(nullptr);
}

void benchmarkTracingThreadName(const std::string &name) {
  if (tracing) tracing->writeThreadName(get_tid(), name);
}

} // namespace roadar
