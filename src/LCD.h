#ifndef _LCD_H
#define _LCD_H

class LCDPanel
{
public:
  LCDPanel(const char* device, int i2cAddress);
  ~LCDPanel();
  
  void Clear();
  void PrintLine(int lineNum, const char* text);
  void Reset();
  
private:
  
  enum Mode{command, data};
  void WriteByte(std::uint8_t val, Mode mode = Mode::command);
  void WriteLowerNib(std::uint8_t val, Mode mode = Mode::command);
  void WriteUpperNib(std::uint8_t val, Mode mode = Mode::command);
  void SetLine(int line);
  void Pulse(std::uint8_t val);

  int              m_fd;
};

#endif

