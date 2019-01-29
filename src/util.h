/*
 * util.h
 *
 *  Created on: Jan 11, 2015
 *      Author: russell
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <unistd.h>
#include <time.h>
#include <chrono>
#include <string>

namespace util
{
  void msleep(int ms);

  time_t TimeFromHourMin(const char* hourMin);
  std::string StrFromTimePoint(const std::chrono::time_point<std::chrono::system_clock>& timePoint);
   std::string StrFromTimePoint(const std::chrono::time_point<std::chrono::system_clock>& timePoint, const char* pattern);
  double FahrenheitFromKelvin(double kelvin);
  double KelvinFromFarenheit(double farenheit);
  std::string ParseRequest(const std::string& request, const char* pname);
  std::string GetRunningProcessPath();
 }

#endif /* UTIL_H_ */
