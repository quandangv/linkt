#include "container.hpp"
#include "node.hpp"
#include "common.hpp"
#include "string_interpolate.hpp"

NAMESPACE(lini::node)

using spot = string_interpolate::replace_spot;

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

string string_interpolate::get() const {
  return interpolate(base, list<position_iterator>{spots}, list<replacement_iterator>{spots});
}

base_p string_interpolate::clone(clone_handler handler) const {
  auto result = std::make_unique<string_interpolate>();
  result->base = base;
  result->spots.reserve(spots.size());
  for(auto& spot : spots)
    result->spots.emplace_back(spot.position, spot.name, node::clone(*spot.replacement, handler));
  return move(result);
}

NAMESPACE_END
