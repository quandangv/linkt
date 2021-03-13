#include "parse.hpp"
#include "common.hpp"
#include "node.hpp"
#include "wrapper.hpp"
#include "reference.hpp"
#include "token_iterator.hpp"

#include <array>

NAMESPACE(node)

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

// Parse an unescaped node string
base_s parse_raw(string& raw, tstring& str, parse_context& context) {
  for (auto it = str.begin(); it < str.end() - 1; it++) {
    if (*it == '\\') {
      switch (*++it) {
        case 'n': str.replace(raw, it - str.begin() - 1, 2, "\n"); break;
        case 't': str.replace(raw, it - str.begin() - 1, 2, "\t"); break;
        case '\\': str.replace(raw, it - str.begin() - 1, 2, "\\"); break;
        case '$': --it; break;
        default: THROW_ERROR(parse, "Unknown escape sequence: \\" + string{*it});
      }
    }
  }
  size_t start, end;
  if (!find_enclosed(str, raw, "${", "{", "}", start, end)) {
    // There is no node inside the string, it's a plain string
    return std::make_shared<plain>(str);
  } else if (start == 0 && end == str.size()) {
    // There is a single node inside, interpolation is unecessary
    str.erase_front(2);
    str.erase_back();
    return parse_escaped(raw, str, context);
  }
  // String interpolation
  std::stringstream ss;
  auto newval = std::make_shared<string_interpolate>();
  do {
    // Write the part we have moved past to the base string
    ss << substr(str, 0, start);

    // Make node from the token, skipping the brackets
    auto token = str.interval(start + 2, end - 1);
    auto value = parse_escaped(raw, token, context);
    if (value) {
      // Mark the position of the token in the base string
      newval->spots.emplace_back(int(ss.tellp()), value);
    }
    str.erase_front(end);
  } while (find_enclosed(str, raw, "${", "{", "}", start, end));
  ss << str;
  newval->base = ss.str();
  return newval;
}

int word_matcher(int c) {
  return c == '?' ? 2 : std::isspace(c) ? 0 : 1;
}

base_s checked_parse_raw(string& raw, tstring& str, parse_context& context) {
  auto result = parse_raw(raw, str, context);
  return result ?: THROW_ERROR(parse, "Unexpected empty parse result");
}

// Parse an escaped node string
base_s parse_escaped(string& raw, tstring& str, parse_context& context) {
  std::array<tstring, 7> tokens;
  auto token_count = fill_tokens<word_matcher>(str, tokens);

  // Extract the fallback before anything else
  base_s fallback;
  for (int i = token_count; i--> 0;) {
    if (!tokens[i].empty() && tokens[i].front() == '?') {
      tokens[i].erase_front();
      auto last_element = token_count - 1;
      token_count = i;
      if (tokens[i].empty() && i < last_element)
        i++;
      tokens[i].merge(tokens[last_element]);
      fallback = parse_raw(raw, tokens[i], context);
    } else trim_quotes(tokens[i]);
  }

  // Finalize nodes that derive from node::meta
  auto make_meta = [&]<typename T>() {
    std::shared_ptr<T> result = std::make_shared<T>(checked_parse_raw(raw, tokens[token_count - 1], context));
    result->fallback = move(fallback);
    return result;
  };
  auto make_operator = [&]()->base_s {
    if (token_count == 0)
      THROW_ERROR(parse, "Empty reference string");
    if (token_count == 1) {
      return std::make_shared<address_ref>(context.parent_based_ref ? context.get_parent() : context.get_current(), tokens[0]);

    } else if (tokens[0] == "dep"_ts) {
      if (token_count != 2)
        THROW_ERROR(parse, "env: Expected 1 components");
      return std::make_shared<address_ref>(context.get_parent(), tokens[1]);

    } else if (tokens[0] == "rel"_ts) {
      if (token_count != 2)
        THROW_ERROR(parse, "env: Expected 1 components");
      return std::make_shared<address_ref>(context.get_current(), tokens[1]);

    } else if (tokens[0] == "file"_ts) {
      tokens[token_count - 1].merge(tokens[1]);
      return make_meta.operator()<file>();

    } else if (tokens[0] == "cmd"_ts) {
      tokens[token_count - 1].merge(tokens[1]);
      return make_meta.operator()<cmd>();

    } else if (tokens[0] == "env"_ts) {
      if (token_count != 2)
        THROW_ERROR(parse, "env: Expected 1 components");
      return make_meta.operator()<env>();

    } else if (tokens[0] == "cache"_ts) {
      if (token_count != 3)
        THROW_ERROR(parse, "cache: Expected 2 components");
      auto result = std::make_shared<cache>();
      result->duration_ms = checked_parse_raw(raw, tokens[1], context);
      result->source = checked_parse_raw(raw, tokens[2], context);
      return result;

    } else if (tokens[0] == "clock"_ts) {
      if (token_count != 4)
        THROW_ERROR(parse, "cache: Expected 2 components");
      auto result = std::make_shared<clock>();
      auto tmp = force_parse_ulong(tokens[1].begin(), tokens[1].size());
      result->tick_duration = std::chrono::milliseconds(tmp);
      tmp = force_parse_ulong(tokens[2].begin(), tokens[2].size());
      result->loop = tmp;
      tmp = force_parse_ulong(tokens[3].begin(), tokens[3].size());
      result->zero_point = steady_time(result->tick_duration * tmp);
      return result;

    } else if (tokens[0] == "array_cache"_ts) {
      if (token_count == 4) {
        auto result = std::make_shared<array_cache>();
        auto size = parse_ulong(tokens[1].begin(), tokens[1].size());
        if (size) {
          result->cache_arr = std::make_shared<std::vector<string>>(*size + 1);
          for (size_t i = 0; i < size; i++)
            result->cache_arr->emplace_back();
        } else {
          auto cache_base = address_ref(context.get_parent(), tokens[1]).get_source();
          if (auto cache = std::dynamic_pointer_cast<array_cache>(cache_base))
            result->cache_arr = cache->cache_arr;
          else THROW_ERROR(parse, "1st argument must be the size of the cache or a parent path to another array_cache: " + str);
        }
        result->source = checked_parse_raw(raw, tokens[2], context);
        result->calculator = checked_parse_raw(raw, tokens[3], context);
        return result;
      } else
        THROW_ERROR(parse, "array_cache: Expected 3 components");

    } else if (tokens[0] == "save"_ts) {
      if (token_count != 3)
        THROW_ERROR(parse, "save: Expected 2 components");
      auto result = std::make_shared<save>();
      result->target = std::make_shared<address_ref>(context.get_current(), tokens[1]);
      result->value = checked_parse_raw(raw, tokens[2], context);
      return result;

    } else if (tokens[0] == "map"_ts) {
      if (token_count != 4)
        THROW_ERROR(parse, "map: Expected 3 components");
      auto result = make_meta.operator()<map>();
      if (auto min = cut_front(tokens[1], ':'); !min.untouched())
        result->from_min = convert<float, strtof>(min);
      result->from_range = convert<float, strtof>(tokens[1]) - result->from_min;

      if (auto min = cut_front(tokens[2], ':'); !min.untouched())
        result->to_min = convert<float, strtof>(min);
      result->to_range = convert<float, strtof>(tokens[2]) - result->to_min;
      return result;

    } else if (tokens[0] == "color"_ts) {
      auto result = make_meta.operator()<color>();
      if (token_count > 2) {
        if (token_count > 3)
          result->processor.inter = cspace::stospace(tokens[1]);
        result->processor.add_modification(tokens[token_count - 2]);
      }
      return result;

    } else if (tokens[0] == "clone"_ts) {
      for (int i = 1; i < token_count; i++) {
        auto source = address_ref(context.get_parent(), tokens[i]).get_source_direct();
        throwing_clone_context clone_context;
        if (!source || !*source)
          THROW_ERROR(parse, "Can't find node to clone");
        if (auto wrp = std::dynamic_pointer_cast<wrapper>(*source)) {
          context.get_current()->merge(wrp, clone_context);
        } else if (i == token_count -1) {
          return (*source)->clone(clone_context);
        } else
          THROW_ERROR(parse, "Can't merge non-wrapper nodes");
      }
      return base_s();
    } else
      THROW_ERROR(parse, "Unsupported operator type: " + tokens[0]);
  };
  auto op = make_operator();
  if (op && fallback)
    return std::make_shared<fallback_wrapper>(op, fallback);
  return op;
}

NAMESPACE_END
