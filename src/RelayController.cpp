/*
 * RelayController.cpp
 *
 *  Created on: Dec 20, 2014
 *      Author: russell
 */

#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <exception>
#include <system_error>
#include "RelayController.h"
#include "util.h"

RelayController::RelayController(int relayCount, const char* device, int i2cAddress) : 
  m_currentlyOnValve(-1), m_fd(-1), m_relayCount(relayCount)
{
  // Warmup
  ::usleep(100);

  m_fd = ::open(device, O_RDWR);

  if (m_fd < 0)
    throw std::system_error();

  int chk = ::ioctl(m_fd, I2C_SLAVE, i2cAddress);

  if (chk <  0)
    throw std::system_error();

  RelaysOff();
}

RelayController::~RelayController()
{
  RelaysOff();
}

void RelayController::RelayOn(int relay)
{
  constexpr int overlapDelay = 2000; 

  if (relay < 0 || relay >= m_relayCount)
    throw std::invalid_argument("relay");

  // Already on?
  if (IsRelayOn(relay))
    return;

  if (m_currentlyOnValve == -1)
  {
    std::uint8_t bitsVal = ~(1 << relay);
    ::write(m_fd, &bitsVal, 1);
    util::msleep(50);
  }
  else
  {
    // Overlap valve on-times to avoid water pressure hammering
    std::uint8_t bitsCurrent = (1 << m_currentlyOnValve);
    std::uint8_t bitsVal = (1 << relay);
    std::uint8_t combined = ~(bitsVal | bitsCurrent);
    ::write(m_fd, &combined, 1);
    util::msleep(overlapDelay);
    bitsVal = ~bitsVal;
    ::write(m_fd, &bitsVal, 1);
    util::msleep(50);    
  }

  m_currentlyOnValve = relay;
}

bool RelayController::IsRelayOn(int relay)
{
  return (relay == m_currentlyOnValve);
}

void RelayController::RelaysOff()
{
  if (m_currentlyOnValve == -1)
    return;

  m_currentlyOnValve = -1;

  std::uint8_t val = 0xFF;

  ::write(m_fd, &val, 1);

  util::msleep(50);
}

void RelayController::Test()
{
  for (int i = 0; i < m_relayCount; ++i)
  {
    RelayOn(i);
    util::msleep(1000);
  }

}
