/*
 * RelayController.h
 *
 *  Created on: Dec 20, 2014
 *      Author: russell
 */

#ifndef RELAYCONTROLLER_H_
#define RELAYCONTROLLER_H_

class RelayController
{
public:
  RelayController(int relayCount, const char* device, int i2cAddress);
  ~RelayController();

  void RelayOn(int relay);
  bool IsRelayOn(int relay);
  void RelaysOff();
  void Test();

private:
  volatile int m_currentlyOnValve;
  int          m_fd;
  int          m_relayCount;
};

#endif /* RELAYCONTROLLER_H_ */
