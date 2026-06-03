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
  explicit DoorTimeoutException(const char *error_msg)
      : std::runtime_error(error_msg) {}
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
  TimedDoor &associatedDoor;

 public:
  explicit DoorTimerAdapter(TimedDoor &targetDoor);
  void Timeout();
};

class TimedDoor : public Door {
 private:
  DoorTimerAdapter *doorAdapter;
  Timer *internalTimer;
  std::thread workerThread;
  int durationLimit;
  bool openedFlag;

 public:
  explicit TimedDoor(int timeoutSec);
  ~TimedDoor();
  bool isDoorOpened() override;
  void unlock() override;
  void lock() override;
  int getTimeOut() const;
  virtual void throwState();
  Timer *getTimer() const { return internalTimer; }
  void triggerTimeoutForTest();
  void registerTimerForTest(int timeoutVal, TimerClient *clientPtr);
};

class Timer {
  TimerClient *registeredClient;
  void sleep(int seconds);

 public:
  void tregister(int timeoutVal, TimerClient *clientPtr);
};

#endif  // INCLUDE_TIMEDDOOR_H_
