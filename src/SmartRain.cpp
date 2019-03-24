/*
 * SmartRain.cpp
 *
 *  Created on: Dec 17, 2014
 *      Author: russell
 */

#include <sstream>
#include <fstream>
#include <time.h>
#include <sys/inotify.h>
#include <stdlib.h>
#include "SmartRain.h"
#include "util.h"
#include "NetworkInfo.h"
#include "WebServer.h"

static constexpr int stationCount = 4;

int main(int argc, const char* argv[])
{
  SmartRain sr;

  // Run web server on seperate thread
  std::thread wsThread([&sr]()
  {
    WebServer webServer("0::0");
    webServer.Run([&sr](http::request<http::dynamic_body>& req, http::response<http::dynamic_body>& resp)
    {
      sr.HandleWebRequest(req, resp);
    });
  });

  sr.Run();

  wsThread.join();

  return 0;
}

SmartRain::SmartRain() : m_log("log.html"), 
  m_lcd("/dev/i2c-1", 0x27), 
  m_relay(stationCount, "/dev/i2c-1", 0x20), m_manualRunStation(-1)
{
  m_shutdown = false;
  m_runningStation = -1;
}

SmartRain::~SmartRain ()
{
}

void SmartRain::Run()
{
  if (!Init())
    return;

  while(!m_shutdown)
  {
    try
    {
      ProcessSchedule();
      UpdateWeatherInfo();
      DrawLCD();
    }
    catch(...)
    {
    }

    // Update status and check schedule once every 15 seconds.
    std::unique_lock<std::mutex> lk(m_mux);
    auto chk = m_cvWake.wait_for(lk, std::chrono::seconds(15));
    if (chk == std::cv_status::timeout)
      continue;
  }

  std::cout << "Shutting down...\n";
}

bool SmartRain::Init()
{
  m_lcd.PrintLine(0, "   SmartRain  2.0");
  std::this_thread::sleep_for(std::chrono::seconds(2));

  std::cout << "Initializing SmartRain..." << std::endl;

  m_log.WriteEvent("Restarting Smartrain");

  std::cout << "Obtaining IP address." << std::endl;
  NetworkInfo ni;
  std::string ipMsg = "IP Address: ";
  ipMsg += ni.GetIP(1);
  m_log.WriteEvent(ipMsg.c_str());

  LoadSettings();

  UpdateWeatherInfo();

  return true;
}

bool SmartRain::LoadSettings()
{
  std::lock_guard<std::mutex> lk(m_mux);

  std::ifstream strm(util::GetRunningProcessPath() + "settings.txt");
  if (strm.fail())
    return false;
  
  std::cout << "Loading settings file..." << std::endl;

  return m_settings.Load(strm);
}

bool SmartRain::SaveSettings()
{
  std::lock_guard<std::mutex> lk(m_mux);

  std::ofstream strm(util::GetRunningProcessPath() + "settings.txt", std::ios::trunc|std::ios::out);
  if (strm.fail())
    return false;
  
  std::cout << "Saving settings file..." << std::endl;

  return m_settings.Save(strm);
}

void SmartRain::Enable(bool enable)
{
  if (m_settings.getEnabled() == enable)
    return;

  m_settings.setEnabled(enable);
  SaveSettings();

  if (!enable)
  {
    m_log.WriteEvent("System disabled.");

    StopAllStations();
  }
  else
    m_log.WriteEvent("System enabled.");
}

bool SmartRain::IsEnabled()
{
  return m_settings.getEnabled();
}

void SmartRain::StartStation(int station, int forMinutes)
{
  if (station < 0 || station >= stationCount)
    return;

  m_runningStation = station;

  std::stringstream logMsg;
  logMsg << "Running station " << station << ".";
  m_log.WriteEvent(logMsg.str().c_str());

  {
    std::lock_guard<std::mutex> lk(m_mux);
    m_stopTime = std::chrono::system_clock::now() + std::chrono::minutes(forMinutes);
  }

  m_relay.RelayOn(station);
  m_lcd.Reset();
}

bool SmartRain::IsStationOn(int station)
{
  return m_relay.IsRelayOn(station);
}

void SmartRain::StopAllStations()
{
  m_runningStation = -1;
  m_relay.RelaysOff();
  m_lcd.Reset();
}

std::string SmartRain::GetFormatedTime()
{
  time_t rawTime;
  time(&rawTime);

  char formatedTime[64] = {0};
  ::strftime(formatedTime, 64, "%h %d %H:%M", localtime(&rawTime));

  return formatedTime;
}

std::string SmartRain::GetStatusLine(int line)
{
  if (line < 0 || line > 3)
    throw std::invalid_argument("line argument invalid.");

  // Get the current time
  auto now = std::chrono::system_clock::now();

  std::stringstream strm;
    
  switch (line)
  {
    case 0:
    {
      strm << util::StrFromTimePoint(now);
      if (m_settings.getEnabled())
        strm << "  E";
      else
        strm << "  D";

      if (m_connected)
        strm << "C";
      else
        strm << "N";
    }
    break;
    
    case 1:
    {
      strm << m_weather.status << " " << std::round(util::FahrenheitFromKelvin(m_weather.temperature)) << char(0xdf);
    }
    break;
    
    case 2:
    {
      // Third line is unused currently.
    }
    break;

    case 3:
    {
      if (m_runningStation != -1)
      {
        strm << "Sta: " << m_runningStation+1 << "    Time: ";

        if (m_manualRunStation == -1)
        {
          auto remaining = m_stopTime - now;
          strm << std::chrono::duration_cast<std::chrono::minutes>(remaining).count() + 1;
        }
        else
          strm << "?";
      }
    }
    break;

    
  }

  return strm.str();
}

void SmartRain::DrawLCD()
{
  m_lcd.PrintLine(0, GetStatusLine(0).c_str());
  m_lcd.PrintLine(1, GetStatusLine(1).c_str());
  m_lcd.PrintLine(2, GetStatusLine(2).c_str());
  m_lcd.PrintLine(3, GetStatusLine(3).c_str());
}

void SmartRain::UpdateWeatherInfo()
{
  static time_t lastCollected = 0;

  time_t now = time(0);

  // Collect every hour.
  if (lastCollected == 0 || (now - lastCollected) > 3600)
  {
    lastCollected = now;

    auto wd = m_ws.GetData();
    if (wd.IsGood())
    {
      m_weather.status = wd.GetStatus();
      m_weather.humidity = wd.GetHumidity();
      m_weather.pressure = wd.GetPressure();
      m_weather.windspeed = wd.GetWindSpeed();
      m_weather.temperature = wd.GetMaxTemp();

      std::stringstream ss;
      ss << m_weather.status;
      ss << " [H:" << m_weather.humidity << "%, P:";
      ss << m_weather.pressure << ", W:";
      ss << m_weather.windspeed << ", T:";
      ss << util::FahrenheitFromKelvin(m_weather.temperature) << "]";

      m_log.WriteEvent(ss.str());

      if (m_weather.IsRaining())
      {
        std::lock_guard<std::mutex> lk(m_mux);
        m_settings.resetLastRain();
      }
    }
    else
      m_log.WriteEvent("Unable to update weather information.");
  }
}

bool SmartRain::AbortRun(std::string& reason)
{
  if (!m_settings.getEnabled())
  {
    reason = "Sprinkler run aborted, system disabled.";
    return true;
  }

  // Disable sprinklers if current precipitation.
  if (m_weather.IsRaining())
  {
    reason = "Sprinkler run aborted due to precipitation.";
    return true;
  }

  // Disable sprinklers if precipitation within last 48 hours.
  int lastRainDays = m_settings.getLastRainDayCount();
  
  if (lastRainDays <= 3)
  {
    reason = "Sprinkler run aborted due to precipitation within the last 3 days.";
    return true;
  }

  // Disable sprinklers if too windy
  if (m_weather.IsWindy())
  {
    reason = "Sprinkler run aborted due to high winds.";
    return true;
  }

  // Disable sprinklers if too cold
  if (m_weather.IsCold())
  {
    reason = "Sprinkler run aborted due to low temperatures.";
    return true;
  }

  return false;
}

void SmartRain::ProcessSchedule()
{
  auto currentTime = std::chrono::system_clock::now();

  std::chrono::time_point<std::chrono::system_clock> sta0_start;
  std::chrono::time_point<std::chrono::system_clock> sta1_start;
  std::chrono::time_point<std::chrono::system_clock> sta2_start;
  std::chrono::time_point<std::chrono::system_clock> sta3_start;
  std::chrono::time_point<std::chrono::system_clock> stop;

  {
    std::lock_guard<std::mutex> lk(m_mux);
    auto midnight = std::chrono::system_clock::from_time_t(util::TimeFromHourMin("00:00"));
    sta0_start = midnight + std::chrono::minutes(m_settings.getStartTime());
    sta1_start = sta0_start + std::chrono::minutes(m_settings.getRunTime(0));
    sta2_start = sta1_start + std::chrono::minutes(m_settings.getRunTime(1));
    sta3_start = sta2_start + std::chrono::minutes(m_settings.getRunTime(2));
    stop = sta3_start + std::chrono::minutes(m_settings.getRunTime(3));
  }

  static std::string sReason = "";
  std::string reason;

  // Turn on sprinkler if necessary
  if (m_manualRunStation == 0 || (m_manualRunStation == -1 && currentTime >= sta0_start && currentTime < sta1_start))
  {
    if (!IsStationOn(0))
    {
      if (AbortRun(reason))
      {
        if (reason != sReason)
        {
          sReason = reason;
          m_log.WriteEvent(reason);
        }
        return;
      }

      int remainingMin = std::chrono::duration_cast<std::chrono::minutes>(sta1_start - std::chrono::system_clock::now()).count();
      std::cout << "Running station 0\n";
      StartStation(0, remainingMin);
    }
  }
  else if (m_manualRunStation == 1 || (m_manualRunStation == -1 && currentTime >= sta1_start && currentTime < sta2_start))
  {
    if (!IsStationOn(1))
    {
      if (AbortRun(reason))
      {
        if (reason != sReason)
        {
          sReason = reason;
          m_log.WriteEvent(reason);
        }
        return;
      }

      int remainingMin = std::chrono::duration_cast<std::chrono::minutes>(sta2_start - std::chrono::system_clock::now()).count();
      std::cout << "Running station 1\n";
      StartStation(1, remainingMin);
    }
  }
  else if (m_manualRunStation == 2 || (m_manualRunStation == -1 && currentTime >= sta2_start && currentTime < sta3_start))
  {
    if (!IsStationOn(2))
    {
      if (AbortRun(reason))
      {
        if (reason != sReason)
        {
          sReason = reason;
          m_log.WriteEvent(reason);
        }
        return;
      }
      
      int remainingMin = std::chrono::duration_cast<std::chrono::minutes>(sta3_start - std::chrono::system_clock::now()).count();
      std::cout << "Running station 2\n";
      StartStation(2, remainingMin);
    }
  }
  else if (m_manualRunStation == 3 || (m_manualRunStation == -1 && currentTime >= sta3_start && currentTime < stop))
  {
    if (!IsStationOn(3))
    {
      if (AbortRun(reason))
      {
        if (reason != sReason)
        {
          sReason = reason;
          m_log.WriteEvent(reason);
        }
        return;
      }

      int remainingMin = std::chrono::duration_cast<std::chrono::minutes>(stop - std::chrono::system_clock::now()).count();
      std::cout << "Running station 3\n";
      StartStation(3, remainingMin);
    }
  }
}

std::string SmartRain::LoadWebPage()
{
  std::ifstream strm(util::GetRunningProcessPath() + "main.html");
  std::string result;
  while(!strm.eof())
  {
    std::string line;
    std::getline(strm, line);
    result += line;
  }

  auto midnight = std::chrono::system_clock::from_time_t(util::TimeFromHourMin("00:00"));

  // Synchronize state
  //
  // Durations
  result.replace(result.find("[dur1]"), 6, std::to_string(m_settings.getRunTime(0)));
  result.replace(result.find("[dur2]"), 6, std::to_string(m_settings.getRunTime(1)));
  result.replace(result.find("[dur3]"), 6, std::to_string(m_settings.getRunTime(2)));
  result.replace(result.find("[dur4]"), 6, std::to_string(m_settings.getRunTime(3)));
  
  // Status
  result.replace(result.find("[stat1]"), 7, m_runningStation == 0 ? "On" : "Off");
  result.replace(result.find("[stat2]"), 7, m_runningStation == 1 ? "On" : "Off");
  result.replace(result.find("[stat3]"), 7, m_runningStation == 2 ? "On" : "Off");
  result.replace(result.find("[stat4]"), 7, m_runningStation == 3 ? "On" : "Off");

  // Start time
  auto startTime = midnight + std::chrono::minutes(m_settings.getStartTime());
  result.replace(result.find("[strTime]"), 9, util::StrFromTimePoint(startTime, "%H:%M"));

  // Enabled disabled
  result.replace(result.find("[disAll]"), 8, m_settings.getEnabled() ? "" : "checked");

  // Update manual override checks
  result.replace(result.find("[m-off]"), 7, m_manualRunStation == -1 ? "checked" : "");
  result.replace(result.find("[man-1]"), 7, m_manualRunStation == 0 ? "checked" : "");
  result.replace(result.find("[man-2]"), 7, m_manualRunStation == 1 ? "checked" : "");
  result.replace(result.find("[man-3]"), 7, m_manualRunStation == 2 ? "checked" : "");
  result.replace(result.find("[man-4]"), 7, m_manualRunStation == 3 ? "checked" : "");
    
  // Weather
  result.replace(result.find("[weather]"), 9, m_weather.status);
  result.replace(result.find("[temp]"), 6, std::to_string((int)util::FahrenheitFromKelvin(m_weather.temperature)));
  result.replace(result.find("[humd]"), 6, std::to_string((int)m_weather.humidity));
  result.replace(result.find("[wind]"), 6, std::to_string((int)m_weather.windspeed));
  result.replace(result.find("[rain]"), 6, std::to_string(m_settings.getLastRainDayCount()));

  return result;
}

void SmartRain::HandleWebRequest(http::request<http::dynamic_body>& req, http::response<http::dynamic_body>& resp)
{
  auto reqMethod = req.method();

  if (reqMethod == boost::beast::http::verb::get)
    std::cout << "\nVERB: get\n";
  else if (reqMethod == boost::beast::http::verb::post)
    std::cout << "VERB: post\n";

  std::cout << "TARGET: " << req.target() << "\n";

  if (reqMethod == boost::beast::http::verb::get)
  {
    std::string target = req.target().to_string();
    resp.set(http::field::content_type, "text/html");

    if (target.find("/process_form?") == 0)
    {
      ProcessForm(target.substr(14));
      // Redirect back to main page.
      beast::ostream(resp.body()) << "<HTML><meta http-equiv=\"refresh\" content=\"0; URL=\'http://zero.local\'\"/></HTML>";
    }
    else
      beast::ostream(resp.body()) << LoadWebPage();
  }
}

void SmartRain::ProcessForm(const std::string& args)
{
  auto value = util::ParseRequest(args, "duration1");
  if (!value.empty())
  {
    auto iVal = std::atol(value.c_str());
    m_settings.setRunTime(0, iVal);
  }
  
  value = util::ParseRequest(args, "duration2");
  if (!value.empty())
  {
    auto iVal = std::atol(value.c_str());
    m_settings.setRunTime(1, iVal);
  }
  
  value = util::ParseRequest(args, "duration3");
  if (!value.empty())
  {
    auto iVal = std::atol(value.c_str());
    m_settings.setRunTime(2, iVal);
  }
  
  value = util::ParseRequest(args, "duration4");
  if (!value.empty())
  {
    auto iVal = std::atol(value.c_str());
    m_settings.setRunTime(3, iVal);
  }

  value = util::ParseRequest(args, "startTime");
  if (!value.empty())
  {
    auto midnight = std::chrono::system_clock::from_time_t(util::TimeFromHourMin("00:00"));
    auto tp = std::chrono::system_clock::from_time_t(util::TimeFromHourMin(value.c_str()));
    auto delta = tp-midnight;
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(delta).count();
    m_settings.setStartTime((int)minutes);
  }

  bool enabled = true;
  value = util::ParseRequest(args, "disableAll");
  if (!value.empty())
    enabled = (value != "on");

  m_settings.setEnabled(enabled);

  value = util::ParseRequest(args, "manualOverride");
  if (!value.empty())
  {
    int oldManualStation = m_manualRunStation;

    if (value == "man1")
    {
      m_manualRunStation = 0;
    }
    else if (value == "man2")
    {
      m_manualRunStation = 1;
    }
    else if (value == "man3")
    {
      m_manualRunStation = 2;
    }
    else if (value == "man4")
    {
      m_manualRunStation = 3;
    }
    else
    {
      m_manualRunStation = -1;
    }
    
    if (oldManualStation != m_manualRunStation)
    {
      StopAllStations();
    }
  }    
    
  SaveSettings();
  ProcessSchedule();
  DrawLCD();
}