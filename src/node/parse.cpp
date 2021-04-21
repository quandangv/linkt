#include "parse.hpp"
#include "common.hpp"
#include "wrapper.hpp"
#include "token_iterator.hpp"

namespace node {

int parse_word_matcher(int c) {
  return c == '?' ? 2 : std::isspace(c) ? 0 : 1;
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

}
