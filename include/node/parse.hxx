#include "node.hpp"
#include "wrapper.hpp"
#include "reference.hpp"

#include <array>

namespace node {

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
        default: throw parse_error("Unknown escape sequence: \\" + string{*it});
      }
    }
  }
  size_t start, end;
  if (!find_enclosed(value, context.raw, "${", "{", "}", start, end)) {
    // There is no node inside the string, it's a plain string
    return parse_plain<plain<T>, T>(value);
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
  return std::shared_ptr<base<T>>();
}

  template<class T> std::shared_ptr<base<T>>
checked_parse_raw(parse_context& context, tstring& value) {
  auto result = parse_raw<T>(context, value);
  return result ?: throw parse_error("Unexpected empty parse result");
}

// Parse an escaped node string
  template<class T> std::shared_ptr<base<T>>
parse_escaped(parse_context& context, tstring& value) {
  parse_preprocessed prep;
  auto fb_str = prep.process(value);
  std::shared_ptr<base<T>> fallback;
  if (!fb_str.untouched())
    prep.fallback = fallback = parse_raw<T>(context, fb_str);

  auto merge_tokens = [&]()->tstring& {
    return prep.tokens[prep.token_count - 1].merge(prep.tokens[1]);
  };
  auto make_operator = [&]()->std::shared_ptr<base<T>> {
    if (prep.token_count == 0)
      throw parse_error("Empty reference string");
    if (prep.token_count == 1) {
      auto ancestor = context.parent_based_ref ? context.get_parent() : context.get_current();
      return std::make_shared<address_ref<T>>(ancestor, prep.tokens[0]);
    } else if (prep.tokens[0] == "dep"_ts) {
      return std::make_shared<address_ref<T>>(context.get_parent(), merge_tokens());
    } else if (prep.tokens[0] == "rel"_ts) {
      return std::make_shared<address_ref<T>>(context.get_current(), merge_tokens());

    } else if (prep.tokens[0] == "file"_ts) {
      if constexpr(std::is_same<T, string>::value)
        return std::make_shared<file>(parse_raw<T>(context, merge_tokens()), move(prep.fallback));
    } else if (prep.tokens[0] == "cmd"_ts) {
      if constexpr(std::is_same<T, string>::value)
        return std::make_shared<cmd>(parse_raw<T>(context, merge_tokens()), move(prep.fallback));
    } else if (prep.tokens[0] == "env"_ts) {
      if constexpr(std::is_same<T, string>::value)
        return std::make_shared<env>(parse_raw<T>(context, merge_tokens()), move(prep.fallback));
    } else if (prep.tokens[0] == "cache"_ts) {
      if constexpr(std::is_same<T, string>::value)
        return cache::parse(context, prep);

    } else if (prep.tokens[0] == "clock"_ts) {
      if constexpr(std::is_same<int, T>::value || std::is_same<string, T>::value)
        return clock::parse(context, prep);

    } else if (prep.tokens[0] == "array_cache"_ts) {
      if constexpr(std::is_same<T, string>::value)
        return array_cache::parse(context, prep);

    } else if (prep.tokens[0] == "save"_ts) {
      if constexpr(std::is_same<T, string>::value)
        return save::parse(context, prep);

    } else if (prep.tokens[0] == "map"_ts) {
      if constexpr(std::is_convertible<float, T>::value || std::is_same<string, T>::value)
        return map::parse(context, prep);

    } else if (prep.tokens[0] == "color"_ts) {
      if constexpr(std::is_same<T, string>::value)
        return color::parse(context, prep);

    } else if (prep.tokens[0] == "var"_ts) {
      if (prep.token_count == 2)
        return parse_plain<settable_plain<T>, T>(trim_quotes(prep.tokens[1]));
      else if (prep.token_count == 3) {
        if constexpr(std::is_same<T, string>::value) {
          if (prep.tokens[1] == "int"_ts)
            return parse_plain<settable_plain<int>, int>(trim_quotes(prep.tokens[2]));
          if (prep.tokens[1] == "float"_ts)
            return parse_plain<settable_plain<float>, float>(trim_quotes(prep.tokens[2]));
          throw parse_error("Parse.var: Invalid var type: " + prep.tokens[1]);
        }
        throw parse_error("Parse.var: Can only specify type when parsing to string");
      } else throw parse_error("Parse.var: Invalid token count");

    } else if (prep.tokens[0] == "clone"_ts) {
      for (int i = 1; i < prep.token_count; i++) {
        auto source = context.get_parent()->get_child_place(prep.tokens[i]);
        throwing_clone_context clone_context;
        if (!source || !*source)
          throw parse_error("Can't find node to clone");
        if (auto wrp = std::dynamic_pointer_cast<wrapper>(*source)) {
          context.get_current()->merge(wrp, clone_context);
        } else if (i == prep.token_count -1) {
          return checked_clone<T>(*source, clone_context, "parse.clone");
        } else
          throw parse_error("Can't merge non-wrapper nodes");
      }
      return {};
    }
    throw parse_error("Unsupported operator type: " + prep.tokens[0]);
  };
  auto op = make_operator();
  if (op && prep.fallback)
    return std::make_shared<fallback_wrapper<T>>(op, fallback);
  return op;
}

}
