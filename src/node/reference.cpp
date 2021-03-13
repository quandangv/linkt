#include "reference.hpp"
#include "common.hpp"
#include "wrapper.hpp"

#include <sstream>

NAMESPACE(node)

address_ref::address_ref(wrapper_w ancestor, tstring path)
    : ancestor_w(ancestor), indirect_paths() {
  trim(path);
  for (tstring indirect; !(indirect = cut_front(path, '.')).untouched();)
    indirect_paths.emplace_back(indirect);
  direct_path = path;
}

string address_ref::get_path() const {
  std::stringstream ss;
  for (auto& path : indirect_paths)
    ss << path << '.';
  ss << direct_path;
  return ss.str();
}

base_s address_ref::get_source() const {
  auto direct = get_source_direct();
  if (!direct || !*direct) {
    return {};
  }
  if (auto direct_wrapper = std::dynamic_pointer_cast<wrapper>(*direct)) {
    return direct_wrapper->map[""];
  }
  return *direct;
}

base_s* address_ref::get_source_direct() const {
  auto ancestor = ancestor_w.lock();
  if (!ancestor) THROW_ERROR(ancestor_destroyed, "get_source_direct");
  for (auto& path : indirect_paths)
    if (!(ancestor = ancestor->get_wrapper(path)))
      return nullptr;

  if (auto it = ancestor->map.find(direct_path); it != ancestor->map.end())
    return &it->second;
  return nullptr;
}

string address_ref::get() const {
  auto src = get_source();
  if (!src) THROW_ERROR(node, "Referenced key not found: " + get_path());
  return src->get();
}

bool address_ref::set(const string& val) {
  auto ancestor = ancestor_w.lock();
  if (!ancestor) THROW_ERROR(ancestor_destroyed, "set");
  auto src = get_source();
  if (!src)
    return false;
  auto target = std::dynamic_pointer_cast<settable>(src);
  return target ? target->set(val) : false;
}

base_s address_ref::clone(clone_context& context) const {
  auto ancestor = ancestor_w.lock();
  if (!ancestor) THROW_ERROR(ancestor_destroyed, "clone");
  // Find the corresponding ancestor in the clone result tree
  auto ancestor_it = find_if(context.ancestors.rbegin(), context.ancestors.rend(), [&](auto& pair) { return pair.first == ancestor; });
  wrapper_s cloned_ancestor;
  if (ancestor_it != context.ancestors.rend()) {
    cloned_ancestor = ancestor_it->second;
  } else if (context.no_dependency) {
    THROW_ERROR(clone, "Address_ref: External dependency");
  } else {
    cloned_ancestor = ancestor;
  }

  // Return a pointer to the referenced node
  if (context.optimize) {
    // If the referenced node already exists in the clone result, we don't have to clone it
    for (auto& path : indirect_paths) {
      cloned_ancestor = cloned_ancestor->add_wrapper(path);
      if (!(ancestor = ancestor->get_wrapper(path)))
        return std::make_shared<address_ref>(cloned_ancestor, string(get_path()));
      context.ancestors.emplace_back(ancestor, cloned_ancestor);
    }

    auto& result = cloned_ancestor->map[direct_path];
    auto result_wrapper = std::dynamic_pointer_cast<wrapper>(result);
    if (result_wrapper) {
      if (result_wrapper->map[""])
        return std::make_shared<ref>(result_wrapper->map[""]);
    } else if (result)
      return std::make_shared<ref>(result);

    auto src_it = ancestor->map.find(direct_path);
    if (src_it == ancestor->map.end() || !src_it->second)
      return std::make_shared<address_ref>(cloned_ancestor, string(get_path()));
    auto tmp_src = move(src_it->second);
    if (result_wrapper) {
      if (auto src_wrapper = std::dynamic_pointer_cast<wrapper>(tmp_src)) {
        result_wrapper->merge(src_wrapper, context);
      } else result = result_wrapper->map[""] = tmp_src->clone(context);
    } else result = tmp_src->clone(context);
    src_it->second = tmp_src;
    return std::make_shared<ref>(result);
  }
  return std::make_shared<address_ref>(cloned_ancestor, string(get_path()));
}

ref::ref(base_w v) : value(v) {
  start:
  auto val = value.lock();
  if (!val) THROW_ERROR(ancestor_destroyed, "ref::ref");
  if (auto meta = std::dynamic_pointer_cast<ref>(val)) {
    value = meta->value;
    goto start;
  }
}

string ref::get() const {
  auto val = value.lock();
  if (!val) THROW_ERROR(ancestor_destroyed, "ref::get");
  return val->get();
}

bool ref::set(const string& v) {
  auto val = value.lock();
  if (!val) THROW_ERROR(ancestor_destroyed, "ref::set");
  if (auto s = std::dynamic_pointer_cast<settable>(val))
    return s->set(v);
  return false;
}

base_s ref::clone(clone_context&) const {
  THROW_ERROR(clone, "node::ref can't be cloned");
}

NAMESPACE_END
