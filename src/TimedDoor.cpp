// Copyright 2021 GHA Test Team

#include "TimedDoor.h"
#include <chrono>
#include <thread>

DoorTimerAdapter::DoorTimerAdapter(TimedDoor &targetDoor)
    : associatedDoor(targetDoor) {}

void DoorTimerAdapter::Timeout() {
  if (associatedDoor.isDoorOpened()) {
    associatedDoor.throwState();
  }
}

TimedDoor::TimedDoor(int timeoutSec)
    : durationLimit(timeoutSec), openedFlag(false) {
  internalTimer = new Timer();
  doorAdapter = new DoorTimerAdapter(*this);
}

TimedDoor::~TimedDoor() {
  if (workerThread.joinable()) {
    workerThread.join();
  }
  delete doorAdapter;
  delete internalTimer;
}

bool TimedDoor::isDoorOpened() { return openedFlag; }

void TimedDoor::unlock() {
  openedFlag = true;
  if (workerThread.joinable()) {
    workerThread.join();
  }
  workerThread = std::thread([this]() {
    try {
      internalTimer->tregister(durationLimit, doorAdapter);
    } catch (const DoorTimeoutException &) {
      // Exception from timer callback - door was left open
    }
  });
}

void TimedDoor::lock() { openedFlag = false; }

int TimedDoor::getTimeOut() const { return durationLimit; }

void TimedDoor::throwState() {
  throw DoorTimeoutException("Security violation: door open limit exceeded");
}

void TimedDoor::triggerTimeoutForTest() { doorAdapter->Timeout(); }

void TimedDoor::registerTimerForTest(int timeoutVal, TimerClient *clientPtr) {
  internalTimer->tregister(timeoutVal, clientPtr);
}

void Timer::sleep(int seconds) {
  std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

void Timer::tregister(int timeoutVal, TimerClient *clientPtr) {
  registeredClient = clientPtr;
  sleep(timeoutVal);
  if (registeredClient) {
    registeredClient->Timeout();
  }
}
