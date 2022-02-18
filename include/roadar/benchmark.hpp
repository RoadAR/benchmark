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

/*!
 * \brief Бенчмарк-лог.
 * \param asThree Сохранение в виде дерева.
 * \return Текст лога.
 */
  std::string benchmarkLog();

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