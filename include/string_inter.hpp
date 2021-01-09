#pragma once

#include "error.hpp"

#include <sstream>
#include <string>
#include <vector>
#include <iterator>
#include <concepts>

template<typename T, typename U>
concept iterable = requires (T a) {
  { *a.begin() } -> std::convertible_to<U>;
  { *a.begin()++ } -> std::convertible_to<U>;
  { *a.end() }   -> std::convertible_to<U>;
};

DEFINE_ERROR(stringinter_error)

struct string_inter {
  std::string base;
  std::vector<int> positions;

  template<typename T>
  std::string get(const T& replacements) const requires iterable<T, std::string> {
    std::stringstream ss;
    auto repit = replacements.begin();
    int lastpoint = 0;
    for (int point : positions) {
      if (point > base.size())
        throw stringinter_error("Position out of range: " + std::to_string(point));
      if (repit == replacements.end())
        throw stringinter_error("Reached end of iterator while replacements st//ill needed");
      ss << base.substr(lastpoint, point - lastpoint) << *repit++;
      lastpoint = point;
    }
    ss << base.substr(lastpoint);
    return ss.str();
  }
};
