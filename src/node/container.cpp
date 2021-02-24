#include "container.hpp"
#include "common.hpp"

#include <array>

NAMESPACE(lini::node)

void container::iterate_children(std::function<void(const string&, const base&)> processor) const {
  iterate_children([&](const string& name, const base_p& child) {
    if (child)
      processor(name, *child);
  });
}

std::optional<string> container::get_child(const tstring& path) const {
  auto ptr = get_child_ptr(path);
  return ptr ? ptr->get() : std::optional<string>{};
}

string container::get_child(const tstring& path, string&& fallback) const {
  auto result = get_child(path);
  return result ? *result : move(fallback);
}

base& container::get_child_ref(const tstring& path) const {
  return *(get_child_ptr(path).get() ?: throw base::error("Key is empty"));
}

bool container::set(const tstring& path, const string& value) {
  auto target = dynamic_cast<settable*>(get_child_ptr(path).get());
  return target ? target->set(value) : false;
}

base_p addable::add(tstring path, string& raw, tstring value) {
  return add(path, parse_string(raw, value, [&](tstring& ts, const base_p& fallback) { return make_address_ref(ts, fallback); }));
}

base_p addable::add(tstring path, string raw) {
  return add(path, raw, tstring(raw));
}

std::shared_ptr<ref> addable::make_address_ref(const tstring& ts, const base_p& fallback) {
  return std::make_unique<address_ref>(*this, ts, move(fallback));
}

NAMESPACE_END
