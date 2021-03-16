#include "common.hpp"
#include "node.hpp"
#include "wrapper.hpp"
#include "reference.hpp"
#include "token_iterator.hpp"

#include <array>

NAMESPACE(node)

// Parse an unescaped node string
template<class T> std::shared_ptr<base<T>>
parse_raw(parse_context& context) {
  auto& val = context.value;
  trim_quotes(val);
  for (auto it = val.begin(); it < val.end() - 1; it++) {
    if (*it == '\\') {
      switch (*++it) {
        case 'n': val.replace(context.raw, it - val.begin() - 1, 2, "\n"); break;
        case 't': val.replace(context.raw, it - val.begin() - 1, 2, "\t"); break;
        case '\\': val.replace(context.raw, it - val.begin() - 1, 2, "\\"); break;
        case '$': --it; break;
        default: THROW_ERROR(parse, "Unknown escape sequence: \\" + string{*it});
      }
    }
  }
  size_t start, end;
  if (!find_enclosed(val, context.raw, "${", "{", "}", start, end)) {
    // There is no node inside the string, it's a plain string
    return std::make_shared<plain<T>>(val);
  } else if (start == 0 && end == val.size()) {
    // There is a single node inside, interpolation is unecessary
    val.erase_front(2);
    val.erase_back();
    return parse_escaped<T>(context);
  }
  if constexpr(std::is_same<T, string>::value) {
    auto str = val;
    // String interpolation
    std::stringstream ss;
    auto newval = std::make_shared<string_interpolate>();
    do {
      // Write the part we have moved past to the base string
      ss << substr(str, 0, start);

      // Make node from the token, skipping the brackets
      val = str.interval(start + 2, end - 1);
      auto value = parse_escaped<T>(context);
      if (value) {
        // Mark the position of the token in the base string
        newval->spots.emplace_back(int(ss.tellp()), value);
      }
      str.erase_front(end);
    } while (find_enclosed(str, context.raw, "${", "{", "}", start, end));
    ss << str;
    newval->base = ss.str();
    return newval;
  }
}

template<class T> std::shared_ptr<base<T>>
checked_parse_raw(parse_context& context) {
  auto result = parse_raw<T>(context);
  return result ?: THROW_ERROR(parse, "Unexpected empty parse result");
}

inline int parse_word_matcher(int c) {
  return c == '?' ? 2 : std::isspace(c) ? 0 : 1;
}

// Parse an escaped node string
template<class T> std::shared_ptr<base<T>>
parse_escaped(parse_context& context) {
  std::array<tstring, 7> tokens;
  tstring str = context.value;
  auto token_count = fill_tokens<parse_word_matcher>(str, tokens);

  // Extract the fallback before anything else
  base_s fallback;
  for (int i = token_count; i--> 0;) {
    if (!tokens[i].empty() && tokens[i].front() == '?') {
      tokens[i].erase_front();
      auto last_element = token_count - 1;
      token_count = i;
      if (tokens[i].empty() && i < last_element)
        i++;
      context.value = tokens[i].merge(tokens[last_element]);
      fallback = parse_raw<T>(context);
      break;
    }
  }

  auto make_operator = [&]()->base_s {
    if (token_count == 0)
      THROW_ERROR(parse, "Empty reference string");
    if (token_count == 1) {
      return std::make_shared<address_ref<string>>(context.parent_based_ref ? context.get_parent() : context.get_current(), tokens[0]);

    } else if (tokens[0] == "dep"_ts) {
      if (token_count != 2)
        THROW_ERROR(parse, "env: Expected 1 components");
      return std::make_shared<address_ref<string>>(context.get_parent(), tokens[1]);

    } else if (tokens[0] == "rel"_ts) {
      if (token_count != 2)
        THROW_ERROR(parse, "env: Expected 1 components");
      return std::make_shared<address_ref<string>>(context.get_current(), tokens[1]);

    } else if (tokens[0] == "file"_ts) {
      context.value = tokens[token_count - 1].merge(tokens[1]);
      return std::make_shared<file>(parse_raw<T>(context), fallback);

    } else if (tokens[0] == "cmd"_ts) {
      context.value = tokens[token_count - 1].merge(tokens[1]);
      return std::make_shared<cmd>(parse_raw<T>(context), fallback);

    } else if (tokens[0] == "env"_ts) {
      if (token_count != 2)
        THROW_ERROR(parse, "env: Expected 1 components");
      context.value = tokens[1];
      return std::make_shared<env>(parse_raw<T>(context), fallback);

    } else if (tokens[0] == "cache"_ts) {
      if (token_count != 3)
        THROW_ERROR(parse, "cache: Expected 2 components");
      auto result = std::make_shared<cache>();
      context.value = tokens[1];
      result->duration_ms = checked_parse_raw<T>(context);
      context.value = tokens[2];
      result->source = checked_parse_raw<T>(context);
      return result;

    } else if (tokens[0] == "clock"_ts) {
      if (token_count != 4)
        THROW_ERROR(parse, "cache: Expected 3 components");
      auto result = std::make_shared<clock>();
      result->tick_duration = std::chrono::milliseconds(
          force_parse_ulong(tokens[1].begin(), tokens[1].size()));
      result->loop = force_parse_ulong(tokens[2].begin(), tokens[2].size());
      result->zero_point = steady_time(
          result->tick_duration * force_parse_ulong(tokens[3].begin(), tokens[3].size()));
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
          auto cache_base = address_ref<string>(context.get_parent(), tokens[1]).get_source();
          if (auto cache = std::dynamic_pointer_cast<array_cache>(cache_base))
            result->cache_arr = cache->cache_arr;
          else THROW_ERROR(parse, "1st argument must be the size of the cache or a parent path to another array_cache: " + str);
        }
        context.value = tokens[2];
        result->source = checked_parse_raw<T>(context);
        context.value = tokens[3];
        result->calculator = checked_parse_raw<T>(context);
        return result;
      } else
        THROW_ERROR(parse, "array_cache: Expected 3 components");

    } else if (tokens[0] == "save"_ts) {
      if (token_count != 3)
        THROW_ERROR(parse, "save: Expected 2 components");
      auto result = std::make_shared<save>();
      result->target = std::make_shared<address_ref<string>>(context.get_current(), tokens[1]);
      context.value = tokens[2];
      result->value = checked_parse_raw<T>(context);
      return result;

    } else if (tokens[0] == "map"_ts) {
      if (token_count != 4)
        THROW_ERROR(parse, "map: Expected 3 components");
      context.value = tokens[token_count - 1];
      auto result = std::make_shared<map>(parse_raw<T>(context));
      if (auto min = cut_front(tokens[1], ':'); !min.untouched())
        result->from_min = convert<float, strtof>(min);
      result->from_range = convert<float, strtof>(tokens[1]) - result->from_min;

      if (auto min = cut_front(tokens[2], ':'); !min.untouched())
        result->to_min = convert<float, strtof>(min);
      result->to_range = convert<float, strtof>(tokens[2]) - result->to_min;
      return result;

    } else if (tokens[0] == "color"_ts) {
      context.value = tokens[token_count - 1];
      auto result = std::make_shared<color>(parse_raw<T>(context), fallback);
      if (token_count > 2) {
        if (token_count > 3)
          result->processor.inter = cspace::stospace(tokens[1]);
        result->processor.add_modification(trim_quotes(tokens[token_count - 2]));
      }
      return result;

    } else if (tokens[0] == "clone"_ts) {
      for (int i = 1; i < token_count; i++) {
        auto source = address_ref<string>(context.get_parent(), tokens[i]).get_source_direct();
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
