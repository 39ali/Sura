#pragma once
class DeletionQueue {
public:
  std::deque<std::function<void()>> queue;

  void push_function(std::function<void()> &&function) {

    queue.push_back(function);
  }

  void flush() {
    for (auto &func : queue) {
      func();
    }
  }
};
