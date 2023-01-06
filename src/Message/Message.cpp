#include <iostream>
#include "Message.hpp"

std::ostream &Message::printSelf(std::ostream &os)
{
  return os << message_;
}


void Print()
{
  std::cout << "Hello World!\n";
}