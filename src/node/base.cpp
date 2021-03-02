#include "base.hpp"
#include "node.hpp"
#include "common.hpp"
#include "wrapper.hpp"
#include "token_iterator.hpp"

#include <map>
#include <sstream>

NAMESPACE(node)

// Create a clone_context and clone using default behaviour
base_p base::clone() const {
  clone_context context;
  auto result = clone(context);
  // Return only if the clone produce no error
  if (context.errors.empty())
    return result;
  // Otherwise throw an exception containing all the errors
  std::stringstream ss;
  for(auto& err : context.errors)
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

// Checks if the value of a node come directly from a plain node, meaning it never changes
bool is_fixed(base_p node) {
  if (auto doc = dynamic_cast<wrapper*>(node.get()))
    node = doc->get_child_ptr(""_ts);
  if (auto fixed = dynamic_cast<plain*>(node.get()))
    return true;
  return false;
}

// Returns the value of the fallback if available. Otherwise throws an error
string defaultable::use_fallback(const string& msg) const {
  return (fallback ?: THROW_ERROR(node, "Failure: " + msg + ". No fallback was found"))->get();
}

// Returns the node that this reference points to
base_p address_ref::get_source() const {
  auto result = ancestor.get_child_place(path);
  return result && *result ? *result : fallback ?:
      THROW_ERROR(node, "Can't find referenced key: " + path);
}

bool address_ref::set(const string& val) {
  // Sets the value of the node pointed to
  auto src = ancestor.get_child_ptr(path);
  // If the path doesn't point to a node, set the value of the fallback
  auto target = dynamic_cast<settable*>(src ? src.get() : fallback ? fallback.get() : nullptr);
  return target ? target->set(val) : false;
}

base_p address_ref::clone(clone_context& context) const {
  // Find the corresponding ancestor in the clone result tree
  auto ancestor_it = find_if(context.ancestors.rbegin(), context.ancestors.rend(), [&](auto& pair) { return pair.first == &ancestor; });
  wrapper* cloned_ancestor;
  if (ancestor_it != context.ancestors.rend()) {
    cloned_ancestor = ancestor_it->second;
  } else if (context.no_dependency) {
    context.report_error("External dependency");
    return base_p();
  } else {
    cloned_ancestor = &ancestor;
  }

  if (context.optimize) {
    // Return a pointer to the referenced node
    // If the referenced node already exists in the clone result, we don't have to clone it
    auto result = cloned_ancestor->get_child_ptr(path);
    if (!result) {
      auto place = ancestor.get_child_place(path);
      if (!place) {
        // If the referenced node can't be found, return a clone of the fallback
        if (fallback)
          result = fallback->clone(context);
        else
          context.report_error("Can't find referenced key: " + path);
      } else {
        // Clone the referenced node, add it to the clone result, and return the pointer
        auto src_ancestor = &ancestor;
        ancestor_processor record_ancestor = [&](tstring& path, wrapper* inner_ancestor)->void {
          src_ancestor = dynamic_cast<wrapper*>(src_ancestor->map.at(path).get());
          context.ancestors.emplace_back(src_ancestor, inner_ancestor);
        };
        // Detach the object from its place while cloning to avoid cyclical references
        auto src = move(*place);
        // Track the added ancestors to be removed after its done
        auto ancestors_mark = context.ancestors.size();
        // Build the clone result tree up to the referenced node in case it's also a reference
        // So that the reference can find its corresponding ancestor
        auto& cloned_place = cloned_ancestor->add(path, &record_ancestor);
        result = cloned_place = src->clone(context);
        context.ancestors.erase(context.ancestors.begin() + ancestors_mark, context.ancestors.end());
        *place = src;
      }
    }
    return result;
  }

  // Return a reference to the corresponding path in the clone result
  return std::make_shared<address_ref>(
    *cloned_ancestor,
    string(path),
    fallback ? fallback->clone(context) : base_p());
}

// Clone the value and fallback of this object to another object.
base_p meta::copy(std::shared_ptr<meta>&& dest, clone_context& context) const {
  if (value)
    dest->value = value->clone(context);
  if (fallback)
    dest->fallback = fallback->clone(context);
  return dest;
}

// Parse an unescaped node string
base_p parse_raw(string& raw, tstring& str, ref_maker rmaker) {
  size_t start, end;
  if (!find_enclosed(str, raw, "${", "{", "}", start, end)) {
    // There is no node inside the string, it's a plain string
    return std::make_shared<plain>(str);
  } else if (start == 0 && end == str.size()) {
    // There is a single node inside, interpolation is unecessary
    str.erase_front(2);
    str.erase_back();
    return parse_escaped(raw, str, rmaker);
  }
  // String interpolation
  std::stringstream ss;
  auto newval = std::make_shared<string_interpolate>();
  do {
    // Write the part we have moved past to the base string
    ss << substr(str, 0, start);

    // Make node from the token, skipping the brackets
    auto token = str.interval(start + 2, end - 1);
    auto value = parse_escaped(raw, token, rmaker);
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

// Parse an escaped node string
base_p parse_escaped(string& raw, tstring& str, ref_maker rmaker) {
  // Extract the fallback before anything
  base_p fallback;
  if (auto fb_str = cut_back(str, '?'); !fb_str.untouched())
    fallback = parse_raw(raw, trim_quotes(fb_str), rmaker);

  std::array<tstring, 5> tokens;
  auto token_count = fill_tokens(str, tokens);

  // Finalize nodes that derive from node::meta
  auto make_meta = [&]<typename T>(const std::shared_ptr<T>& ptr) {
    ptr->fallback = move(fallback);
    ptr->value = parse_raw(raw, trim_quotes(tokens[token_count - 1]), rmaker);
    return ptr;
  };
  if (token_count == 0)
    THROW_ERROR(parse, "Empty reference string");
  if (token_count == 1) {
    // Return a reference node
    return rmaker(tokens[0], fallback);

  } else if (tokens[0] == "file"_ts) {
    tokens[token_count - 1].merge(tokens[1]);
    return make_meta(std::make_shared<file>());
    
  } else if (tokens[0] == "cmd"_ts) {
    tokens[token_count - 1].merge(tokens[1]);
    return make_meta(std::make_shared<cmd>());

  } else if (tokens[0] == "env"_ts) {
    if (token_count != 2)
      THROW_ERROR(parse, "env: Expected 1 components");
    return make_meta(std::make_shared<env>());

  } else if (tokens[0] == "map"_ts) {
    if (token_count != 4)
      THROW_ERROR(parse, "map: Expected 3 components");
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
      THROW_ERROR(parse, "clone: Expected 1 components");
    return rmaker(tokens[1], fallback)->get_source()->clone();
  } else
    THROW_ERROR(parse, "Unsupported reference type: " + tokens[0]);
}

NAMESPACE_END
