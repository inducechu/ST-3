// Copyright 2021 GHA Test Team

#ifndef INCLUDE_TIMEDDOOR_H_
#define INCLUDE_TIMEDDOOR_H_

#include <stdexcept>
#include <thread>

class DoorTimerAdapter;
class Timer;
class Door;
class TimedDoor;

class DoorTimeoutException : public std::runtime_error {
 public:
  explicit DoorTimeoutException(const char *msg) : std::runtime_error(msg) {}
};

class TimerClient {
 public:
  virtual void Timeout() = 0;
};

class Door {
 public:
  virtual void lock() = 0;
  virtual void unlock() = 0;
  virtual bool isDoorOpened() = 0;
};

class DoorTimerAdapter : public TimerClient {
 private:
  TimedDoor &door;

 public:
  explicit DoorTimerAdapter(TimedDoor &);
  void Timeout();
};

class TimedDoor : public Door {
 private:
  DoorTimerAdapter *adapter;
  Timer *timer;
  std::thread timerThread;
  int iTimeout;
  bool isOpened;

 public:
  explicit TimedDoor(int);
  ~TimedDoor();
  bool isDoorOpened() override;
  void unlock() override;
  void lock() override;
  int getTimeOut() const;
  virtual void throwState();
  Timer *getTimer() const { return timer; }
  void triggerTimeoutForTest();
  void registerTimerForTest(int timeout, TimerClient *client);
};

class Timer {
  TimerClient *client;
  void sleep(int);

 public:
  void tregister(int, TimerClient *);
};

#endif // INCLUDE_TIMEDDOOR_H_
