/*!
* \file
* \brief Модуль бенчмарк-логов.
*/

#pragma once

#include <string>

#define R_FUNC

#ifndef BENCHMARK_DISABLED
#define R_BENCHMARK_START(_identifier_) roadar::benchmarkStart(_identifier_, __FILE__, __LINE__)
#define R_BENCHMARK_STOP(_identifier_) roadar::benchmarkStop(_identifier_, __FILE__, __LINE__)
//-V:R_BENCHMARK:1044
#define R_BENCHMARK(_identifier_)                                  \
 for (bool _r_bench_bool = R_BENCHMARK_START(_identifier_); _r_bench_bool; _r_bench_bool = false, R_BENCHMARK_STOP(_identifier_))

// вспомогательные директивы для создания переменной с номером строчки
#define R_HIDDEN_SCOPED_L__(_identifier_, line) roadar::ScopedBenchmark r_bench##line(_identifier_)
#define R_HIDDEN_SCOPED_L_(_identifier_, line) R_HIDDEN_SCOPED_L__(_identifier_, line)

#define R_BENCHMARK_SCOPED(_identifier_) roadar::ScopedBenchmark r_bench(_identifier_)
#define R_BENCHMARK_SCOPED_RESET(_identifier_) r_bench.reset(_identifier_)
#define R_BENCHMARK_SCOPED_L(_identifier_) R_HIDDEN_SCOPED_L_(_identifier_, __LINE__)

#define R_BENCHMARK_LOG(_without_fields_, ...) roadar::benchmarkLog(_without_fields_, ##__VA_ARGS__)
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

  enum class Format {
    table = 0,
    json = 1
  };

/*!
* \brief Бенчмарк-лог.
* \return Текст лога.
*/
  R_FUNC
  std::string benchmarkLog(Field withoutFields = Field::none, Format format = Format::table,
                           std::ostream *out = nullptr);

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
