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
    context.report_error("External dependency");
    return base_s();
  } else {
    cloned_ancestor = ancestor;
  }

  // Return a pointer to the referenced node
  if (context.optimize) {
    // If the referenced node already exists in the clone result, we don't have to clone it
    base_s result;
    auto tmp_ancestor = cloned_ancestor;
    for (auto& path : indirect_paths)
      if (!(tmp_ancestor = tmp_ancestor->get_wrapper(path)))
        goto no_existing;

    if (auto it = tmp_ancestor->map.find(direct_path); it != tmp_ancestor->map.end())
      result = it->second;
    if (auto result_wrapper = std::dynamic_pointer_cast<wrapper>(result))
      result = result_wrapper->map[""];

    if (!result) {
      no_existing:
      auto place = get_source_direct();
      if (place && *place) {
        auto tmp_place = move(*place);
        // Track the added ancestors to be removed after its done
        auto ancestors_mark = context.ancestors.size();
        for (auto& path : indirect_paths) {
          cloned_ancestor = cloned_ancestor->add_wrapper(path);
          if (!(ancestor = ancestor->get_wrapper(path)))
            THROW_ERROR(node, "Can't track source tree");
          context.ancestors.emplace_back(ancestor, cloned_ancestor);
        }
        auto& cloned_place = cloned_ancestor->map[direct_path];
        if (auto cloned_place_wrapper = std::dynamic_pointer_cast<wrapper>(cloned_place)) {
          if (auto place_wrapper = std::dynamic_pointer_cast<wrapper>(tmp_place)) {
            cloned_place_wrapper->merge(place_wrapper, context);
            result = cloned_place;
          } else result = cloned_place_wrapper->map[""] = tmp_place->clone(context);
        } else result = cloned_place = tmp_place->clone(context);
        context.ancestors.erase(context.ancestors.begin() + ancestors_mark, context.ancestors.end());
        *place = tmp_place;
        return std::make_shared<ref>(result);
      }
    } else
      return std::make_shared<ref>(result);
  }
  return std::make_shared<address_ref>(
    cloned_ancestor,
    string(get_path()));
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

base_s ref::clone(clone_context& context) const {
  context.report_error("node::ref can't be cloned");
  return base_s();
}

NAMESPACE_END
