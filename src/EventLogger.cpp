#include <fstream>
#include <iostream>
#include <string>
#include <time.h>
#include "EventLogger.h"

const std::string header = R"r(<HTML>
<HEAD>
<TITLE>Event Log</TITLE>
<meta name="viewport" content="width=device-width; initial-scale=1.0; maximum-scale=1.0;"/>
</HEAD>
<BODY>
<PRE>)r";

EventLogger::EventLogger(const char* logFileName) : m_fileName(logFileName)
{
  Open();
}

EventLogger::~EventLogger()
{
  Flush();
}

void EventLogger::WriteEvent(const std::string& entry, bool flushToDisk /* = true */)
{
  WriteEvent(entry.c_str(), flushToDisk);
}

void EventLogger::WriteEvent(const char* entry, bool flushToDisk /* = true */)
{
  time_t now = time(nullptr);
  auto timeInfo = localtime(&now);

  char fmt[64]={0};
  strftime(fmt, 64, "%m/%d %R] ", timeInfo);
  std::string out = "[";
  out += fmt;
  out += entry;

  m_eventList.push_front(out);

  // Limit log size to 1000 entries.
  if (m_eventList.size() > 1000)
    m_eventList.pop_back();

  if (flushToDisk)
    Flush();
}

bool EventLogger::Open()
{
  std::cout << m_fileName << std::endl;
  std::ifstream strm(m_fileName);

  if (!strm.good())
    return false;

  strm.seekg(header.size());

  while (!strm.eof())
  {
    std::string line;
    std::getline(strm, line);
    if (line.find("[") != std::string::npos)
      m_eventList.push_back(line);
  }

  return true;
}

bool EventLogger::Flush()
{
  std::ofstream strm(m_fileName);
  if (!strm.good())
    return false;

  WriteHTMLHeader(strm);

  for (auto e : m_eventList)
    strm << e << std::endl;

  WriteHTMLFooter(strm);

  strm.close();

  return true;
}

bool EventLogger::WriteHTMLHeader(std::ofstream& strm)
{
  if (!strm.good())
    return false;

  strm << header << std::endl;

  return true;
}

bool EventLogger::WriteHTMLFooter(std::ofstream& strm)
{
  if (!strm.good())
    return false;

  std::string footer = R"r(</PRE>
</BODY>
</HTML>)r";

  strm << footer << std::endl;

  return true;
}
