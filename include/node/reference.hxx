#include "wrapper.hpp"

#include <sstream>

namespace node {

  template<class T>
address_ref<T>::address_ref(std::weak_ptr<wrapper> ancestor, tstring path)
    : ancestor_w(ancestor), indirect_paths() {
  trim(path);
  for (tstring indirect; !(indirect = cut_front(path, '.')).untouched();)
    indirect_paths.emplace_back(indirect);
  direct_path = path;
}

  template<class T> string
address_ref<T>::get_path() const {
  std::stringstream ss;
  for (auto& path : indirect_paths)
    ss << path << '.';
  ss << direct_path;
  return ss.str();
}

  template<class T> base_s
address_ref<T>::get_source() const {
  auto direct = get_source_direct();
  if (!direct || !*direct) {
    return {};
  }
  auto direct_wrapper = std::dynamic_pointer_cast<wrapper>(*direct);
  return direct_wrapper ? direct_wrapper->map[""] : *direct;
}

  template<class T> base_s*
address_ref<T>::get_source_direct() const {
  auto ancestor = ancestor_w.lock();
  if (!ancestor) throw ancestor_destroyed_error("address_ref(" + get_path() + ")::get_source_direct");
  for (auto& path : indirect_paths)
    if (!(ancestor = ancestor->get_wrapper(path)))
      return nullptr;

  if (auto it = ancestor->map.find(direct_path); it != ancestor->map.end())
    return &it->second;
  return nullptr;
}

  template<class T>
address_ref<T>::operator T() const {
  try {
    auto src = get_source();
    if (!src) throw node_error("Get: Referenced key not found: " + get_path());
    if (auto convert = std::dynamic_pointer_cast<base<T>>(src))
      return convert->operator T();
    return parse<T>(src->get(), "address_ref::operator T");
  } catch (const std::exception& e) {
    throw node_error("In " + get_path() + ": " + e.what());
  }
}

  template<class T> bool
address_ref<T>::set(const T& val) {
  auto src = get_source();
  if (!src) return false;
  if (auto target = std::dynamic_pointer_cast<settable<T>>(src))
    return target->set(val);
  if constexpr (!std::is_same<T, string>::value)
    if (auto target = std::dynamic_pointer_cast<settable<string>>(src))
      return target->set(std::to_string(val));
  return false;
}

  template<class T> base_s
address_ref<T>::clone(clone_context& context) const {
  auto ancestor = ancestor_w.lock();
  if (!ancestor) throw ancestor_destroyed_error("clone");
  // Find the corresponding ancestor in the clone result tree
  auto ancestor_it = find_if(context.ancestors.rbegin(), context.ancestors.rend(), [&](auto& pair) { return pair.first == ancestor; });
  wrapper_s cloned_ancestor;
  if (ancestor_it != context.ancestors.rend()) {
    cloned_ancestor = ancestor_it->second;
  } else if (context.no_dependency) {
    throw clone_error("Address_ref: External dependency");
  } else {
    cloned_ancestor = ancestor;
  }

  // Return a pointer to the referenced node
  if (context.optimize) {
    // If the referenced node already exists in the clone result, we don't have to clone it
    for (auto& path : indirect_paths) {
      cloned_ancestor = cloned_ancestor->add_wrapper(path);
      if (!(ancestor = ancestor->get_wrapper(path)))
        return std::make_shared<address_ref<T>>(cloned_ancestor, string(get_path()));
      context.ancestors.emplace_back(ancestor, cloned_ancestor);
    }

    base_s result;
    {
      auto& cloned = cloned_ancestor->map[direct_path];
      auto cloned_wrapper = std::dynamic_pointer_cast<wrapper>(cloned);
      if (cloned_wrapper) {
        if (result = cloned_wrapper->map[""]) {
          goto return_result;
        }
      } else if (result = cloned) {
        goto return_result;
      }

      auto src_it = ancestor->map.find(direct_path);
      if (src_it == ancestor->map.end() || !src_it->second)
        return std::make_shared<address_ref<T>>(cloned_ancestor, string(get_path()));
      auto tmp_src = move(src_it->second);
      if (cloned_wrapper) {
        if (auto src_wrapper = std::dynamic_pointer_cast<wrapper>(tmp_src)) {
          cloned_wrapper->merge(src_wrapper, context);
        } else cloned = cloned_wrapper->map[""] = tmp_src->clone(context);
      } else cloned = tmp_src->clone(context);
      src_it->second = tmp_src;
      result = cloned;
    }
    return_result:
    if (!result) throw clone_error("result is null");
    if (auto converted = std::dynamic_pointer_cast<base<T>>(result))
      return std::make_shared<ref<T>>(converted);
    if constexpr(!std::is_same<T, string>::value)  // Always true
      return std::make_shared<adapter<T>>(result);
  }
  return std::make_shared<address_ref<T>>(cloned_ancestor, string(get_path()));
}

  template<class T> bool
address_ref<T>::is_fixed() const {
  auto src = get_source();
  if (!src) throw node_error("is_fixed: Referenced key not found: " + get_path());
  return src->is_fixed();
}

  template<class T>
ref<T>::ref(std::weak_ptr<base<T>> source_w) : source_w(source_w) {
  start:
  auto source = source_w.lock();
  if (!source) throw ancestor_destroyed_error("ancestor_destroyed_error: ref::ref");
  if (auto meta = std::dynamic_pointer_cast<ref<T>>(source)) {
    source_w = meta->source_w;
    goto start;
  }
}

  template<class T>
ref<T>::operator T() const {
  auto source = source_w.lock();
  if (!source) throw ancestor_destroyed_error("ancestor_destroyed_error: ref::get");
  return source->operator T();
}

  template<class T> bool
ref<T>::set(const T& value) {
  auto source = this->source_w.lock();
  if (!source) throw ancestor_destroyed_error("ancestor_destroyed_error: ref::set");
  if (auto s = std::dynamic_pointer_cast<settable<T>>(source))
    return s->set(value);
  return false;
}

  template<class T> base_s
ref<T>::clone(clone_context&) const {
  throw clone_error("Ref: is optimized and can't be cloned further");
}

inline base_s upref::clone(clone_context& context) const {
  auto ancestor = source_w.lock();
  if (!ancestor) throw ancestor_destroyed_error("clone");
  // Find the corresponding ancestor in the clone result tree
  auto ancestor_it = find_if(context.ancestors.rbegin(), context.ancestors.rend(), [&](auto& pair) { return pair.first == ancestor; });
  if (ancestor_it != context.ancestors.rend()) {
    return std::make_shared<upref>(ancestor_it->second);
  } else if (context.no_dependency) {
    throw clone_error("Ref: Can't find the cloned ancestor");
  } else {
    return std::make_shared<upref>(ancestor);
  }
}

  template<class T> bool
ref<T>::is_fixed() const {
  auto source = this->source_w.lock();
  if (!source) throw ancestor_destroyed_error("ancestor_destroyed_error: ref::is_fixed");
  return source->is_fixed();
}

  template<class T> bool
adapter<T>::set(const T& value) {
  auto source = source_w.lock();
  if (!source) return false;
  if (auto target = std::dynamic_pointer_cast<settable<T>>(source))
    return target->set(value);
  if constexpr (!std::is_same<T, string>::value)
    if (auto target = std::dynamic_pointer_cast<settable<string>>(source))
      return target->set(std::to_string(value));
  return false;
}

  template<class T>
adapter<T>::operator T() const {
  auto str = ref<string>::operator string();
  return parse<T>(str, "adapter::operator T");
}

}
