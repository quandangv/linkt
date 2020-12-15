#include "delink.hpp"

void delink(document& doc, errorlist& err) {
  for(auto& sec_key : doc) {
    auto& section = sec_key.second;
    for(auto& keyval : section) {
      auto& value = keyval.second;
        
    }
  }
}
