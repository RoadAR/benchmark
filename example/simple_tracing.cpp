//
// Created by Alexander Graschenkov on 13.06.2022.
//


#include <iostream>
#include <chrono>
#include <thread>
#include <roadar/benchmark.hpp>
#include <mutex>
#include <condition_variable>


inline void sleep_ms(long long val) {
  std::this_thread::sleep_for(std::chrono::milliseconds(val));
}

class SimpleSemaphore {
public:
  SimpleSemaphore(uint32_t count = 0, uint32_t maxCount = 5): count_(count), maxCount_(maxCount) {
  }

  inline void notify() {
    {
      std::unique_lock<std::mutex> lock(mtx_);
      while (count_ >= maxCount_)
        cv_.wait_for(lock, std::chrono::milliseconds(interruptWaitMs_));
//      cv_.wait(lock, [&] { return count_ < maxCount_; });
      count_++;
    }
    cv_.notify_one();
  }
  inline void wait() {
    std::unique_lock<std::mutex> lock(mtx_);
    while (count_ <= 0)
      cv_.wait_for(lock, std::chrono::milliseconds(interruptWaitMs_));
//    cv_.wait(lock, [&] { return count_ > 0; });
    count_--;
  }
private:
  std::mutex mtx_;
  std::condition_variable cv_;
  uint32_t count_;
  uint32_t maxCount_;
  long long interruptWaitMs_ = 1;
};


void produceLoop(int threadId, SimpleSemaphore *semaphore, int count, long long delay) {
  R_TRACING_THREAD_NAME("produce_"+ std::to_string(threadId));
  for (int i = 0; i < count; i++) {
    R_BENCHMARK_SCOPED_L("produce");
    R_BENCHMARK("prepare") {
      sleep_ms(delay);
    }
    R_BENCHMARK("notify_wait") {
      semaphore->notify();
    }
  }
}


void readLoop(int threadId, SimpleSemaphore *semaphore, int count, long long processDelay) {
  R_TRACING_THREAD_NAME("read_"+ std::to_string(threadId));
  for (int i = 0; i < count; i++) {
    R_BENCHMARK_SCOPED_L("read");
    R_BENCHMARK("wait") {
      semaphore->wait();
    }
    R_BENCHMARK("process") {
      sleep_ms(processDelay);
    }
  }
}

int main(int argc, const char * argv[]) {
  std::cout << "Program started" << std::endl;
  R_TRACING_START("../tracing.json");
  SimpleSemaphore sem(0, 1);
  std::thread t1(produceLoop, 1, &sem, 5, 5);
  std::thread t2(produceLoop, 2, &sem, 5, 2);
  std::thread t3(readLoop, 3, &sem, 10, 3);
  t1.join();
  t2.join();
  t3.join();
  R_TRACING_STOP();

  std::cout << "Done" << std::endl;
  std::cout << R_BENCHMARK_LOG(roadar::Field::lastAverage) << std::endl;
  std::string appPath = (argv[0]);
  appPath = appPath.substr(0, appPath.find_last_of("/\\"));
  std::string tracePath = appPath.substr(0, appPath.find_last_of("/\\")) + "/tracing.json";
  std::cout << "Use https://ui.perfetto.dev/ to open tracing.json (" << tracePath << ")" << std::endl;
  return 0;
}

