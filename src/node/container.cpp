#include "container.hpp"
#include "common.hpp"

#include <array>

NAMESPACE(lini::node)

bool container::has_child(const tstring& path) const {
  auto ptr = get_child_ptr(path);
  return ptr && *ptr;
}

void container::iterate_children(std::function<void(const string&, const base&)> processor) const {
  iterate_children([&](const string& name, const base_p& child) {
    if (child)
      processor(name, *child);
  });
}

std::optional<string> container::get_child(const tstring& path) const {
  if (auto ptr = get_child_ptr(path); ptr) {
    if (auto& value = *ptr; value) {
      try {
        return value->get();
      } catch(const std::exception& e) {
        throw error("Exception while retrieving value of '" + path + "': " + e.what());
      }
    } else
      LG_INFO("contriner-get_child: failed due to value being null: " << path);
  } else
    LG_INFO("container-get_child: failed due to key not found: " << path);
  return {};
}

string container::get_child(const tstring& path, string&& fallback) const {
  if (auto result = get_child(path); result)
    return *result;
  return forward<string>(fallback);
}

base& container::get_child_ref(const tstring& path) const {
  auto ptr = get_child_ptr(path);
  if (ptr && *ptr)
    return **ptr;
  throw base::error("Key is empty");
}

bool container::set(const tstring& path, const string& value) {
  if (auto ptr = get_child_ptr(path); ptr)
    if (auto target = dynamic_cast<settable*>(ptr->get()); target)
      return target->set(value);
  return false;
}

base_pp addable::add(tstring path, string& raw, tstring value) {
  auto node = parse_string(raw, value, [&](tstring& ts, base_p&& fallback) { return make_address_ref(ts, move(fallback)); });
  if (node)
    return add(path, move(node));
  return {};
}

base_pp addable::add(tstring path, string raw) {
  tstring value(raw);
  return add(path, raw, value);
}

base_p addable::make_ref(const tstring& ts, base_p&& fallback) {
  auto ptr = get_child_ptr(ts);
  if (!ptr)
    ptr = add(ts, base_p{});
  return std::make_unique<ref>(ptr, move(fallback));
}

base_p addable::make_address_ref(const tstring& ts, base_p&& fallback) {
  return std::make_unique<address_ref>(*this, ts, move(fallback));
}

NAMESPACE_END
