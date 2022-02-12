//
// Created by Alexander Graschenkov on 11.02.2022.
//


#include <iostream>
#include <chrono>
#include <thread>
#include "benchmark.hpp"

using namespace roadar;

inline void sleep_ms(long long val) {
  std::this_thread::sleep_for(std::chrono::milliseconds(val));
}

void algorithm1() {
  benchmarkStart("algo_1");
  for (int i = 0; i < 10; i++) {
    benchmarkStart("step_1_1");
    sleep_ms(10);
    benchmarkStop("step_1_1");
  }

  for (int i = 0; i < 10; i++) {
    benchmarkStart("step_1_2");
    BENCHMARK("part_1") {
      sleep_ms(15);
    }
    BENCHMARK("part_2") {
      sleep_ms(15);
    }
    benchmarkStop("step_1_2");
  }
  benchmarkStop("algo_1");
}

void algorithm2() {
  benchmarkStart("algo_2");
  for (int i = 0; i < 50; i++) {
    benchmarkStart("step_2_1");
    sleep_ms((long long)i);
    benchmarkStop("step_2_1");
  }
  benchmarkStop("algo_2");
}

int main(int argc, const char * argv[]) {
  std::cout << "Program started" << std::endl;
  std::thread t1(algorithm1);
  std::thread t2(algorithm1);
  std::thread t3(algorithm2);
  t1.join();
  t2.join();
  t3.join();
  benchmarkStart("test not finished");
  sleep_ms(10);
  
  std::cout << "Done" << std::endl;
  std::cout << benchmarkLog() << std::endl;
}