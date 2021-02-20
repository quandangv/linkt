#include "container.hpp"
#include "node.hpp"
#include "common.hpp"
#include "string_interpolate.hpp"

GLOBAL_NAMESPACE

using spot = string_interpolate_ref::replace_spot;

struct replacement_iterator {
  vector<spot>::const_iterator it;
  explicit replacement_iterator(const std::vector<spot>::const_iterator& it) : it(it) {}
  replacement_iterator& operator++() { ++it; return *this; }
  bool operator==(const replacement_iterator& other) const { return other.it == it; }
  string operator*() { return (*it).replacement->get(); }
};
struct position_iterator {
  vector<spot>::const_iterator it;
  explicit position_iterator(const std::vector<spot>::const_iterator& it) : it(it) {}
  position_iterator& operator++() { ++it; return *this; }
  bool operator==(const position_iterator& other) const { return other.it == it; }
  size_t operator*() { return it->position; }
};
template<typename IteratorType>
struct list {
  const vector<spot>& list;

  IteratorType begin() const {
    return IteratorType(list.begin());
  }
  IteratorType end() const {
    return IteratorType(list.end());
  }
};

string string_interpolate_ref::get() const {
  return interpolate(base, list<position_iterator>{spots}, list<replacement_iterator>{spots});
}

GLOBAL_NAMESPACE_END
