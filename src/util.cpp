/*
 * util.cpp
 *
 *  Created on: Jan 11, 2015
 *      Author: russell
 */

#include "util.h"
#include <libgen.h>
#include <string>
#include <sstream>
#include <chrono>
#include <time.h>

namespace util
{
  void msleep(int ms)
  {
    ::usleep(1000*ms);
  }

  std::chrono::time_point<std::chrono::system_clock> TimePointFromString(const char* timeString)
  {
    std::tm tm{};
    ::strptime(timeString, "%H:%M:%S", &tm);
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
  }

  std::string StringFromTimePoint(const std::chrono::time_point<std::chrono::system_clock>& timePoint)
  {
    auto theTime = std::chrono::system_clock::to_time_t(timePoint);
    auto theParts = localtime(&theTime);
    std::stringstream ss;
    ss << theParts->tm_hour << ":" << theParts->tm_min << ":" << theParts->tm_sec;

    return ss.str();
  }

  time_t TimeFromHourMin(const char* hourMin)
  {
    time_t currentTime;
    time(&currentTime);

    std::string sHourMin(hourMin);
    if (sHourMin.empty())
      return currentTime;

    int pos = sHourMin.find(':');
    if (pos == std::string::npos)
      return currentTime;
    
    try
    {
      int hour = std::stoi(sHourMin.substr(0, pos));
      int min = std::stoi(sHourMin.substr(pos + 1));

      tm* tm = localtime(&currentTime);
      tm->tm_hour = hour;
      tm->tm_min = min;

      return mktime(tm);
    }
    catch(...)
    {
    }

    return currentTime;
  }

  std::string StrFromTimePoint(const std::chrono::time_point<std::chrono::system_clock>& timePoint)
  {
    return StrFromTimePoint(timePoint, "%a %b %d %H:%M");
  }

  std::string StrFromTimePoint(const std::chrono::time_point<std::chrono::system_clock>& timePoint, const char* pattern)
  {
    auto t = std::chrono::system_clock::to_time_t(timePoint);
    auto tm = ::localtime(&t);
    char buf[80]={0};
    ::strftime(buf, sizeof(buf), pattern, tm);

    return buf;
  }

  
  double FahrenheitFromKelvin(double kelvin)
  {
    return (kelvin - 273.15) * 1.8 + 32.0;
  }

  double KelvinFromFarenheit(double farenheit)
  {
    return (farenheit + 459.67)*(5.0/9.0);
  }

  std::string ParseRequest(const std::string& request, const char* pname)
  {
    if (request.empty() || pname == nullptr)
      return "";

    std::string name(pname);
    auto pos = request.find(name + "=");
    if (pos == std::string::npos)
      return "";

    auto beg = pos + name.size() + 1;
    auto end = request.find("&", beg);
    auto value = request.substr(beg, end-beg);
    
    // Check for : in value.
    auto chk = value.find("%3A");
    if (chk != std::string::npos)
      value.replace(chk, 3, ":");

    return value;
  }

   std::string GetRunningProcessPath()
   {
     char buf[1024]={0};
     if (0 == readlink("/proc/self/exe", buf, 1024))
       return "";

     std::string result = dirname(buf);

     return result + "/";
   }
}

