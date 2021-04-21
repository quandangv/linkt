#include "base.hpp"
#include "common.hpp"
#include "wrapper.hpp"
#include "token_iterator.hpp"

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

  template<> unsigned long
parse<unsigned long>(const char* str, size_t len) {
  if (!str) throw node_error("trying to parse null");
  char* end;
  auto result = std::strtoul(str, &end, 10);
  if (end != str + len)
    throw std::logic_error("Parse to ulong failed: "s + str);
  return result;
}

  template<> int
parse<int>(const char* str, size_t len) {
  return parse<unsigned long>(str, len);
}

  template<> float
parse<float>(const char* str, size_t len) {
  if (!str) throw node_error("trying to parse null");
  char* end;
  auto result = std::strtof(str, &end);
  if (end != str + len)
    throw std::logic_error("Parse to float failed: "s + str);
  return result;
}

  template<> string
parse<string>(const char* str, size_t len) {
  if (!str) throw node_error("trying to parse null");
  return string(str, len);
}

NAMESPACE_END
