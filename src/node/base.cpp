#include "base.hpp"
#include "common.hpp"
#include "wrapper.hpp"
#include "token_iterator.hpp"

#include <map>

NAMESPACE(lini::node)

base_p clone(const base_p& src) {
  return src ? clone(*src) : base_p{};
}

base_p clone(const base& base_src, clone_handler handler) {
  auto src = dynamic_cast<const clonable*>(&base_src);
  return src ? src->clone(handler) : handler(base_src);
}

base_p clone(const base_p& src, clone_handler handler) {
  return src ? clone(*src, handler) : base_p{};
}

base_p clone(const base& base_src) {
  std::map<const addable*, addable*> ancestors;
  clone_handler handler = [&](const base& base_src)->base_p {
    if (auto src = dynamic_cast<const addable*>(&base_src); src) {
      auto result = std::make_shared<wrapper>();
      ancestors.emplace(src, result.get());
      src->iterate_children([&](const string& name, const base_p& child) {
        result->add(name, clone(child, handler));
      });
      return move(result);
    }
    if (auto src = dynamic_cast<const address_ref*>(&base_src); src) {
      auto ancestor = ancestors.find(&src->ancestor);
      return std::make_shared<address_ref>(
        ancestor != ancestors.end() ? *ancestor->second : src->ancestor,
        string(src->path),
        clone(src->fallback, handler));
    }
    throw base::error("Node of type '" + string(typeid(base_src).name()) + "' can't be cloned");
  };
  return clone(base_src, handler);
}

string defaultable::use_fallback(const string& msg) const {
  return (fallback ?: throw base::error("Failure: " + msg + ". And no fallback was found"))->get();
}

base_p meta::copy(std::unique_ptr<meta>&& dest, clone_handler handler) const {
  if (value)
    dest->value = clone(*value, handler);
  if (fallback)
    dest->fallback = clone(*fallback, handler);
  return move(dest);
}

base_p parse(string& raw, tstring& str, ref_maker rmaker) {
  base_p fallback;
  if (auto fb_str = cut_back(str, '?'); !fb_str.untouched())
    fallback = parse_string(raw, trim_quotes(fb_str), rmaker);

  std::array<tstring, 5> tokens;
  auto token_count = fill_tokens(str, tokens);
  auto make_meta = [&]<typename T>(const std::shared_ptr<T>& ptr) {
    ptr->fallback = move(fallback);
    ptr->value = parse_string(raw, trim_quotes(tokens[token_count - 1]), rmaker);
    return ptr;
  };
  if (token_count == 0)
    throw addable::error("parse: Empty reference conent");
  if (token_count == 1) {
    return rmaker(tokens[0], move(fallback));

  } else if (tokens[0] == "file"_ts) {
    tokens[token_count - 1].merge(tokens[1]);
    return make_meta(std::make_shared<file>());
    
  } else if (tokens[0] == "cmd"_ts) {
    tokens[token_count - 1].merge(tokens[1]);
    return make_meta(std::make_shared<cmd>());

  } else if (tokens[0] == "env"_ts) {
    if (token_count != 2)
      throw addable::error("parse: Expected 2 components");
    return make_meta(std::make_shared<env>());

  } else if (tokens[0] == "map"_ts) {
    if (token_count != 4)
      throw addable::error("parse.map: Expected 3 components");
    auto newval = std::make_shared<map>();
    if (auto min = cut_front(tokens[1], ':'); !min.untouched())
      newval->from_min = convert<float, strtof>(min);
    newval->from_range = convert<float, strtof>(tokens[1]) - newval->from_min;

    if (auto min = cut_front(tokens[2], ':'); !min.untouched())
      newval->to_min = convert<float, strtof>(min);
    newval->to_range = convert<float, strtof>(tokens[2]) - newval->to_min;
    return make_meta(newval);

  } else if (tokens[0] == "color"_ts) {
    auto newval = std::make_shared<color>();
    if (token_count > 2) {
      if (token_count > 3)
        newval->processor.inter = cspace::stospace(tokens[1]);
      newval->processor.add_modification(tokens[token_count - 2]);
    }
    return make_meta(newval);

  } else
    throw addable::error("Unsupported reference type: " + tokens[0]);
}

base_p parse_string(string& raw, tstring& str, ref_maker rmaker) {
  size_t start, end;
  if (!find_enclosed(str, raw, "${", "{", "}", start, end)) {
    // There is no node inside the string, it's a normal string
    return std::make_shared<plain>(str);
  } else if (start == 0 && end == str.size()) {
    // There is a single node inside, interpolation is unecessary
    str.erase_front(2);
    str.erase_back();
    return parse(raw, str, rmaker);
  }
  // String interpolation
  std::stringstream ss;
  auto newval = std::make_shared<string_interpolate>();
  do {
    // Write the part we have moved past to the base string
    ss << substr(str, 0, start);

    // Make node from the token, skipping the brackets
    auto token = str.interval(start + 2, end - 1);
    auto value = parse(raw, token, rmaker);
    if (value) {
      // Mark the position of the token in the base string
      newval->spots.push_back({int(ss.tellp()), "", move(value)});
    }
    str.erase_front(end);
  } while (find_enclosed(str, raw, "${", "{", "}", start, end));
  ss << str;
  newval->base = ss.str();
  return move(newval);
}

NAMESPACE_END
