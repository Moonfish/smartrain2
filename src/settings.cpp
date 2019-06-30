#include "settings.h"

constexpr int sCurrentVersion = 2;

Settings::Settings()
{
  // Defaults
  this->enabled = true;
  this->lastRain = std::chrono::system_clock::now() - std::chrono::hours(4*24);
  this->startTime = 6*60; // 6:00 AM
  for(auto& dur : this->runTimes)
    dur = 10; // 10 minutes
}

bool Settings::Load(std::istream& strm)
{
  if (strm.bad())
    return false;

  std::lock_guard<std::mutex> lk(mux);

  int version = sCurrentVersion;
  
  strm >> version;
  if (version > sCurrentVersion)
    return false;
    
  strm >> this->enabled;
  
  time_t tp;
  strm >> tp;
  this->lastRain = std::chrono::system_clock::from_time_t(tp);
  
  strm >> this->startTime;
  strm >> this->startTime2;
  
  for(auto& dur : this->runTimes)
    strm >> dur;

  return true;
}

bool Settings::Save(std::ostream& strm)
{
  if (strm.bad())
    return false;

  std::lock_guard<std::mutex> lk(mux);

  strm << sCurrentVersion << "\n";
  strm << this->enabled << "\n";
  strm << std::chrono::system_clock::to_time_t(this->lastRain) << "\n";
  strm << this->startTime << "\n";
  strm << this->startTime2 << "\n";
  for(auto& dur : this->runTimes)
    strm << dur << "\n";

  return true;
}

bool Settings::getEnabled() 
{
  std::lock_guard<std::mutex> lk(mux);
  return enabled;
}

void Settings::setEnabled(bool _enabled)
{
  std::lock_guard<std::mutex> lk(mux);
  enabled = _enabled;
}

int Settings::getStartTime()
{
  std::lock_guard<std::mutex> lk(mux);
  return startTime;
}

int Settings::getStartTime2()
{
  std::lock_guard<std::mutex> lk(mux);
  return startTime2;
}

void Settings::setStartTime(int minutes)
{
  std::lock_guard<std::mutex> lk(mux);
  startTime = minutes;
}

void Settings::setStartTime2(int minutes)
{
  std::lock_guard<std::mutex> lk(mux);
  startTime2 = minutes;
}

int Settings::getRunTime(int station)
{
  std::lock_guard<std::mutex> lk(mux);
  if (station < 0 || station > numStations-1)
    return 0;

  return runTimes[station];
}

void Settings::setRunTime(int station, int minutes)
{
  std::lock_guard<std::mutex> lk(mux);
  if (station >= 0 || station < numStations-1)
    runTimes[station] = minutes > 0 ? minutes : 0;  
}

int Settings::getLastRainDayCount()
{
  std::lock_guard<std::mutex> lk(mux);
  auto delta = std::chrono::system_clock::now() - lastRain;
  
  auto hours = std::chrono::duration_cast<std::chrono::hours>(delta);
  
  return hours.count()/24;
}

void Settings::resetLastRain()
{
  std::lock_guard<std::mutex> lk(mux);
  lastRain = std::chrono::system_clock::now();
}
