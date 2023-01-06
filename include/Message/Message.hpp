#pragma once

#include <map>
#include <functional>
class People
{
  std::map<std::string, std::function<void (void)>> function_map_;
public:
  void push();
  void oepn();
};

class Manager
{
private:
  std::map<std::string, People> node_map;
public:
  bool resigerNode(const std::string nodename);
  bool runNun(const std::string nodename, const std::string funcname);
  // *
  // *
  // *
  bool delNode(const std::string nodename);
};


/**
 *
 */
class Message
{
public:
  Message(const std::string &message): message_(message) {};
  friend std::ostream &operator<<(std::ostream &os, Message &obj)
  {
    return obj.printSelf(os);
  }
private:
  std::string message_;
  std::ostream &printSelf(std::ostream &os);
};