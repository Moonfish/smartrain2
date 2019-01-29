#include "i2c-dev.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <exception>
#include <system_error>
#include <chrono>
#include <thread>
#include "LCD.h"

LCDPanel::LCDPanel(const char* device, int i2cAddress) : m_fd(0)
{
  // Powerup delay
  usleep(5000);

  m_fd = ::open(device, O_RDWR);

  if (m_fd < 0)
  {
    std::cout << "Could not open specified I2C dev file\n";
    throw std::system_error();
  }

  int chk = ::ioctl(m_fd, I2C_SLAVE, i2cAddress);

  if (chk <  0)
  {
    std::cout << "Could not access I2C dev file\n";
    throw std::system_error();
  }

  // Reset sequence
  usleep(1000);
  WriteLowerNib(0x03);
  usleep(5000);
  WriteLowerNib(0x03);
  usleep(5000);
  WriteLowerNib(0x03);
  usleep(2000);
  WriteLowerNib(0x02);
  usleep(1000);

  // Multi-line
  WriteByte(0x20|0x08);
  // Display on
  WriteByte(0x04|0x08);

  Clear();

  std::cout << "LCD initialized\n";
}

LCDPanel::~LCDPanel()
{
  Clear();

  if (m_fd >= 0)
    ::close(m_fd);
}

void LCDPanel::WriteByte(std::uint8_t val, Mode mode /* = Mode::command */)
{
  WriteUpperNib(val, mode);
  WriteLowerNib(val, mode);
}

void LCDPanel::WriteLowerNib(std::uint8_t val, Mode mode)
{
  std::uint8_t modeBit = (mode == Mode::command) ? 0 : 1;
  constexpr std::uint8_t BACK_LIGHT = 0x8;

  std::uint8_t byte = modeBit | ((val << 4) & 0xf0) | BACK_LIGHT;
  i2c_smbus_write_byte(m_fd, byte);
  Pulse(byte);
}

void LCDPanel::WriteUpperNib(std::uint8_t val, Mode mode)
{
  std::uint8_t modeBit = (mode == Mode::command) ? 0 : 1;
  constexpr std::uint8_t BACK_LIGHT = 0x8;

  std::uint8_t byte = modeBit | (val & 0xf0) | BACK_LIGHT;
  i2c_smbus_write_byte(m_fd, byte);
  Pulse(byte);
}

void LCDPanel::Pulse(std::uint8_t val)
{
  constexpr std::uint8_t E_BIT = 0x4;

  usleep(60);
  std::uint8_t byte = val | E_BIT;
  i2c_smbus_write_byte(m_fd, byte);
  usleep(60);
  byte = (val & ~E_BIT);
  i2c_smbus_write_byte(m_fd, byte);
  usleep(60);
}

void LCDPanel::Clear()
{
  if (m_fd <= 0)
    return;

  WriteByte(0x01);  
}

void LCDPanel::PrintLine(int lineNum, const char* text)
{
  if (m_fd <= 0)
    return;

  constexpr int PANEL_COLUMNS = 20;
  constexpr int PANEL_ROWS = 4;

   if (lineNum < 0 || lineNum > PANEL_ROWS)
    throw std::invalid_argument("lineNum");

  SetLine(lineNum);
  
  // Truncate if necessary
  size_t strLen = ::strlen(text);
  if (strLen > PANEL_COLUMNS)
    strLen = PANEL_COLUMNS;
  
  for (size_t i = 0; i < strLen; i++)
    WriteByte((std::uint8_t)text[i], Mode::data);

  for (size_t r = 0; r < PANEL_COLUMNS-strLen; r++)
    WriteByte((std::uint8_t)' ', Mode::data);
}

void LCDPanel::SetLine(int line)
{
  // Display memory offsets
  constexpr std::uint8_t code[] = {0x80, 0xC0, 0x94, 0xD4};

  // Position cursor at beginning of line.
  WriteByte(code[line]);
}


