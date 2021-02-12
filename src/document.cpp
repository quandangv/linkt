#include "document.hpp"
#include "common.hpp"
#include "tstring.hpp"
#include "token_iterator.hpp"

#include <sstream>
#include <iostream>

GLOBAL_NAMESPACE

string_ref_p2 document::get_child_ptr(tstring path) const {
  if (auto immediate_path = cut_front(trim(path), '.'); !immediate_path.untouched()) {
    if (auto iterator = map.find(immediate_path); iterator != map.end() && iterator->second)
      if (auto child = dynamic_cast<container*>(iterator->second->get()); child)
        return child->get_child_ptr(path);
  } else if (auto iterator = map.find(path); iterator != map.end())
    return iterator->second;
  return {};
}

string_ref_p2 document::add(tstring path, string_ref_p&& value) {
  // Check path for invalid characters
  trim(path);
  for(char c : path)
    if(auto invalid = strchr(" #$\"'(){}[]", c); invalid)
      throw error("Invalid character '" + string{*invalid} + "' in path: " + path);
    
  if (auto immediate_path = cut_front(path, '.'); !immediate_path.untouched()) {
    // This isn't the final node of the path
    auto& ptr = map[immediate_path];
    if (!ptr) {
      ptr = std::make_shared<string_ref_p>(std::make_unique<document>());
    }
    auto& child_ptr = *ptr;
    if (!child_ptr) {
      auto tmp = std::make_unique<document>();
      auto res = tmp->add(path, move(value));
      child_ptr = move(tmp);
      return res;
    } if (auto child = dynamic_cast<addable*>(child_ptr.get()); child)
      return child->add(path, move(value));
    throw error("Child " + immediate_path + " already exists but can't be added to");

  } else {
    // This is the final node of the path
    auto& place = map[path];
    if (!place) {
      place = std::make_shared<string_ref_p>();
    } else if (*place) {
      throw error("Duplicate key: " + static_cast<string>(path));
    }
    *place = move(value);
    return place;
  }
}

void document::iterate_children(std::function<void(const string&, const string_ref&)> processor) const {
  for(auto pair : map) {
    if (pair.second && *pair.second)
    processor(pair.first, **pair.second);
  }
}

string document::get() const {
  return value ? value->get() : "";
}

GLOBAL_NAMESPACE_END
