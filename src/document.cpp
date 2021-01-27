#include "document.hpp"
#include "common.hpp"
#include "tstring.hpp"
#include "token_iterator.hpp"

#include <sstream>
#include <iostream>

GLOBAL_NAMESPACE

using namespace std;

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
  if (auto immediate_path = cut_front(path, '.'); !immediate_path.untouched()) {
    // This isn't the final node of the path
    auto& ptr = map[immediate_path];
    if (!ptr) {
      ptr = make_shared<string_ref_p>(make_unique<document>());
    }
    auto& child_ptr = *ptr;
    if (!child_ptr) {
      auto tmp = make_unique<document>();
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
      place = make_shared<string_ref_p>();
    } else if (*place) {
      throw error("Duplicate key: " + static_cast<string>(path));
    }
    *place = move(value);
    return place;
  }
}

GLOBAL_NAMESPACE_END
