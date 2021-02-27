#include "base.hpp"
#include "node.hpp"
#include "common.hpp"
#include "wrapper.hpp"
#include "token_iterator.hpp"

#include <map>

NAMESPACE(lini::node)

base_p clone(const base& base_src, clone_handler handler, clone_mode mode) {
  auto src = dynamic_cast<const clonable*>(&base_src);
  return src ? src->clone(handler, mode) : handler(base_src);
}

base_p clone(const base_p& src, clone_handler handler, clone_mode mode) {
  return src ? clone(*src, handler, mode) : base_p{};
}

base_p clone(const base_p& src, clone_mode mode) {
  return src ? clone(*src, mode) : base_p{};
}

base_p clone(const base& base_src, clone_mode mode) {
  std::vector<std::pair<const wrapper*, wrapper*>> ancestors;
  clone_handler handler = [&](const base& base_src)->base_p {
    if (auto src = dynamic_cast<const wrapper*>(&base_src); src) {
      auto result = std::make_shared<wrapper>();
      ancestors.emplace_back(src, result.get());
      result->value = clone(src->value, handler, mode);
      src->iterate_children([&](const string& name, const base_p& child) {
        LG_DBUG("Add child: " << name);
        if (auto& place = result->add(name); !place)
          place = clone(child, handler, mode);
        LG_DBUG("End child: " << name << " " << result->get_child_ptr(name).get());
      });
      ancestors.pop_back();
      return result ?: throw base::error("Empty wrapper clone result");
    }

    if (auto src = dynamic_cast<const address_ref*>(&base_src); src) {
      auto ancestor_it = find_if(ancestors.rbegin(), ancestors.rend(), [&](auto& pair) { return pair.first == &src->ancestor; });
      auto ancestor = ancestor_it != ancestors.rend() ? ancestor_it->second :
          !(bool)(mode & clone_mode::no_dependency) ? &src->ancestor :
          throw base::error("External dependency");

      if ((int)(mode & clone_mode::optimize)) {
        auto result = ancestor->get_child_ptr(src->path);
        if (auto wrpr = dynamic_cast<wrapper*>(result.get()); wrpr)
          result = wrpr->value;
        if (!result) {
          // This will recursively dereference chain references.
          auto src_ancestor = ancestor_it->first;
          auto ancestors_mark = ancestors.size();
          ancestor_processor source_tracer = [&](tstring& path, wrapper* inner_ancestor)->void {
            src_ancestor = dynamic_cast<wrapper*>(src_ancestor->get_child_ptr(path).get())
                ?: throw base::error("Invalid reference");
            ancestors.emplace_back(src_ancestor, inner_ancestor);
          };
          result = (ancestor->add(src->path, &source_tracer) = clone(src->get_source(), handler, mode)) ?: throw base::error("Clone returns null");
          ancestors.erase(ancestors.begin() + ancestors_mark, ancestors.end());
        }
        return result ?: throw base::error("Empty address_ref clone result");
      }

      return std::make_shared<address_ref>(
        *ancestor,
        string(src->path),
        clone(src->fallback, handler, mode));
    }
    throw base::error("Node of type '" + string(typeid(base_src).name()) + "' can't be cloned");
  };
  return clone(base_src, handler, mode);
}

bool is_fixed(base_p node) {
  if (!node)
    throw base::error("Empty node");
  if (auto doc = dynamic_cast<wrapper*>(node.get()); doc)
    node = doc->value;
  if (auto fixed = dynamic_cast<plain*>(node.get()); fixed)
    return true;
  return false;
}

string defaultable::use_fallback(const string& msg) const {
  return (fallback ?: throw base::error("Failure: " + msg + ". And no fallback was found"))->get();
}

base_p address_ref::get_source() const {
  // auto result = ancestor.get_child_ptr(path);
  // if (auto ref = dynamic_cast<address_ref*>(result.
  return ancestor.get_child_ptr(path);
}

string address_ref::get() const {
  auto result = get_source();
  return result ? result->get() : use_fallback("Referenced path doesn't exist: " + path);
}

bool address_ref::set(const string& val) {
  auto src = ancestor.get_child_ptr(path);
  auto target = dynamic_cast<settable*>(src ? src.get() : fallback ? fallback.get() : nullptr);
  if (target)
    return target->set(val);
  return false;
}


base_p meta::copy(std::shared_ptr<meta>&& dest, clone_handler handler, clone_mode mode) const {
  if (value)
    dest->value = clone(*value, handler, mode);
  if (fallback)
    dest->fallback = clone(*fallback, handler, mode);
  return dest;
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
    throw parse_error("parse: Empty reference conent");
  if (token_count == 1) {
    return rmaker(tokens[0], fallback);

  } else if (tokens[0] == "file"_ts) {
    tokens[token_count - 1].merge(tokens[1]);
    return make_meta(std::make_shared<file>());
    
  } else if (tokens[0] == "cmd"_ts) {
    tokens[token_count - 1].merge(tokens[1]);
    return make_meta(std::make_shared<cmd>());

  } else if (tokens[0] == "env"_ts) {
    if (token_count != 2)
      throw parse_error("parse: Expected 2 components");
    return make_meta(std::make_shared<env>());

  } else if (tokens[0] == "map"_ts) {
    if (token_count != 4)
      throw parse_error("parse.map: Expected 3 components");
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

  } else if (tokens[0] == "clone"_ts) {
    if (token_count != 2)
      throw parse_error("parse: Expected 2 components");
    auto ref = rmaker(tokens[1], fallback);
    return clone(*(ref->get_source().get() ?: throw parse_error("parse: Referenced key not found")));

  } else
    throw parse_error("Unsupported reference type: " + tokens[0]);
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
      newval->spots.emplace_back(int(ss.tellp()), value);
    }
    str.erase_front(end);
  } while (find_enclosed(str, raw, "${", "{", "}", start, end));
  ss << str;
  newval->base = ss.str();
  return move(newval);
}

NAMESPACE_END
