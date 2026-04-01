// Copyright 2021 GHA Test Team
#include "TimedDoor.h"
#include <thread>
#include <chrono>
#include <stdexcept>

//
// DoorTimerAdapter
//
DoorTimerAdapter::DoorTimerAdapter(TimedDoor& _door) : door(_door) {}

void DoorTimerAdapter::Timeout() {
    if (door.isDoorOpened()) {
        door.throwState();
    }
}

void TimedDoor::changeAdapter(DoorTimerAdapter* _adapter) {
    adapter = _adapter;
}

TimedDoor::~TimedDoor() {
    timer.stopWorker();
    delete adapter;
}

//
// TimedDoor
//
TimedDoor::TimedDoor(int timeout) {
    iTimeout = timeout;
    isOpened = false;
    adapter = new DoorTimerAdapter(*this);
}

bool TimedDoor::isDoorOpened() {
    return isOpened;
}

void TimedDoor::unlock() {
    isOpened = true;
    timer.tregister(iTimeout, adapter);
}

void TimedDoor::lock() {
    isOpened = false;
}

int TimedDoor::getTimeOut() const {
    return iTimeout;
}

void TimedDoor::throwState() {
    throw std::runtime_error("Door left opened for too long");
}

void Timer::sleep(int time) {
    std::this_thread::sleep_for(std::chrono::seconds(time));
}

void Timer::stopWorker() {
    isRunning = false;
    if (worker.joinable()) {
        worker.join();
    }
}

Timer::~Timer() {
    stopWorker();
}

void Timer::tregister(int timeout, TimerClient *_client) {
    stopWorker();
    isRunning = true;
    client = _client;
    worker = std::thread([this, timeout]() {
        sleep(timeout);
        if (isRunning && client) {
            client->Timeout();
        }
    });
}
