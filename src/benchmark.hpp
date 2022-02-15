/*!
 * \file
 * \brief Модуль бенчмарк-логов.
 */

#pragma once

#include <string>


//! Шаблон для бенчмарк-теста производительности.
#define ROADAR_BENCHMARK(__identifier)                                  \
  for (bool finished = !roadar::benchmarkStart(__identifier); !finished; \
    finished = true, roadar::benchmarkStop(__identifier))


namespace roadar {

/*!
 * \brief Начало бенчмарка.
 * \param[in] identifier Идентификатор.
 * \return Флаг успешного завершения метода.
 */
  bool benchmarkStart(const std::string &identifier);

/*!
 * \brief Конец бенчмарка.
 * \param[in] identifier Идентификатор.
 * \return Флаг успешного завершения метода.
 */
  bool benchmarkStop(const std::string &identifier);

/*!
 * \brief Бенчмарк-лог.
 * \param asThree Сохранение в виде дерева.
 * \return Текст лога.
 */
    std::string benchmarkLog();


  class ScopedBenchmark {
  public:
    explicit ScopedBenchmark(const std::string& identifier);
    ~ScopedBenchmark();
  private:
    std::string m_identifier;
  };

} // namespace RoadAR