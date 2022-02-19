/*!
 * \file
 * \brief Модуль бенчмарк-логов.
 */

#pragma once

#include <string>


//! Шаблон для бенчмарк-теста производительности.
#define RBENCHMARK(__identifier)                                  \
  for (bool finished = !roadar::benchmarkStart(__identifier); !finished; \
    finished = true, roadar::benchmarkStop(__identifier))

//!
#define RBENCHMARK_ENUM_FLAG_OPERATORS(T)                                                                                                                                            \
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
 * \return Флаг успешного завершения метода.
 */
  bool benchmarkStart(const std::string &identifier, const std::string &file = "", int line = 0);
  #define RBENCHMARK_START(_identifier_) roadar::benchmarkStart(_identifier_, __FILE__, __LINE__)

/*!
 * \brief Конец бенчмарка.
 * \param[in] identifier Идентификатор.
 * \return Флаг успешного завершения метода.
 */
  bool benchmarkStop(const std::string &identifier, const std::string &file = "", int line = 0);
  #define RBENCHMARK_STOP(_identifier_) roadar::benchmarkStop(_identifier_, __FILE__, __LINE__)

  enum class Field {
    none          = 0,
    total         = 1<<0,   // 0x01
    times         = 1<<1,   // 0x02
    average       = 1<<2,   // 0x04
    lastAverage   = 1<<3,   // 0x08
    percent       = 1<<4,   // 0x10
    percentMissed = 1<<5    // 0x20
  };
  RBENCHMARK_ENUM_FLAG_OPERATORS(Field)

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

} // namespace RoadAR