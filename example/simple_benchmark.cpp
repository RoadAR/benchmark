//
// Created by Alexander Graschenkov on 11.02.2022.
//


#include <iostream>
#include <chrono>
#include <thread>
#include <roadar/benchmark.hpp>

//using namespace roadar;

inline void sleep_ms(long long val) {
  std::this_thread::sleep_for(std::chrono::milliseconds(val));
}

void algorithm1() {
  RBENCHMARK_START("algo_1");
  for (int i = 0; i < 10; i++) {
    roadar::ScopedBenchmark bench("step_1_1");
    sleep_ms(10);
    bench.reset("step_1_2");
    sleep_ms(5);
  }

  for (int i = 0; i < 10; i++) {
    roadar::ScopedBenchmark bench("scoped");
    RBENCHMARK_START("step_1_2");
    RBENCHMARK("part_1") {
      sleep_ms(15);
    }
    RBENCHMARK("part_2") {
      sleep_ms(15);
    }
    RBENCHMARK_STOP("step_1_2");
  }
  // this will be not captured by children `algo_1`
  // so you will see in missed percent
  sleep_ms(25);
  RBENCHMARK_STOP("algo_1");
}

void algorithm2() {
  RBENCHMARK_START("algo_2");
  for (int i = 0; i < 50; i++) {
    RBENCHMARK_START("step_2_1");
    sleep_ms((long long)i);
    RBENCHMARK_STOP("step_2_1");
  }
  RBENCHMARK_STOP("algo_2");
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
  RBENCHMARK_START("test not finished");
  sleep_ms(10);

  std::cout << "Done" << std::endl;
  std::cout << roadar::benchmarkLog(roadar::Field::lastAverage) << std::endl;
  return 0;
}