// Copyright 2021 GHA Test Team

#include "TimedDoor.h"
#include <chrono>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>

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
  explicit MockTimedDoor(int timeoutVal) : TimedDoor(timeoutVal) {}
  MOCK_METHOD(bool, isDoorOpened, (), (override));
  MOCK_METHOD(void, throwState, (), (override));
};

class SecureTimedDoorFixture : public ::testing::Test {
protected:
  void SetUp() override { testObject = new TimedDoor(0); }

  void TearDown() override {
    delete testObject;
    testObject = nullptr;
  }

  TimedDoor *testObject;
};

TEST_F(SecureTimedDoorFixture, InitialStateIsLocked) {
  EXPECT_FALSE(testObject->isDoorOpened());
}

TEST_F(SecureTimedDoorFixture, TransitionFromUnlockToLock) {
  testObject->unlock();
  EXPECT_TRUE(testObject->isDoorOpened());
  testObject->lock();
  EXPECT_FALSE(testObject->isDoorOpened());
}

TEST_F(SecureTimedDoorFixture, TransitionFromLockToUnlock) {
  testObject->lock();
  EXPECT_FALSE(testObject->isDoorOpened());
  testObject->unlock();
  EXPECT_TRUE(testObject->isDoorOpened());
}

TEST_F(SecureTimedDoorFixture, ValidateTimeoutRetrieval) {
  TimedDoor alternativeDoor(15);
  EXPECT_EQ(alternativeDoor.getTimeOut(), 15);
  EXPECT_EQ(testObject->getTimeOut(), 0);
}

TEST_F(SecureTimedDoorFixture, FireTimeoutOnUnlockedState) {
  testObject->unlock();
  EXPECT_THROW(testObject->triggerTimeoutForTest(), DoorTimeoutException);
}

TEST_F(SecureTimedDoorFixture, FireTimeoutOnLockedState) {
  testObject->lock();
  EXPECT_NO_THROW(testObject->triggerTimeoutForTest());
}

TEST_F(SecureTimedDoorFixture, VerifyExceptionStringContent) {
  testObject->unlock();
  try {
    testObject->triggerTimeoutForTest();
    FAIL() << "DoorTimeoutException was expected but not thrown.";
  } catch (const DoorTimeoutException &exception) {
    EXPECT_STREQ(exception.what(),
                 "Security violation: door open limit exceeded");
  }
}

TEST_F(SecureTimedDoorFixture, TimerDispatchesTimeoutEvent) {
  MockTimerClient mockClientInstance;
  EXPECT_CALL(mockClientInstance, Timeout()).Times(1);
  testObject->registerTimerForTest(0, &mockClientInstance);
}

TEST_F(SecureTimedDoorFixture, InvokeMockDoorLockInterface) {
  MockDoor dummyDoor;
  EXPECT_CALL(dummyDoor, lock()).Times(1);
  dummyDoor.lock();
}

TEST_F(SecureTimedDoorFixture, InvokeMockDoorUnlockInterface) {
  MockDoor dummyDoor;
  EXPECT_CALL(dummyDoor, unlock()).Times(1);
  dummyDoor.unlock();
}

TEST_F(SecureTimedDoorFixture, ReadStateFromMockDoor) {
  MockDoor dummyDoor;
  EXPECT_CALL(dummyDoor, isDoorOpened()).WillOnce(::testing::Return(true));
  EXPECT_TRUE(dummyDoor.isDoorOpened());
}

TEST_F(SecureTimedDoorFixture, DirectlyInvokeMockClientTimeout) {
  MockTimerClient dummyClient;
  EXPECT_CALL(dummyClient, Timeout()).Times(1);
  dummyClient.Timeout();
}

TEST_F(SecureTimedDoorFixture, AdapterTriggersExceptionIfOpened) {
  MockTimedDoor dynamicMockDoor(8);
  EXPECT_CALL(dynamicMockDoor, isDoorOpened())
      .WillOnce(::testing::Return(true));
  EXPECT_CALL(dynamicMockDoor, throwState())
      .Times(1)
      .WillOnce(::testing::Invoke([]() {
        throw DoorTimeoutException(
            "Security violation: door open limit exceeded");
      }));
  EXPECT_THROW(dynamicMockDoor.triggerTimeoutForTest(), DoorTimeoutException);
}

TEST_F(SecureTimedDoorFixture, AdapterBypassesExceptionIfClosed) {
  MockTimedDoor dynamicMockDoor(8);
  EXPECT_CALL(dynamicMockDoor, isDoorOpened())
      .WillOnce(::testing::Return(false));
  EXPECT_CALL(dynamicMockDoor, throwState()).Times(0);
  EXPECT_NO_THROW(dynamicMockDoor.triggerTimeoutForTest());
}
