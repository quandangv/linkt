#include "base.hpp"
#include "common.hpp"
#include "wrapper.hpp"

#include <sstream>

NAMESPACE(node)

throwing_clone_context::~throwing_clone_context()  noexcept(false) {
  if (std::uncaught_exceptions() || errors.empty())
    return;
  // Throw an exception containing all the errors
  std::stringstream ss;
  for(auto& err : errors)
    ss << err.first << ": " << err.second << '\n';
  THROW_ERROR(node, "Errors while cloning: \n" + ss.str());
}

// Use in text parsing, separate the key and the content using a separator character
// Reports an error if the separator character isn't found
bool errorlist::extract_key(tstring& line, int linecount, char separator, tstring& key) {
  key = cut_front(line, separator);
  if (key.untouched())
    return report_error(linecount, "Line ignored: " + line), false;
  return true;
}

wrapper_s parse_context::get_current() {
  if (current)
    return current;
  if (!place)
    THROW_ERROR(parse, "Get-current: Both current and place are null");
  if ((current = std::dynamic_pointer_cast<wrapper>(*place)))
    return current;
  current = wrapper::wrap(*place);
  place = nullptr;
  return current;
}

wrapper_s parse_context::get_parent() {
  if (parent)
    return parent;
  THROW_ERROR(parse, "parent is null");
}

base_s& parse_context::get_place() {
  if (!place) {
    if (!current)
      THROW_ERROR(parse, "Get-place: Both current and place are null");
    place = &current->map[""];
  }
  if (auto wrp = dynamic_cast<wrapper*>(place->get()))
    place = &wrp->map[""];
  return *place ? THROW_ERROR(parse, "get_place: Duplicate key") : *place;
}

// Checks if the value of a node come directly from a plain node, meaning it never changes
bool is_fixed(base_s node) {
  if (auto doc = dynamic_cast<wrapper*>(node.get()))
    node = doc->get_child_ptr(""_ts);
  return dynamic_cast<fixed*>(node.get());
}

NAMESPACE_END
