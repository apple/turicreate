/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#pragma once

#include <chrono>
#include <ratio>

typedef std::chrono::duration<double, std::ratio<1>> cmDuration;

/*
 * This function will return number of seconds in the requested type T.
 *
 * A duration_cast from duration<double> to duration<T> will not yield what
 * one might expect if the double representation does not fit into type T.
 * This function aims to safely convert, by clamping the double value between
 * the permissible valid values for T.
 */
template <typename T>
T cmDurationTo(const cmDuration& duration);

#ifndef CMDURATION_CPP
extern template int cmDurationTo<int>(const cmDuration&);
extern template unsigned int cmDurationTo<unsigned int>(const cmDuration&);
#endif
