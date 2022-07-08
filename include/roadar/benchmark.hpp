/*!
* \file
* \brief Модуль бенчмарк-логов.
*/

#pragma once

#include <string>


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
  bool benchmarkStart(const std::string &identifier, const std::string &file = "", int line = 0);

/*!
* \brief Конец бенчмарка.
* \param[in] identifier Идентификатор.
*/
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
  std::string benchmarkLog(Field withoutFields = Field::none);

/*!
* \brief Очищает все завершенные замеры
*/
  void benchmarkReset();


  class ScopedBenchmark {
  public:
    explicit ScopedBenchmark(const std::string& identifier);
    void reset(const std::string& newIdentifier);
    ~ScopedBenchmark();
  private:
    std::string m_identifier;
  };

// To view result of tracing use https://ui.perfetto.dev/
  void benchmarkStartTracing(const std::string &writeJsonPath, const std::string &file = "", int line = 0);
  void benchmarkStopTracing();
  void benchmarkTracingThreadName(const std::string &name);
} // namespace RoadarNumbers
