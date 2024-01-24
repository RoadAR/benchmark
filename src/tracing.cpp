//
//  tracing.cpp
//  TestBenchmark
//
//  Created by Alexander Graschenkov on 24.01.2024.
//

#include <roadar/tracing.hpp>
#include <sstream>

namespace roadar {
namespace Tracing {

Serializer::Serializer(const std::string& filepath, bool flushOnMeasure, std::string &outErrMsg)
: flushOnMeasure_(flushOnMeasure) {
    std::lock_guard<std::mutex> lock(mut_);
    outStream_.open(filepath);
    
    if (outStream_.is_open()) {
        writeHeader();
    } else {
        outErrMsg = "RBenchmark::Tracing::Serializer could not open results file:\n" + filepath;
    }
}
Serializer::~Serializer() {
    end();
}

void Serializer::end() {
    std::lock_guard<std::mutex> lock(mut_);
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
    {
        std::lock_guard<std::mutex> lock(threadIdMutex_);
        for (const auto &d: data_) {
            write(d, true);
        }
    }
    data_.clear();
    writeFooter();
    outStream_.close();
}


void Serializer::saveTrace(TraceInfo info) {
    std::lock_guard<std::mutex> lock(mut_);
    data_.push_back(std::move(info));
}

void Serializer::write(const TraceInfo& info, bool threadSafe) {
    std::stringstream json;
    
    auto tidIdx = getThreadIdx(info.tid, false);
    json << std::setprecision(3) << std::fixed;
    json << ",{";
    json << "\"cat\":\"function\",";
    json << "\"dur\":" << (info.duration) << ',';
    json << "\"name\":\"" << info.name << "\",";
    json << "\"ph\":\"X\",";
    json << "\"pid\":0,";
    json << "\"tid\":" << tidIdx << ",";
    json << "\"ts\":" << info.startTime;
    json << "}";
    
    if (threadSafe) {
        write(json.str(), flushOnMeasure_);
    } else {
        std::lock_guard<std::mutex> lock(mut_);
        write(json.str(), flushOnMeasure_);
    }
}

void Serializer::writeThreadName(const std::thread::id &tid, const std::string &name) {
    std::stringstream json;
    
    auto tidIdx = getThreadIdx(tid, true);
    json << std::setprecision(3) << std::fixed;
    json << ",{";
    json << "\"cat\":\"function\",";
    json << "\"name\":\"thread_name\",";
    json << "\"ph\":\"M\",";
    json << "\"pid\":0,";
    json << "\"tid\":" << tidIdx << ",";
    json << "\"args\":{\"name\":\"" << name << "\"}";
    json << "}";
    
    std::lock_guard<std::mutex> lock(mut_);
    write(json.str(), flushOnMeasure_);
}

void Serializer::writeHeader() {
    outStream_ << R"({"otherData": {},"traceEvents":[{})";
    outStream_.flush();
}

void Serializer::writeFooter() {
    outStream_ << "]}";
    outStream_.flush();
}

void Serializer::write(const std::string &str, bool flush) {
    if (!outStream_.is_open()) return;
    outStream_ << str;
    if(flush) outStream_.flush();
}

int32_t Serializer::getThreadIdx(const std::thread::id &id, bool lock) {
    if (lock) threadIdMutex_.lock();
    if (threadIdxMap_.count(id) == 0) {
        threadIdxMap_[id] = lastThreadIdx_++;
    }
    auto result = threadIdxMap_.at(id);
    if (lock) threadIdMutex_.unlock();
    return result;
}
} // namespace Tracing
} // namespace roadar
