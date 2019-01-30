# SmartRain2
This is the second version of a simple homebrew sprinkler control system.
The implementation is entirely in C++ with the only dependency on asio::boost (header only) for the webserver.
The application is intended to run on a raspberrypi or beaglebone running linux.
It periodically queries weather information and uses this to determine whether to run the stations (precipitation, wind and temperature are considered).
The application also exposes a simple form based webserver for control and access to current information.
