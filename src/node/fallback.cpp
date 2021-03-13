#include "fallback.hpp"
#include "common.hpp"

NAMESPACE(node)

// Returns the value of the fallback if available. Otherwise throws an error
string with_fallback::use_fallback(const string& msg) const {
  return (fallback ?: THROW_ERROR(node, "Failure: " + msg + ". No fallback was found"))->get();
}

fallback_wrapper::fallback_wrapper(base_s value, base_s fallback) :
    with_fallback(fallback), value(value) {
  if (!fallback || !value)
    THROW_ERROR(required_field_null, "fallback_wrapper::fallback_wrapper");
}

string fallback_wrapper::get() const {
  try {
    return value->get();
  } catch (const std::exception& e) {
    return fallback->get();
  }
}

bool fallback_wrapper::set(const string& str) {
  if (auto value_settable = std::dynamic_pointer_cast<settable>(value))
    if (value_settable->set(str))
      return true;
  if (auto fallback_settable = std::dynamic_pointer_cast<settable>(fallback))
    return fallback_settable->set(str);
  return false;
}

base_s fallback_wrapper::clone(clone_context& context) const {
  return std::make_shared<fallback_wrapper>(value->clone(context), fallback->clone(context));
}

NAMESPACE_END
