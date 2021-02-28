#include "base.hpp"
#include "node.hpp"
#include "common.hpp"
#include "wrapper.hpp"
#include "token_iterator.hpp"

#include <map>
#include <sstream>

NAMESPACE(lini::node)

base_p base::clone() const {
  clone_context context;
  context.no_dependency = true;
  auto result = clone(context);
  return context.errors.empty() ? result :
      throw error("Errors while cloning: \n" + context.errors.merge_errors());
}

string errorlist::merge_errors() const {
  std::stringstream ss;
  for(auto& err : *this)
    ss << err.first << ": " << err.second << '\n';
  return ss.str();
}

void errorlist::report_error(int linecount, const string& msg) {
  emplace_back("line " +std::to_string(linecount), msg);
}

void errorlist::report_error(int linecount, const string& key, const string& msg) {
  emplace_back("line " +std::to_string(linecount) + ", " + key, msg);
}

void errorlist::report_error(const string& key, const string& msg) {
  emplace_back(key, msg);
}

bool errorlist::extract_key(tstring& line, int linecount, char separator, tstring& key) {
  key = cut_front(line, separator);
  if (key.untouched())
    return report_error(linecount, "Line ignored: " + line), false;
  return true;
}

void clone_context::report_error(const string& msg) {
  errors.report_error(current_path, msg);
}

bool is_fixed(base_p node) {
  if (!node)
    throw base::error("Empty node");
  if (auto doc = dynamic_cast<wrapper*>(node.get()); doc)
    node = doc->get_child_ptr(""_ts);
  if (auto fixed = dynamic_cast<plain*>(node.get()); fixed)
    return true;
  return false;
}

string defaultable::use_fallback(const string& msg) const {
  return (fallback ?: throw base::error("Failure: " + msg + ". And no fallback was found"))->get();
}

base_p address_ref::get_source() const {
  auto result = ancestor.get_child_ptr(path);
  return result ?: fallback ?: throw base::error("Can't find referenced key: " + path);
}

string address_ref::get() const {
  return get_source()->get();
}

bool address_ref::set(const string& val) {
  auto src = ancestor.get_child_ptr(path);
  auto target = dynamic_cast<settable*>(src ? src.get() : fallback ? fallback.get() : nullptr);
  if (target)
    return target->set(val);
  return false;
}

base_p address_ref::clone(clone_context& context) const {
  auto ancestor_it = find_if(context.ancestors.rbegin(), context.ancestors.rend(), [&](auto& pair) { return pair.first == &ancestor; });
  auto cloned_ancestor = ancestor_it != context.ancestors.rend() ? ancestor_it->second :
      !context.no_dependency ? &ancestor : throw base::error("External dependency");

  if (context.optimize) {
    auto result = cloned_ancestor->get_child_ptr(path);
    if (auto wrpr = dynamic_cast<wrapper*>(result.get()); wrpr)
      result = wrpr->get_child_ptr(""_ts);
    if (!result) {
      // This will recursively dereference chain references.
      auto src_ancestor = &ancestor;
      ancestor_processor source_tracer = [&](tstring& path, wrapper* inner_ancestor)->void {
        src_ancestor = dynamic_cast<wrapper*>(src_ancestor->get_child_ptr(path).get())
            ?: throw base::error("Invalid reference");
        context.ancestors.emplace_back(src_ancestor, inner_ancestor);
      };
      auto src = ancestor.get_child_ptr(path);
      if (!src) {
        LG_DBUG("Fallback: " << fallback.get());
        result = (fallback ?: throw base::error("Clone: Can't find referenced key: " + path))->clone(context);
      } else {
        auto ancestors_mark = context.ancestors.size();
        auto& place = cloned_ancestor->add(path, &source_tracer);
        result = place = src->clone(context);
        context.ancestors.erase(context.ancestors.begin() + ancestors_mark, context.ancestors.end());
      }
    }
    return result ?: throw base::error("Empty address_ref clone result");
  }

  return std::make_shared<address_ref>(
    *cloned_ancestor,
    string(path),
    fallback ? fallback->clone(context) : base_p());
}

base_p meta::copy(std::shared_ptr<meta>&& dest, clone_context& context) const {
  if (value)
    dest->value = value->clone(context);
  if (fallback)
    dest->fallback = fallback->clone(context);
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
    return rmaker(tokens[1], fallback)->get_source()->clone();
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
