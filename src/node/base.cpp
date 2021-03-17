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

int parse_word_matcher(int c) {
  return c == '?' ? 2 : std::isspace(c) ? 0 : 1;
}

tstring parse_preprocessed::process(tstring& value) {
  token_count = fill_tokens<parse_word_matcher>(value, tokens);
  // Extract the fallback before anything else
  for (int i = token_count; i--> 0;) {
    if (!tokens[i].empty() && tokens[i].front() == '?') {
      tokens[i].erase_front();
      auto last_element = token_count - 1;
      token_count = i;
      if (tokens[i].empty() && i < last_element)
        i++;
      return tokens[i].merge(tokens[last_element]);
    }
  }
  return tstring();
}

template<> unsigned long parse<unsigned long>(const char* str, size_t len) {
  char* end;
  auto result = std::strtoul(str, &end, 10);
  if (end != str + len)
    throw std::logic_error("Parse to ulong failed: "s + str);
  return result;
}

template<> int parse<int>(const char* str, size_t len) {
  return parse<unsigned long>(str, len);
}

template<> string parse<string>(const char* str, size_t len) {
  return string(str, len);
}

NAMESPACE_END
