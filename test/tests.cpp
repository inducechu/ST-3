// Copyright 2021 GHA Test Team

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "TimedDoor.h"
#include <thread>
#include <chrono>

// Mock classes for interface testing (extended in tests.cpp per requirements)
class MockDoor : public Door {
 public:
  MOCK_METHOD(void, lock, (), (override));
  MOCK_METHOD(void, unlock, (), (override));
  MOCK_METHOD(bool, isDoorOpened, (), (override));
};

class MockTimerClient : public TimerClient {
 public:
  MOCK_METHOD(void, Timeout, (), (override));
};

class MockTimedDoor : public TimedDoor {
 public:
  explicit MockTimedDoor(int timeout) : TimedDoor(timeout) {}
  MOCK_METHOD(bool, isDoorOpened, (), (override));
  MOCK_METHOD(void, throwState, (), (override));
};

class TimedDoorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    door = new TimedDoor(0); // Zero timeout for fast tests
  }

  void TearDown() override {
    delete door;
    door = nullptr;
  }

  TimedDoor *door;
};

TEST_F(TimedDoorTest, DoorIsInitiallyClosed) {
  EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, LockClosesDoor) {
  door->unlock();
  EXPECT_TRUE(door->isDoorOpened());
  door->lock();
  EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, UnlockOpensDoor) {
  door->lock();
  EXPECT_FALSE(door->isDoorOpened());
  door->unlock();
  EXPECT_TRUE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, GetTimeoutReturnsCorrectValue) {
  TimedDoor customDoor(42);
  EXPECT_EQ(customDoor.getTimeOut(), 42);
  EXPECT_EQ(door->getTimeOut(), 0);
}

TEST_F(TimedDoorTest, ThrowStateThrowsWhenDoorOpen) {
  door->unlock();
  EXPECT_THROW(door->triggerTimeoutForTest(), DoorTimeoutException);
}

TEST_F(TimedDoorTest, ThrowStateDoesNotThrowWhenDoorClosed) {
  door->lock();
  EXPECT_NO_THROW(door->triggerTimeoutForTest());
}

TEST_F(TimedDoorTest, ExceptionHasCorrectMessage) {
  door->unlock();
  try {
    door->triggerTimeoutForTest();
    FAIL() << "Expected DoorTimeoutException";
  } catch (const DoorTimeoutException &e) {
    EXPECT_STREQ(e.what(), "Door remained open past timeout");
  }
}

TEST_F(TimedDoorTest, TimerCallsTimeoutOnClient) {
  MockTimerClient mockClient;
  EXPECT_CALL(mockClient, Timeout()).Times(1);
  door->registerTimerForTest(0, &mockClient);
}

TEST_F(TimedDoorTest, MockDoorLockIsCalled) {
  MockDoor mockDoor;
  EXPECT_CALL(mockDoor, lock()).Times(1);
  mockDoor.lock();
}

TEST_F(TimedDoorTest, MockDoorUnlockIsCalled) {
  MockDoor mockDoor;
  EXPECT_CALL(mockDoor, unlock()).Times(1);
  mockDoor.unlock();
}

TEST_F(TimedDoorTest, MockDoorIsDoorOpenedIsCalled) {
  MockDoor mockDoor;
  EXPECT_CALL(mockDoor, isDoorOpened()).WillOnce(::testing::Return(true));
  EXPECT_TRUE(mockDoor.isDoorOpened());
}

TEST_F(TimedDoorTest, MockTimerClientTimeoutIsCalled) {
  MockTimerClient mockClient;
  EXPECT_CALL(mockClient, Timeout()).Times(1);
  mockClient.Timeout();
}

TEST_F(TimedDoorTest, AdapterCallsThrowStateWhenDoorOpen) {
  MockTimedDoor mockDoor(5);
  EXPECT_CALL(mockDoor, isDoorOpened()).WillOnce(::testing::Return(true));
  EXPECT_CALL(mockDoor, throwState()).Times(1).WillOnce(::testing::Invoke([]() {
    throw DoorTimeoutException("Door remained open past timeout");
  }));
  EXPECT_THROW(mockDoor.triggerTimeoutForTest(), DoorTimeoutException);
}

TEST_F(TimedDoorTest, AdapterDoesNotCallThrowStateWhenDoorClosed) {
  MockTimedDoor mockDoor(5);
  EXPECT_CALL(mockDoor, isDoorOpened()).WillOnce(::testing::Return(false));
  EXPECT_CALL(mockDoor, throwState()).Times(0);
  EXPECT_NO_THROW(mockDoor.triggerTimeoutForTest());
}
