#ifndef __EVENTS_H_
#define __EVENTS_H_

#include <iostream>
#include <stdexcept>
#include <functional>
#include <typeinfo>
#include <string>
#include <map>

class EventEmitter {
  typedef std::vector<void*> triggerlist;  
  std::map<std::string, triggerlist> events;
  std::map<std::string, bool> events_once;

  template <typename Callback> 
  struct traits : public traits<decltype(&Callback::operator())> {
  };

  template <typename ClassType, typename R, typename... Args>
  struct traits<R(ClassType::*)(Args...) const> {

    typedef std::function<R(Args...)> fn;
  };

  template <typename Callback>
  typename traits<Callback>::fn
  to_function (Callback& cb) {

    return static_cast<typename traits<Callback>::fn>(cb);
  }

  int _listeners = 0;

  public:
    int maxListeners = 10;

    int listeners() {
      return this->_listeners;
    }

    template <typename Callback>
    void on(const std::string& name, Callback cb) {

      auto it = events.find(name);
      if (it == events.end()) {
        triggerlist newList;
        events[name] = std::move(newList);
      }

      if (++this->_listeners >= this->maxListeners) {
        std::cout 
          << "warning: possible EventEmitter memory leak detected. " 
          << this->_listeners 
          << " listeners added. "
          << std::endl;
      };

      auto f = to_function(cb);
      auto fn = new decltype(f)(to_function(cb));
      events[name].push_back(static_cast<void*>(fn));
      //events[name] = static_cast<void*>(fn);
    }

    template <typename Callback>
    void once(const std::string& name, Callback cb) {
      this->on(name, cb);
      events_once[name] = true;
    }

    void off() {
      events.clear();
      events_once.clear();
      this->_listeners = 0;
    }

    void off(const std::string& name) {

      auto it = events.find(name);

      if (it != events.end()) {
        events.erase(it);
        this->_listeners--;

        auto once = events_once.find(name);
        if (once != events_once.end()) {
          events_once.erase(once);
        }
      }
    }

    template <typename ...Args> 
    void emit(std::string name, Args... args) {

      auto it = events.find(name);
      if (it != events.end()) {
        for (auto & element :  events.at(name)) {
            auto fp = static_cast<std::function<void(Args...)>*>(element);
            (*fp)(args...);
        }
      }

      auto once = events_once.find(name);
      if (once != events_once.end()) {
        this->off(name);
      }
    }

    EventEmitter(int max_listeners = 100) {
      this->maxListeners = max_listeners;
    }

    ~EventEmitter (void) {
      events.clear();
    }
};

#endif

