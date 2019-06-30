#pragma once

#include <iostream>
#include <string>
#include <mutex>

class Settings
{
  public:
  Settings();

  static constexpr int numStations = 4;

  // Persistence
  bool Load(std::istream&);
  bool Save(std::ostream&);

  bool getEnabled();
  void setEnabled(bool enabled);
  int getStartTime();
  int getStartTime2();
  void setStartTime(int minutes);
  void setStartTime2(int minutes);
  int getRunTime(int station);
  void setRunTime(int station, int minutes);

  // Number of days since last rain  
  int getLastRainDayCount();
  // Reset last rain to now.
  void resetLastRain();

  private:

  // Persistent members
  std::mutex mux;
  bool enabled;
  int startTime;   // In minutes from midnight
  int startTime2;  // In minutes from midnight
  int runTimes[numStations]; // In minutes
  std::chrono::time_point<std::chrono::system_clock> lastRain;    
};

