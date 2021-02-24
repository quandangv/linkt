#include "wrapper.hpp"
#include "common.hpp"
#include "tstring.hpp"
#include "token_iterator.hpp"

#include <sstream>
#include <iostream>

NAMESPACE(lini::node)

base_p wrapper::get_child_ptr(tstring path) const {
  if (auto immediate_path = cut_front(trim(path), '.'); !immediate_path.untouched()) {
    if (auto iterator = map.find(immediate_path); iterator != map.end())
      if (auto child = dynamic_cast<container*>(iterator->second.get()); child)
        return child->get_child_ptr(path);
  } else if (auto iterator = map.find(path); iterator != map.end())
    return iterator->second;
  return {};
}

wrapper& wrapper::wrap(base_p& place) {
  return *assign(place, std::make_shared<wrapper>(place));
}

base_p& wrapper::add(tstring path, const base_p& value) {
  // Check path for invalid characters
  trim(path);
  for(char c : path)
    if(auto invalid = strchr(" #$\"'(){}[]", c); invalid)
      throw error("Invalid character '" + string{*invalid} + "' in path: " + path);

  if (auto immediate_path = cut_front(path, '.'); !immediate_path.untouched()) {
    // This isn't the final part of the path
    auto& ptr = map[immediate_path];
    return (!ptr ? assign(ptr, std::make_shared<wrapper>()) : dynamic_cast<addable*>(ptr.get()) ?: &wrap(ptr))->add(path, value);
  } else {
    // This is the final part of the path
    auto& place = map[path];
    (!place ? place : (dynamic_cast<wrapper*>(place.get()) ?: throw error("Duplicate key: " + path))->value) = value;
    return place;
  }
}

void wrapper::iterate_children(std::function<void(const string&, const base_p&)> processor) const {
  for(auto pair : map)
    processor(pair.first, pair.second);
}

string wrapper::get() const {
  return value ? value->get() : "";
}

NAMESPACE_END
