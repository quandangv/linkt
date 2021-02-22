#include "node.hpp"
#include "wrapper.hpp"
#include "common.hpp"

#include <fstream>
#include <cstdlib>
#include <array>
#include <map>

NAMESPACE(lini::node)

string address_ref::get() const {
  auto result = ancestor.get_child(path);
  return result ? *result : use_fallback("Referenced key doesn't exist");
}

bool address_ref::set(const string& val) {
  auto src = ancestor.get_child_ptr(path);
  auto target = dynamic_cast<settable*>(src && *src ? src->get() : fallback ? fallback.get() : nullptr);
  if (target)
    return target->set(val);
  return false;
}

base_p clone(const base_p& src) {
  return src ? clone(*src) : base_p{};
}

base_p clone(const base& base_src, clone_handler handler) {
  if (auto src = dynamic_cast<const clonable*>(&base_src); src)
    return src->clone(handler);
  return handler(base_src);
}

base_p clone(const base_p& src, clone_handler handler) {
  return src ? clone(*src, handler) : base_p{};
}

base_p clone(const base& base_src) {
  std::map<const addable*, addable*> ancestors;
  clone_handler handler = [&](const base& base_src)->base_p {
    if (auto src = dynamic_cast<const addable*>(&base_src); src) {
      auto result = std::make_unique<wrapper>();
      ancestors.emplace(src, result.get());
      src->iterate_children([&](const string& name, const base_p& child) {
        result->add(name, clone(child, handler));
      });
      return move(result);
    }
    
    if (auto src = dynamic_cast<const address_ref*>(&base_src); src) {
      auto ancestor = ancestors.find(&src->ancestor);
      return std::make_unique<address_ref>(
        ancestor != ancestors.end() ? *ancestor->second : src->ancestor,
        string(src->path),
        clone(src->fallback, handler));
    }
    throw base::error("Node of type '" + string(typeid(base_src).name()) + "' can't be cloned");
  };
  return clone(base_src, handler);
}

string ref::get() const {
  if (src && *src) {
    return (*src)->get();
  }
  return use_fallback("Referenced key doesn't exist");
}

bool ref::set(const string& val) {
  settable* target = dynamic_cast<settable*>(*src ? src->get() : fallback ? fallback.get() : nullptr);
  if (target)
    return target->set(val);
  return false;
}

string defaultable::use_fallback(const string& msg) const {
  if (fallback)
    return fallback->get();
  throw base::error("Reference failed: " + msg + ". And no fallback was found");
}

base_p meta::copy(std::unique_ptr<meta>&& dest, clone_handler handler) const {
  if (value)
    dest->value = clone(*value, handler);
  if (fallback)
    dest->fallback = clone(*fallback, handler);
  return move(dest);
}

string color::get() const {
  try {
    auto result = processor.operate(value->get());
    if (result.empty() && fallback)
      return fallback->get();
    return result;
  } catch(const std::exception& e) {
    return use_fallback("Color processing failed, due to: " + string(e.what()));
  }
}

base_p color::clone(clone_handler handler) const {
  auto result = std::make_unique<color>();
  result->processor = processor;
  return meta::copy(move(result), handler);
}

string env::get() const {
  auto result = getenv(value->get().data());
  if (result == nullptr)
    return use_fallback("Environment variable not found: " + value->get());
  return string(result);
}

bool env::set(const string& newval) {
  setenv(value->get().data(), newval.data(), true);
  return true;
}

base_p env::clone(clone_handler handler) const {
  return meta::copy(std::make_unique<env>(), handler);
}

string file::get() const {
  std::ifstream ifs(value->get().data());
  if (ifs.fail())
    return use_fallback("Can't read file: " + value->get());
  string result(std::istreambuf_iterator<char>{ifs}, {});
  ifs.close();
  auto last_line = result.find_last_not_of("\r\n");
  result.erase(last_line + 1);
  return move(result);
}

bool file::set(const string& content) {
  std::ofstream ofs(value->get().data(), std::ios_base::trunc);
  if (ofs.fail())
    return false;
  ofs << content;
  ofs.close();
  return true;
}

base_p file::clone(clone_handler handler) const {
  return meta::copy(std::make_unique<file>(), handler);
}

string cmd::get() const {
  string result;
  try {
    auto file = popen(value->get().data(), "r");
    std::array<char, 128> buf;
    while (fgets(buf.data(), 128, file) != nullptr)
      result += buf.data();
  } catch (const std::exception& e) {
    use_fallback("Encountered error: " + string(e.what()));
  }
  auto last_line = result.find_last_not_of("\r\n");
  result.erase(last_line + 1);
  return result;
}

base_p cmd::clone(clone_handler handler) const {
  return meta::copy(std::make_unique<cmd>(), handler);
}

string map::get() const {
  auto str = value ? value->get() : use_fallback("Value key doesn't exist");
  size_t remaining;
  auto num =  std::stof(str, &remaining);
  if (remaining != str.size())
    throw std::invalid_argument("value is not a number");
  return std::to_string(to_min + to_range/from_range*(num - from_min));
}

base_p map::clone(clone_handler handler) const {
  auto result = std::make_unique<map>();
  result->from_min = from_min;
  result->from_range = from_range;
  result->to_min = to_min;
  result->to_range = to_range;
  return meta::copy(move(result), handler);
}

NAMESPACE_END
