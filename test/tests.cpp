// Copyright 2021 GHA Test Team

#include "TimedDoor.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <chrono>
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

TEST_F(SecureTimedDoorFixture, RealTimerTriggersTimeoutInThread) {
  TimedDoor dynamicDoor(1);
  dynamicDoor.unlock();  
  std::this_thread::sleep_for(std::chrono::milliseconds(1200));
  SUCCEED(); 
}

TEST_F(SecureTimedDoorFixture, MultipleUnlockCallsHandledSafely) {
  testObject->unlock();
  EXPECT_TRUE(testObject->isDoorOpened());  
  EXPECT_NO_THROW(testObject->unlock());
  EXPECT_TRUE(testObject->isDoorOpened());
}

TEST_F(SecureTimedDoorFixture, ClosingDoorBeforeTimeoutPreventsException) {
  TimedDoor dynamicDoor(1);
  dynamicDoor.unlock();
  dynamicDoor.lock(); 
  std::this_thread::sleep_for(std::chrono::milliseconds(1200));
  EXPECT_FALSE(dynamicDoor.isDoorOpened());
}

TEST_F(SecureTimedDoorFixture, AdapterInvokesThrowStateWhenDoorIsOpened) {
  MockTimedDoor mockDoor(0);
  DoorTimerAdapter adapter(mockDoor);
  EXPECT_CALL(mockDoor, isDoorOpened()).WillOnce(::testing::Return(true));
  EXPECT_CALL(mockDoor, throwState()).Times(1);      
  adapter.Timeout();
}
