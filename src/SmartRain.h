/*
 * SmartRain.h
 *
 *  Created on: Dec 17, 2014
 *      Author: russell
 */

#ifndef SMARTRAIN_H_
#define SMARTRAIN_H_

#include <iostream>
#include <string>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <chrono>
#include <functional>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

#include "WebServer.h"
#include "LCD.h"
#include "RelayController.h"
#include "WeatherService.h"
#include "EventLogger.h"
#include "util.h"
#include "settings.h"

struct Weather
{
  std::string status;
  double      pressure;
  double      humidity;
  double      temperature;
  double      windspeed;
  const double cNormalPressure = 27.3; // Inches Hg for Yucaipa

  Weather() : pressure(cNormalPressure), humidity(50.0), temperature(290.0), windspeed(0.0)
  {
    // Setup defaults (if no internet connection)
  }

  bool IsRaining()
  {
    if (status == "Rain" || status == "Snow" || status == "Hail" || status == "Sleet")
      return true;

    // High humidity + dropping pressure = likely rain.
    if (humidity >= 90 && pressure < cNormalPressure * 0.9)
      return true;

    return false;
  }

  bool IsWindy()
  {
    // Greater than 25 knot winds (too windy for sprinkler)
    return windspeed > 25.0;
  }

  bool IsCold()
  {
    return temperature < util::KelvinFromFarenheit(40.0);
  }
};

class SmartRain
{
public:
  SmartRain();
  ~SmartRain();

  void Run();

  void Enable(bool enable); // Enables or disables all valves.
  bool IsEnabled();         // Returns true if system is enabled.

  void StartStation(int station, int forMinutes);
  void StopAllStations();
  bool IsStationOn(int station);
  bool IsStationDisabled(int station);
  std::string GetStatusLine(int line);
  std::string GetFormatedTime();

  Weather GetWeather(){return m_weather;}
  void HandleWebRequest(http::request<http::dynamic_body>& req, http::response<http::dynamic_body>& resp);
  void ProcessForm(const std::string& args);
private:
  bool Init();
  bool LoadSettings();
  bool SaveSettings();
  void DrawLCD();
  void ProcessSchedule();
  void UpdateWeatherInfo();
  bool AbortRun(std::string& reason);

  std::string LoadWebPage();

  Settings          m_settings;
  bool              m_connected; 
  int 		          m_runningStation;
  std::chrono::time_point<std::chrono::system_clock> m_stopTime;
  Weather           m_weather;

  EventLogger       m_log;
  WeatherService    m_ws;

  LCDPanel   	      m_lcd;
  RelayController   m_relay;

  std::atomic_bool  m_shutdown;
  std::atomic_bool  m_manualMode;

  std::mutex              m_mux;
  std::condition_variable m_cvWake;
};

#endif /* SMARTRAIN_H_ */
