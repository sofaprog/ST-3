// Copyright 2021 GHA Test Team

#ifndef INCLUDE_TIMEDDOOR_H_
#define INCLUDE_TIMEDDOOR_H_

#include <thread>
#include <atomic>

class DoorTimerAdapter;
class Timer;
class Door;
class TimedDoor;

class TimerClient {
 public:
  virtual void Timeout() = 0;
  virtual ~TimerClient() = default;
};

class Door {
 public:
  virtual void lock() = 0;
  virtual void unlock() = 0;
  virtual bool isDoorOpened() = 0;
  virtual ~Door() = default;
};

class DoorTimerAdapter : public TimerClient {
 private:
  TimedDoor& door;

 public:
  explicit DoorTimerAdapter(TimedDoor&);
  void Timeout() override;
};

class Timer {
  std::thread worker;
  std::atomic<bool> isRunning;
  TimerClient *client;
  void sleep(int);

 public:
  Timer() : isRunning(false), client(nullptr) {}
  ~Timer();

  void stopWorker();
  void tregister(int, TimerClient*);
};

class TimedDoor : public Door {
 private:
  DoorTimerAdapter * adapter;
  int iTimeout;
  bool isOpened;
  Timer timer;

 public:
  ~TimedDoor();
  explicit TimedDoor(int);

  bool isDoorOpened() override;
  void unlock() override;
  void lock() override;
  int  getTimeOut() const;
  void throwState();

  void changeAdapter(DoorTimerAdapter*);
};

#endif  // INCLUDE_TIMEDDOOR_H_
