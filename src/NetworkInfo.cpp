#include "NetworkInfo.h"
#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

NetworkInfo::NetworkInfo()
{
  //ctor
}

NetworkInfo::~NetworkInfo()
{
  //dtor
}

NetworkInfo::ADDRESSES NetworkInfo::GetConnections()
{
  NetworkInfo::ADDRESSES out;

  struct ifaddrs * ifAddrStruct=NULL;
  void * tmpAddrPtr=NULL;

  getifaddrs(&ifAddrStruct);

  for (struct ifaddrs* ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next)
  {
      if (!ifa->ifa_addr)
          continue;

      if (ifa->ifa_addr->sa_family == AF_INET)
      { // check it is IP4
          // is a valid IP4 Address
          tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
          char addressBuffer[INET_ADDRSTRLEN];
          inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
          out.insert(addressBuffer);
      } else if (ifa->ifa_addr->sa_family == AF_INET6)
      { // check it is IP6
          // is a valid IP6 Address
          tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
          char addressBuffer[INET6_ADDRSTRLEN];
          inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
          out.insert(addressBuffer);
      }
  }

  if (ifAddrStruct!=NULL)
    freeifaddrs(ifAddrStruct);

  return out;
}

std::string NetworkInfo::GetIP(int connection)
{
  ADDRESSES cnx = GetConnections();

  ADDRESSES::iterator it = cnx.begin();

  return *(++it);
}
