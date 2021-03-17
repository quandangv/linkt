#include "node.hpp"
#include "common.hpp"
#include "wrapper.hpp"
#include "string_interpolate.hpp"

NAMESPACE(node)

using spot = string_interpolate::replace_spot;

template<typename T>
struct base_it {
  vector<spot>::const_iterator it;
  T& operator++() { ++it; return reinterpret_cast<T&>(*this); }
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

string_interpolate::operator string() const {
  return interpolate(base, list<position_it>{spots}, list<replacement_it>{spots});
}

base_s string_interpolate::clone(clone_context& context) const {
  if (context.optimize && is_fixed())
    return std::make_shared<plain<string>>(get());
  auto result = std::make_unique<string_interpolate>();
  result->base = base;
  result->spots.reserve(spots.size());
  for(auto& spot : spots)
    result->spots.emplace_back(spot.position, checked_clone<string>(spot.replacement, context, "string_interpolate::clone"));
  return result;
}

bool string_interpolate::is_fixed() const {
  for(auto& spot : spots)
    if (!spot.replacement->is_fixed())
      return false;
  return true;
}

NAMESPACE_END
