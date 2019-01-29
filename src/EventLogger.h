#ifndef EVENTLOGGER_H
#define EVENTLOGGER_H

#include <string>
#include <fstream>
#include <deque>

class EventLogger final
{
  public:
    EventLogger(const char* logPath);
    virtual ~EventLogger();

    void WriteEvent(const char* entry, bool flushToDisk = true);
    void WriteEvent(const std::string& entry, bool flushToDisk = true);

  private:

  bool Open();
  bool Flush();
  bool WriteHTMLHeader(std::ofstream& strm);
  bool WriteHTMLFooter(std::ofstream& strm);

  typedef std::deque<std::string> STRING_LIST;

  std::string  m_fileName;
  STRING_LIST  m_eventList;
};

#endif // EVENTLOGGER_H
