//
// Created by Alexander Graschenkov on 11.02.2022.
//


#include <iostream>
#include <chrono>
#include <thread>
#include "benchmark.hpp"

//using namespace roadar;

inline void sleep_ms(long long val) {
  std::this_thread::sleep_for(std::chrono::milliseconds(val));
}

void algorithm1() {
  roadar::benchmarkStart("algo_1");
  for (int i = 0; i < 10; i++) {
    roadar::benchmarkStart("step_1_1");
    sleep_ms(10);
    roadar::benchmarkStop("step_1_1");
  }

  for (int i = 0; i < 10; i++) {
    roadar::ScopedBenchmark bench("scoped");
    roadar::benchmarkStart("step_1_2");
    ROADAR_BENCHMARK("part_1") {
      sleep_ms(15);
    }
    ROADAR_BENCHMARK("part_2") {
      sleep_ms(15);
    }
    roadar::benchmarkStop("step_1_2");
  }
  roadar::benchmarkStop("algo_1");
}

void algorithm2() {
  roadar::benchmarkStart("algo_2");
  for (int i = 0; i < 50; i++) {
    roadar::benchmarkStart("step_2_1");
    sleep_ms((long long)i);
    roadar::benchmarkStop("step_2_1");
  }
  roadar::benchmarkStop("algo_2");
}

int main(int argc, const char * argv[]) {
  std::cout << "Program started" << std::endl;
  std::thread t1(algorithm1);
  std::thread t2(algorithm1);
  std::thread t3(algorithm1);
  std::thread t4(algorithm1);
  std::thread t5(algorithm2);
  t1.join();
  t2.join();
  t3.join();
  t4.join();
  t5.join();
  roadar::benchmarkStart("test not finished");
  sleep_ms(10);

  std::cout << "Done" << std::endl;
  std::cout << roadar::benchmarkLog() << std::endl;
  return 0;
}