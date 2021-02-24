#include "node.hpp"
#include "common.hpp"
#include "string_interpolate.hpp"

NAMESPACE(lini::node)

using spot = string_interpolate::replace_spot;

template<typename T>
struct base_it {
  vector<spot>::const_iterator it;
  T& operator++() { ++it; return *(T*)this; }
  bool operator==(const T& other) const { return other.it == it; }
};
struct replacement_it : public base_it<replacement_it> {
  string operator*() { return (*it).replacement->get(); }
};
struct position_it : public base_it<position_it> {
  size_t operator*() { return it->position; }
};
template<typename IteratorType>
struct list {
  const vector<spot>& list;

  IteratorType begin() const {
    return IteratorType{list.begin()};
  }
  IteratorType end() const {
    return IteratorType{list.end()};
  }
};

string string_interpolate::get() const {
  return interpolate(base, list<position_it>{spots}, list<replacement_it>{spots});
}

base_p string_interpolate::clone(clone_handler handler) const {
  LG_DBUG("Clone string interpolate")
  auto result = std::make_unique<string_interpolate>();
  result->base = base;
  result->spots.reserve(spots.size());
  for(auto& spot : spots)
    result->spots.emplace_back(spot.position, spot.name, node::clone(*spot.replacement, handler));
  return move(result);
}

NAMESPACE_END
