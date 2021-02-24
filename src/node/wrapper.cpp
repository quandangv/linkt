#include "wrapper.hpp"
#include "common.hpp"
#include "tstring.hpp"
#include "token_iterator.hpp"

#include <sstream>
#include <iostream>

NAMESPACE(lini::node)

void wrapper::iterate_children(std::function<void(const string&, const base&)> processor) const {
  iterate_children([&](const string& name, const base_p& child) {
    if (child)
      processor(name, *child);
  });
}

std::optional<string> wrapper::get_child(const tstring& path) const {
  auto ptr = get_child_ptr(path);
  return ptr ? ptr->get() : std::optional<string>{};
}

string wrapper::get_child(const tstring& path, string&& fallback) const {
  auto result = get_child(path);
  return result ? *result : move(fallback);
}

base& wrapper::get_child_ref(const tstring& path) const {
  return *(get_child_ptr(path).get() ?: throw base::error("Key is empty"));
}

bool wrapper::set(const tstring& path, const string& value) {
  auto target = dynamic_cast<settable*>(get_child_ptr(path).get());
  return target ? target->set(value) : false;
}

base_p wrapper::add(tstring path, string& raw, tstring value) {
  return add(path, parse_string(raw, value, [&](tstring& ts, const base_p& fallback) { return make_address_ref(ts, fallback); }));
}

base_p wrapper::add(tstring path, string raw) {
  return add(path, raw, tstring(raw));
}

std::shared_ptr<address_ref> wrapper::make_address_ref(const tstring& ts, const base_p& fallback) {
  return std::make_unique<address_ref>(*this, ts, move(fallback));
}


base_p wrapper::get_child_ptr(tstring path) const {
  if (auto immediate_path = cut_front(trim(path), '.'); !immediate_path.untouched()) {
    if (auto iterator = map.find(immediate_path); iterator != map.end())
      if (auto child = dynamic_cast<wrapper*>(iterator->second.get()); child)
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
    return (!ptr ? assign(ptr, std::make_shared<wrapper>()) : dynamic_cast<wrapper*>(ptr.get()) ?: &wrap(ptr))->add(path, value);
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

void wrapper::optimize() {
  node::optimize(value);
  for(auto pair : map)
    node::optimize(pair.second);
}

NAMESPACE_END
