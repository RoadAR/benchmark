#pragma once

#include <stdio.h>
#include <string>
#include <fstream>
#include <thread>
#include <mutex>
#include <vector>
#include <unordered_map>

namespace roadar {
namespace Tracing {
struct TraceInfo {
  std::string name;
  std::thread::id tid; // thread id
  unsigned long long startTime;
  unsigned long long duration;
  int stackDepth;
};

class Serializer {
public:
  Serializer(const Serializer&) = delete;
  Serializer(Serializer&&) = delete;
  
  Serializer(const std::string& filepath, bool flushOnMeasure, std::string &outErrMsg);
  ~Serializer();
  
  void saveTrace(TraceInfo info);
  
  void write(const TraceInfo& info, bool threadSafe = false);
  
  void writeThreadName(const std::thread::id &tid, const std::string &name);
  
  void end();
  
private:
  std::mutex mut_;
  std::mutex threadIdMutex_;
  std::ofstream outStream_;
  bool flushOnMeasure_;
  std::vector<TraceInfo> data_;
  std::unordered_map<std::thread::id, int32_t> threadIdxMap_;
  int32_t lastThreadIdx_ = 0;
  
  void writeHeader();
  void writeFooter();
  void write(const std::string &str, bool flush);
  
  int32_t getThreadIdx(const std::thread::id &id, bool lock);
};
} // namespace Tracing
} // namespace roadar
