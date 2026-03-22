// Copyright 2021 GHA Test Team

#include "TimedDoor.h"
#include <chrono>
#include <thread>

DoorTimerAdapter::DoorTimerAdapter(TimedDoor &d) : door(d) {}

void DoorTimerAdapter::Timeout() {
  if (door.isDoorOpened()) {
    door.throwState();
  }
}

TimedDoor::TimedDoor(int timeout) : iTimeout(timeout), isOpened(false) {
  timer = new Timer();
  adapter = new DoorTimerAdapter(*this);
}

TimedDoor::~TimedDoor() {
  if (timerThread.joinable()) {
    timerThread.join();
  }
  delete adapter;
  delete timer;
}

bool TimedDoor::isDoorOpened() { return isOpened; }

void TimedDoor::unlock() {
  isOpened = true;
  if (timerThread.joinable()) {
    timerThread.join();
  }
  timerThread = std::thread([this]() {
    try {
      timer->tregister(iTimeout, adapter);
    } catch (const DoorTimeoutException &) {
      // Exception from timer callback - door was left open
    }
  });
}

void TimedDoor::lock() { isOpened = false; }

int TimedDoor::getTimeOut() const { return iTimeout; }

void TimedDoor::throwState() {
  throw DoorTimeoutException("Door remained open past timeout");
}

void TimedDoor::triggerTimeoutForTest() { adapter->Timeout(); }

void TimedDoor::registerTimerForTest(int timeout, TimerClient *client) {
  timer->tregister(timeout, client);
}

void Timer::sleep(int seconds) {
  std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

void Timer::tregister(int timeout, TimerClient *c) {
  client = c;
  sleep(timeout);
  if (client) {
    client->Timeout();
  }
}
