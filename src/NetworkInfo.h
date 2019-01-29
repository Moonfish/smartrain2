#ifndef NETWORKINFO_H
#define NETWORKINFO_H

#include <string>
#include <set>

class NetworkInfo
{
  public:
    NetworkInfo();
    virtual ~NetworkInfo();

    typedef std::set<std::string> ADDRESSES;
    ADDRESSES GetConnections();
    std::string GetIP(int connection);

  private:
};

#endif // NETWORKINFO_H
