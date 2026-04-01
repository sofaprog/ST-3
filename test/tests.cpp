#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <chrono>
#include <stdexcept>
#include "TimedDoor.h"

using ::testing::Exactly;
using ::testing::AtLeast;

class TimedDoorTest : public ::testing::Test {
protected:
    TimedDoor* door;
    
    void SetUp() override {
        door = new TimedDoor(2);
    }
    void TearDown() override {
        delete door;
    }
};

TEST_F(TimedDoorTest, DoorInitiallyClosed) {
    EXPECT_FALSE(door->isDoorOpened());
}
TEST_F(TimedDoorTest, TimeoutValueStored) {
    EXPECT_EQ(2, door->getTimeOut());
}
TEST_F(TimedDoorTest, UnlockOpensDoor) {
    door->unlock();
    EXPECT_TRUE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, LockClosesDoor) {
    door->unlock();
    door->lock();
    EXPECT_FALSE(door->isDoorOpened());
}
TEST_F(TimedDoorTest, ThrowStateThrowsRuntimeError) {
    EXPECT_THROW(door->throwState(), std::runtime_error);
}

class DoorTimerAdapterTest : public ::testing::Test {
protected:
    TimedDoor* door;
    DoorTimerAdapter* adapter;
    void SetUp() override {
        door = new TimedDoor(1);
        adapter = new DoorTimerAdapter(*door);
    }
    void TearDown() override {
        delete door;
        delete adapter;
    }
};
TEST_F(DoorTimerAdapterTest, TimeoutThrowsIfDoorOpen) {
    door->unlock();
    EXPECT_THROW(adapter->Timeout(), std::runtime_error);
}
TEST_F(DoorTimerAdapterTest, TimeoutNoThrowIfDoorClosed) {
    door->lock();
    EXPECT_NO_THROW(adapter->Timeout());
}

class MockTimerClient : public TimerClient {
public:
    MOCK_METHOD(void, Timeout, (), (override));
};
TEST(TimerTest, TimerCallsTimeout) {
    MockTimerClient client;
    EXPECT_CALL(client, Timeout()).Times(Exactly(1));
    Timer timer;
    timer.tregister(0, &client);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

TEST(TimerTest, TimerDoesNotCallAfterStop) {
    MockTimerClient client;
    EXPECT_CALL(client, Timeout()).Times(Exactly(0));
    Timer timer;
    timer.tregister(1, &client);
    timer.stopWorker();
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
}

class TestableDoorTimerAdapter : public DoorTimerAdapter {
private:
    std::exception_ptr* storedException;
public:
    TestableDoorTimerAdapter(TimedDoor& door, std::exception_ptr* ex)
        : DoorTimerAdapter(door), storedException(ex) {}
    void Timeout() override {
        try {
            DoorTimerAdapter::Timeout();
        } catch (...) {
            *storedException = std::current_exception();
        }
    }
};

class IntegrationTest : public ::testing::Test {
protected:
    TimedDoor* door;
    TestableDoorTimerAdapter* testAdapter;
    std::exception_ptr exception;
    void SetUp() override {
        door = new TimedDoor(1);
        testAdapter = new TestableDoorTimerAdapter(*door, &exception);
        door->changeAdapter(testAdapter);
    }
    void TearDown() override {
        delete door;
    }
};

// Тест 10: Интеграция - исключение выбрасывается если дверь открыта дольше таймаута
TEST_F(IntegrationTest, ThrowsWhenDoorOpenAfterTimeout) {
    door->unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(1050));
    ASSERT_NE(exception, nullptr);
}

// Тест 11: Интеграция - исключение НЕ выбрасывается если дверь закрыта вовремя
TEST_F(IntegrationTest, NoThrowWhenDoorClosedBeforeTimeout) {
    door->unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    door->lock();
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    ASSERT_EQ(exception, nullptr);
}
