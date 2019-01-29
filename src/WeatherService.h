#include <iostream>
#include <string>

class WeatherData
{
  WeatherData() : m_temp(0), m_maxTemp(0), m_humidity(0), m_pressure(0), m_windSpeed(0), m_good(false)
  {
  }

  WeatherData(double temp, double maxTemp, double humidity, double pressure, double windSpeed, const char* status) :
    m_temp(temp), m_maxTemp(maxTemp), m_humidity(humidity), m_pressure(pressure), m_windSpeed(windSpeed),
    m_good(true), m_status(status)
  {
  }

  friend class WeatherService;

  double m_temp;
  double m_maxTemp;
  double m_humidity;
  double m_pressure;
  double m_windSpeed;
  bool   m_good;
  std::string m_status;


  public:

  bool   IsGood(){return m_good;}
  double GetTemp(){return m_temp;}
  double GetMaxTemp(){return m_maxTemp;}
  double GetHumidity(){return m_humidity;}
  double GetPressure(){return m_pressure;}
  double GetWindSpeed(){return m_windSpeed;}
  const char* GetStatus(){return m_status.c_str();}
};

class WeatherService
{
public:

  WeatherService();
  ~WeatherService();
  WeatherData GetData();

private:

  static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata);
  size_t OnGetDataFromServer(char* ptr, size_t size, size_t nmemb);

  std::string m_result;
};


