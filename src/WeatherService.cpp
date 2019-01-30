#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "WeatherService.h"

namespace pt = boost::property_tree;
using tcp = boost::asio::ip::tcp;       
namespace http = boost::beast::http;   

WeatherService::WeatherService() 
{
}

WeatherService::~WeatherService()
{
}

WeatherData WeatherService::GetData()
{
  WeatherData wdBad;

  try
  { 
    auto const host("api.openweathermap.org");
    auto const port("80");
    auto const body("/data/2.5/weather?<client details>");

    boost::asio::io_context ioc;
    tcp::resolver resolver{ioc};
    tcp::socket socket{ioc};

    auto const results = resolver.resolve(host, port);
    boost::asio::connect(socket, results.begin(), results.end());

    http::request<http::string_body> req{http::verb::get, body, 11};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    http::write(socket, req);

    boost::beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(socket, buffer, res);

    std::stringstream ss;
    ss << res.body();

    boost::system::error_code ec;
    socket.shutdown(tcp::socket::shutdown_both, ec);
    
    pt::ptree root;
    pt::read_json(ss, root);

    if(root.empty())
      return wdBad; 

    std::string status = root.get<std::string>("weather..main", "<unknown>");
    double temp = root.get<double>("main.temp", 0);
    double maxTemp = root.get<double>("main.temp_max", 0);
    double humidity = root.get<double>("main.humidity", 0);
    double pressure = root.get<double>("main.pressure", 0);
    double windSpeed = root.get<double>("wind.speed", 0);
    
    WeatherData wdGood(temp, maxTemp, humidity, pressure, windSpeed, status.c_str());
    return wdGood;
  }
  catch(...)
  {
  }

  return wdBad;
}



