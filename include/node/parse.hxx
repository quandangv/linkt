#include "common.hpp"
#include "node.hpp"
#include "wrapper.hpp"
#include "reference.hpp"
#include "token_iterator.hpp"

#include <array>

NAMESPACE(node)

// Parse an unescaped node string
template<class T> std::shared_ptr<base<T>>
parse_raw(parse_context& context, tstring& value) {
  trim_quotes(value);
  for (auto it = value.begin(); it < value.end() - 1; it++) {
    if (*it == '\\') {
      switch (*++it) {
        case 'n': value.replace(context.raw, it - value.begin() - 1, 2, "\n"); break;
        case 't': value.replace(context.raw, it - value.begin() - 1, 2, "\t"); break;
        case '\\': value.replace(context.raw, it - value.begin() - 1, 2, "\\"); break;
        case '$': --it; break;
        default: THROW_ERROR(parse, "Unknown escape sequence: \\" + string{*it});
      }
    }
  }
  size_t start, end;
  if (!find_enclosed(value, context.raw, "${", "{", "}", start, end)) {
    // There is no node inside the string, it's a plain string
    return std::make_shared<plain<T>>(value);
  } else if (start == 0 && end == value.size()) {
    // There is a single node inside, interpolation is unecessary
    value.erase_front(2);
    value.erase_back();
    return parse_escaped<T>(context, value);
  }
  if constexpr(std::is_same<T, string>::value) {
    // String interpolation
    std::stringstream ss;
    auto newval = std::make_shared<string_interpolate>();
    do {
      // Write the part we have moved past to the base string
      ss << substr(value, 0, start);

      // Make node from the token, skipping the brackets
      auto token = value.interval(start + 2, end - 1);
      auto replacement = parse_escaped<T>(context, token);
      if (replacement) {
        // Mark the position of the token in the base string
        newval->spots.emplace_back(int(ss.tellp()), replacement);
      }
      value.erase_front(end);
    } while (find_enclosed(value, context.raw, "${", "{", "}", start, end));
    ss << value;
    newval->base = ss.str();
    return newval;
  }
}

template<class T> std::shared_ptr<base<T>>
checked_parse_raw(parse_context& context, tstring& value) {
  auto result = parse_raw<T>(context, value);
  return result ?: THROW_ERROR(parse, "Unexpected empty parse result");
}

inline int parse_word_matcher(int c) {
  return c == '?' ? 2 : std::isspace(c) ? 0 : 1;
}

// Parse an escaped node string
template<class T> std::shared_ptr<base<T>>
parse_escaped(parse_context& context, tstring& value) {
  parse_preprocessed prep;
  prep.token_count = fill_tokens<parse_word_matcher>(value, prep.tokens);

  // Extract the fallback before anything else
  base_s fallback;
  for (int i = prep.token_count; i--> 0;) {
    if (!prep.tokens[i].empty() && prep.tokens[i].front() == '?') {
      prep.tokens[i].erase_front();
      auto last_element = prep.token_count - 1;
      prep.token_count = i;
      if (prep.tokens[i].empty() && i < last_element)
        i++;
      fallback = parse_raw<T>(context, prep.tokens[i].merge(prep.tokens[last_element]));
      break;
    }
  }

  auto make_operator = [&]()->base_s {
    if (prep.token_count == 0)
      THROW_ERROR(parse, "Empty reference string");
    if (prep.token_count == 1) {
      return std::make_shared<address_ref<string>>(context.parent_based_ref ? context.get_parent() : context.get_current(), prep.tokens[0]);

    } else if (prep.tokens[0] == "dep"_ts) {
      if (prep.token_count != 2)
        THROW_ERROR(parse, "env: Expected 1 components");
      return std::make_shared<address_ref<string>>(context.get_parent(), prep.tokens[1]);

    } else if (prep.tokens[0] == "rel"_ts) {
      if (prep.token_count != 2)
        THROW_ERROR(parse, "env: Expected 1 components");
      return std::make_shared<address_ref<string>>(context.get_current(), prep.tokens[1]);

    } else if (prep.tokens[0] == "file"_ts) {
      return std::make_shared<file>(parse_raw<T>(context, prep.tokens[prep.token_count - 1].merge(prep.tokens[1])), fallback);

    } else if (prep.tokens[0] == "cmd"_ts) {
      return std::make_shared<cmd>(parse_raw<T>(context, prep.tokens[prep.token_count - 1].merge(prep.tokens[1])), fallback);

    } else if (prep.tokens[0] == "env"_ts) {
      if (prep.token_count != 2)
        THROW_ERROR(parse, "env: Expected 1 components");
      return std::make_shared<env>(parse_raw<T>(context, prep.tokens[1]), fallback);

    } else if (prep.tokens[0] == "cache"_ts) {
      if (prep.token_count != 3)
        THROW_ERROR(parse, "cache: Expected 2 components");
      auto result = std::make_shared<cache>();
      result->duration_ms = checked_parse_raw<T>(context, prep.tokens[1]);
      result->source = checked_parse_raw<T>(context, prep.tokens[2]);
      return result;

    } else if (prep.tokens[0] == "clock"_ts) {
      if (prep.token_count != 4)
        THROW_ERROR(parse, "cache: Expected 3 components");
      auto result = std::make_shared<clock>();
      result->tick_duration = std::chrono::milliseconds(
          force_parse_ulong(prep.tokens[1].begin(), prep.tokens[1].size()));
      result->loop = force_parse_ulong(prep.tokens[2].begin(), prep.tokens[2].size());
      result->zero_point = steady_time(
          result->tick_duration * force_parse_ulong(prep.tokens[3].begin(), prep.tokens[3].size()));
      return result;

    } else if (prep.tokens[0] == "array_cache"_ts) {
      if (prep.token_count == 4) {
        auto result = std::make_shared<array_cache>();
        auto size = parse_ulong(prep.tokens[1].begin(), prep.tokens[1].size());
        if (size) {
          result->cache_arr = std::make_shared<std::vector<string>>(*size + 1);
          for (size_t i = 0; i < size; i++)
            result->cache_arr->emplace_back();
        } else {
          auto cache_base = address_ref<string>(context.get_parent(), prep.tokens[1]).get_source();
          if (auto cache = std::dynamic_pointer_cast<array_cache>(cache_base))
            result->cache_arr = cache->cache_arr;
          else THROW_ERROR(parse, "1st argument must be the size of the cache or a parent path to another array_cache: " + value);
        }
        result->source = checked_parse_raw<T>(context, prep.tokens[2]);
        result->calculator = checked_parse_raw<T>(context, prep.tokens[3]);
        return result;
      } else
        THROW_ERROR(parse, "array_cache: Expected 3 components");

    } else if (prep.tokens[0] == "save"_ts) {
      if (prep.token_count != 3)
        THROW_ERROR(parse, "save: Expected 2 components");
      auto result = std::make_shared<save>();
      result->target = std::make_shared<address_ref<string>>(context.get_current(), prep.tokens[1]);
      result->value = checked_parse_raw<T>(context, prep.tokens[2]);
      return result;

    } else if (prep.tokens[0] == "map"_ts) {
      if (prep.token_count != 4)
        THROW_ERROR(parse, "map: Expected 3 components");
      auto result = std::make_shared<map>(parse_raw<T>(context, prep.tokens[prep.token_count - 1]));
      if (auto min = cut_front(prep.tokens[1], ':'); !min.untouched())
        result->from_min = convert<float, strtof>(min);
      result->from_range = convert<float, strtof>(prep.tokens[1]) - result->from_min;

      if (auto min = cut_front(prep.tokens[2], ':'); !min.untouched())
        result->to_min = convert<float, strtof>(min);
      result->to_range = convert<float, strtof>(prep.tokens[2]) - result->to_min;
      return result;

    } else if (prep.tokens[0] == "color"_ts) {
      auto result = std::make_shared<color>(parse_raw<T>(context, prep.tokens[prep.token_count - 1]), fallback);
      if (prep.token_count > 2) {
        if (prep.token_count > 3)
          result->processor.inter = cspace::stospace(prep.tokens[1]);
        result->processor.add_modification(trim_quotes(prep.tokens[prep.token_count - 2]));
      }
      return result;

    } else if (prep.tokens[0] == "clone"_ts) {
      for (int i = 1; i < prep.token_count; i++) {
        auto source = address_ref<string>(context.get_parent(), prep.tokens[i]).get_source_direct();
        throwing_clone_context clone_context;
        if (!source || !*source)
          THROW_ERROR(parse, "Can't find node to clone");
        if (auto wrp = std::dynamic_pointer_cast<wrapper>(*source)) {
          context.get_current()->merge(wrp, clone_context);
        } else if (i == prep.token_count -1) {
          return (*source)->clone(clone_context);
        } else
          THROW_ERROR(parse, "Can't merge non-wrapper nodes");
      }
      return base_s();
    } else
      THROW_ERROR(parse, "Unsupported operator type: " + prep.tokens[0]);
  };
  auto op = make_operator();
  if (op && fallback)
    return std::make_shared<fallback_wrapper>(op, fallback);
  return op;
}

NAMESPACE_END
