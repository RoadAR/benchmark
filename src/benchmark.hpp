/*!
 * \file
 * \brief Модуль бенчмарк-логов.
 */

#pragma once

#include <string>


namespace roadar {

//! Шаблон для бенчмарк-теста производительности.
#define BENCHMARK(__identifier)                                  \
  for (bool finished = !benchmarkStart(__identifier); !finished; \
       finished = true, benchmarkStop(__identifier))

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

} // namespace RoadAR