#include "document.hpp"

#include <sstream>
#include <iostream>

using std::endl;

string to_string(const document& doc) {
  std::stringstream ss;
  auto print_keyval = [&](const section::value_type& keyval) {
    ss << keyval.first << " = ";
    if (keyval.second.empty())
      ss << endl;
    else if (keyval.second.front() == ' ' || keyval.second.back() == ' ')
      ss << '"' << keyval.second << '"' << endl;
    else 
      ss << keyval.second << endl;
  };
  if(doc.find("") != doc.end())
    for(auto& keyval : doc.at(""))
      print_keyval(keyval);

  for(auto& sec : doc) {
    if(sec.first.empty()) continue;
    ss << endl << '[' << sec.first << ']' << endl;
    for(auto& keyval : sec.second)
      print_keyval(keyval);
  }
  return ss.str();
}
