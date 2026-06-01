// Copyright 2021 GHA Test Team

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstdint>
#include <stdexcept>
#include "TimedDoor.h"

using ::testing::_;
using ::testing::AtLeast;

// ─── Mock-классы для тестирования интерфейсов ────────────────────────────────

class MockDoor : public Door {
 public:
    MOCK_METHOD(void, lock,   (), (override));
    MOCK_METHOD(void, unlock, (), (override));
    MOCK_METHOD(bool, isDoorOpened, (), (override));
};

class MockTimerClient : public TimerClient {
 public:
    MOCK_METHOD(void, Timeout, (), (override));
};

// ─── Фикстура для основных тестов ────────────────────────────────────────────

class TimedDoorTest : public ::testing::Test {
 protected:
    TimedDoor* door;

    void SetUp() override {
        door = new TimedDoor(5);
    }

    void TearDown() override {
        delete door;
    }
};

// ─── Тесты TimedDoor ──────────────────────────────────────────────────────────

// 1. Дверь создаётся в закрытом состоянии
TEST_F(TimedDoorTest, InitiallyDoorIsClosed) {
    EXPECT_FALSE(door->isDoorOpened());
}

// 2. unlock() открывает дверь
TEST_F(TimedDoorTest, UnlockOpensDoor) {
    door->unlock();
    EXPECT_TRUE(door->isDoorOpened());
}

// 3. lock() закрывает дверь
TEST_F(TimedDoorTest, LockClosesDoor) {
    door->unlock();
    door->lock();
    EXPECT_FALSE(door->isDoorOpened());
}

// 4. getTimeOut() возвращает заданное значение таймаута
TEST_F(TimedDoorTest, GetTimeoutReturnsCorrectValue) {
    EXPECT_EQ(door->getTimeOut(), 5);
}

// 5. getTimeOut() с другим значением
TEST_F(TimedDoorTest, GetTimeoutReturnsCorrectValueForDifferentTimeout) {
    TimedDoor anotherDoor(10);
    EXPECT_EQ(anotherDoor.getTimeOut(), 10);
}

// 6. throwState() выбрасывает исключение, если дверь открыта
TEST_F(TimedDoorTest, ThrowStateThrowsWhenDoorIsOpen) {
    door->unlock();
    EXPECT_THROW(door->throwState(), std::runtime_error);
}

// 7. throwState() НЕ выбрасывает исключение, если дверь закрыта
TEST_F(TimedDoorTest, ThrowStateDoesNotThrowWhenDoorIsClosed) {
    door->lock();
    EXPECT_NO_THROW(door->throwState());
}

// 8. После unlock() и lock() throwState() не выбрасывает исключение
TEST_F(TimedDoorTest, ThrowStateNoThrowAfterUnlockThenLock) {
    door->unlock();
    door->lock();
    EXPECT_NO_THROW(door->throwState());
}

// ─── Фикстура для тестов адаптера ────────────────────────────────────────────

class DoorTimerAdapterTest : public ::testing::Test {
 protected:
    TimedDoor* door;
    DoorTimerAdapter* adapter;

    void SetUp() override {
        door    = new TimedDoor(3);
        adapter = new DoorTimerAdapter(*door);
    }

    void TearDown() override {
        delete adapter;
        delete door;
    }
};

// 9. Timeout() адаптера выбрасывает исключение при открытой двери
TEST_F(DoorTimerAdapterTest, TimeoutThrowsWhenDoorIsOpen) {
    door->unlock();
    EXPECT_THROW(adapter->Timeout(), std::runtime_error);
}

// 10. Timeout() адаптера НЕ выбрасывает исключение при закрытой двери
TEST_F(DoorTimerAdapterTest, TimeoutDoesNotThrowWhenDoorIsClosed) {
    door->lock();
    EXPECT_NO_THROW(adapter->Timeout());
}

// ─── Тесты Mock-интерфейсов ───────────────────────────────────────────────────

// 11. MockDoor: lock() вызывается ровно один раз
TEST(MockDoorTest, LockCalledOnce) {
    MockDoor mockDoor;
    EXPECT_CALL(mockDoor, lock()).Times(1);
    mockDoor.lock();
}

// 12. MockDoor: unlock() вызывается ровно один раз
TEST(MockDoorTest, UnlockCalledOnce) {
    MockDoor mockDoor;
    EXPECT_CALL(mockDoor, unlock()).Times(1);
    mockDoor.unlock();
}

// 13. MockDoor: isDoorOpened() возвращает заданное значение
TEST(MockDoorTest, IsDoorOpenedReturnsMockedValue) {
    MockDoor mockDoor;
    EXPECT_CALL(mockDoor, isDoorOpened()).WillOnce(::testing::Return(true));
    EXPECT_TRUE(mockDoor.isDoorOpened());
}

// 14. MockTimerClient: Timeout() вызывается хотя бы один раз
TEST(MockTimerClientTest, TimeoutCalledAtLeastOnce) {
    MockTimerClient mockClient;
    EXPECT_CALL(mockClient, Timeout()).Times(AtLeast(1));
    mockClient.Timeout();
}
