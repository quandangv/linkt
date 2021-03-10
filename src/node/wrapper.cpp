#include "wrapper.hpp"
#include "common.hpp"
#include "tstring.hpp"
#include "token_iterator.hpp"

#include <sstream>
#include <iostream>

NAMESPACE(node)

// Returns the pointer to the node at the specified path
// This will return the inner node of a wrapper.
base_s wrapper::get_child_ptr(tstring path) const {
  if (auto immediate_path = cut_front(trim(path), '.'); !immediate_path.untouched()) {
    if (auto iterator = map.find(immediate_path); iterator != map.end())
      if (auto child = dynamic_cast<wrapper*>(iterator->second.get()))
        return child->get_child_ptr(path);
  } else if (auto iterator = map.find(path); iterator != map.end()) {
    if (auto child = dynamic_cast<wrapper*>(iterator->second.get()))
      return child->map[""];
    return iterator->second;
  }
  return {};
}

// Returns the pointer to the storage of the node or wrapper at the specified path
// Returns null if it doesn't exist
base_s* wrapper::get_child_place(tstring path) {
  if (auto immediate_path = cut_front(trim(path), '.'); !immediate_path.untouched()) {
    if (auto iterator = map.find(immediate_path); iterator != map.end())
      if (auto child = dynamic_cast<wrapper*>(iterator->second.get()))
        return child->get_child_place(path);
  } else if (auto iterator = map.find(path); iterator != map.end())
    return &iterator->second;
  return nullptr;
}

std::optional<string> wrapper::get_child(const tstring& path) const {
  auto ptr = get_child_ptr(path);
  return ptr ? ptr->get() : std::optional<string>{};
}

string wrapper::get_child(const tstring& path, string&& fallback) const {
  auto result = get_child(path);
  return result ? *result : move(fallback);
}

base_s& wrapper::add(tstring path, ancestor_processor* processor) {
  trim(path);
  for (char c : path)
    if (auto invalid = strchr(" #$\"'(){}[]", c))
      THROW_ERROR(wrapper, "Invalid character '" + string{*invalid} + "' in path: " + path);

  if (auto immediate_path = cut_front(path, '.'); !immediate_path.untouched()) {
    // This isn't the final part of the path
    auto& ptr = map[immediate_path];
    wrapper_s ancestor;
    if (!ptr) {
      ancestor = std::make_shared<wrapper>();
      ptr = ancestor;
    } else if (!(ancestor = std::dynamic_pointer_cast<wrapper>(ptr)))
      ancestor = wrap(ptr);
    if (processor)
      processor->operator()(immediate_path, ancestor);
    return ancestor->add(path);
  } else {
    // This is the final part of the path
    auto& place = map[path];
    if (!place)
      return place;
    wrapper* wrpr = dynamic_cast<wrapper*>(place.get());
    return wrpr ? wrpr->map[""] : place;
  }
}

base_s& wrapper::add(tstring path, const base_s& value) {
  auto& place = add(path);
  return !place ? (place = value) : !value ? place : THROW_ERROR(wrapper, "Add: Duplicate key");
}

base_s& wrapper::add(tstring path, string& raw, tstring value, parse_context& context) {
  add(path);
  context.parent = shared_from_this();
  context.current.reset();
  context.place = get_child_place(path);
  if (auto node = parse_raw(raw, value, context))
    return context.get_place() = node;
  return *context.place;
}

void wrapper::iterate_children(std::function<void(const string&, const base_s&)> processor) const {
  for(auto& pair : map)
    processor(pair.first, pair.second);
}

void wrapper::iterate_children(std::function<void(const string&, const base&)> processor) const {
  iterate_children([&](const string& name, const base_s& child) {
    if (child)
      processor(name, *child);
  });
}


wrapper_s wrapper::wrap(base_s& place) {
  auto wrp = std::make_shared<wrapper>(place);
  place = wrp;
  return wrp;
}

bool wrapper::set(const tstring& path, const string& value) {
  auto target = dynamic_cast<settable*>(get_child_ptr(path).get());
  return target ? target->set(value) : false;
}

string wrapper::get() const {
  const auto& value = get_child_ptr(""_ts);
  return value ? value->get() : "";
}

void wrapper::optimize(clone_context& context) {
  context.optimize = true;
  auto tmp = std::make_shared<wrapper>();
  tmp->map.swap(map);
  merge(tmp, context);
}

void wrapper::merge(const const_wrapper_s& src, clone_context& context) {
  context.ancestors.emplace_back(src, shared_from_this());
  for(auto& pair : src->map) {
    if (!pair.second)
      continue;
    auto last_path = context.current_path;
    context.current_path += context.ancestors.size() == 1 ? pair.first : ("." + pair.first);
    try {
      if (auto& place = map[pair.first]; !place)
        place = pair.second->clone(context);
      else if (auto wrp = dynamic_cast<wrapper*>(place.get())) {
        if (auto src_wrp = std::dynamic_pointer_cast<wrapper>(pair.second))
          wrp->merge(src_wrp, context);
        else
          wrp->map[""] = pair.second->clone(context);
      }
    } catch (const std::exception& e) {
      context.report_error("Exception while cloning " + context.current_path + ": " + e.what());
    }
    context.current_path = last_path;
  }
  context.ancestors.pop_back();
}

base_s wrapper::clone(clone_context& context) const {
  auto result = std::make_shared<wrapper>();
  result->merge(shared_from_this(), context);
  return result;
}

NAMESPACE_END
