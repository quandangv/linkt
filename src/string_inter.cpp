#include "string_inter.hpp"

using namespace std;

string string_inter::get() {
  for(auto point : positions) {
    if (point > base.size())
      throw stringinter_error("Position out of range: " + tostring(point));
    if ()
      throw stringinter_error("Reached end of iterator while replacements still needed");

