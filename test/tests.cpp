#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <chrono>
#include <stdexcept>
#include "TimedDoor.h"

using ::testing::Exactly;
using ::testing::AtLeast;

// ==================== Тесты для TimedDoor ====================

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

// Тест 1: Проверка начального состояния (дверь закрыта)
TEST_F(TimedDoorTest, DoorInitiallyClosed) {
    EXPECT_FALSE(door->isDoorOpened());
}

// Тест 2: Проверка сохранения таймаута
TEST_F(TimedDoorTest, TimeoutValueStored) {
    EXPECT_EQ(2, door->getTimeOut());
}

// Тест 3: Проверка открытия двери
TEST_F(TimedDoorTest, UnlockOpensDoor) {
    door->unlock();
    EXPECT_TRUE(door->isDoorOpened());
}

// Тест 4: Проверка закрытия двери
TEST_F(TimedDoorTest, LockClosesDoor) {
    door->unlock();
    door->lock();
    EXPECT_FALSE(door->isDoorOpened());
}

// Тест 5: Проверка исключения
TEST_F(TimedDoorTest, ThrowStateThrowsRuntimeError) {
    EXPECT_THROW(door->throwState(), std::runtime_error);
}

// ==================== Тесты для DoorTimerAdapter ====================

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

// Тест 6: Адаптер выбрасывает исключение при открытой двери
TEST_F(DoorTimerAdapterTest, TimeoutThrowsIfDoorOpen) {
    door->unlock();
    EXPECT_THROW(adapter->Timeout(), std::runtime_error);
}

// Тест 7: Адаптер не выбрасывает исключение при закрытой двери
TEST_F(DoorTimerAdapterTest, TimeoutNoThrowIfDoorClosed) {
    door->lock();
    EXPECT_NO_THROW(adapter->Timeout());
}

// ==================== Тесты для Timer с моками ====================

class MockTimerClient : public TimerClient {
public:
    MOCK_METHOD(void, Timeout, (), (override));
};

// Тест 8: Проверка вызова Timeout у клиента
TEST(TimerTest, TimerCallsTimeout) {
    MockTimerClient client;
    EXPECT_CALL(client, Timeout()).Times(Exactly(1));
    
    Timer timer;
    timer.tregister(0, &client);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

// Тест 9: Проверка что таймер не вызывает после остановки
TEST(TimerTest, TimerDoesNotCallAfterStop) {
    MockTimerClient client;
    EXPECT_CALL(client, Timeout()).Times(Exactly(0));
    
    Timer timer;
    timer.tregister(1, &client);
    timer.stopWorker();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
}

// ==================== Интеграционные тесты ====================

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