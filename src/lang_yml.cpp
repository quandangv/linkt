#include "languages.hpp"
#include "common.hpp"
#include "tstring.hpp"

#include <vector>
#include <cstring>
#include <iostream>

GLOBAL_NAMESPACE

constexpr const char comment_chars[] = ";#";

struct indentpair {
  int indent;
  addable* node;
  indentpair(int indent, addable* node) : node(node), indent(indent) {}
};

string_ref_p throw_ref_maker(const tstring&, string_ref_p&&) {
  throw std::invalid_argument("Can't make reference to children");
}

void parse_yml(std::istream& is, document& doc, errorlist& err) {
  vector<indentpair> nodes{indentpair(-1, &doc)};
  string raw;

  // Iterate through lines
  for (int linecount = 1; std::getline(is, raw); linecount++, raw.clear()) {
    LG_DBUG("parse: line: " << raw);
    tstring line(raw);

    int indent = ltrim(line);
    // skip empty and comment lines
    if (!line.empty() && !strchr(comment_chars, line.front())) {
      while (nodes.back().indent >= indent)
        nodes.pop_back();
      char op;
      if (tstring key; op = err.extract_key(line, linecount, ':', key)) {
        auto type = line.front();
        line.erase_front();
        trim_quotes(line);
        try {
          auto parent = nodes.back().node;
          if (!parent)
            err.report_error(linecount, "Parent can't take children");
          string_ref_p* place = parent->add(key, string_ref_p{}).get();
          document* wrapper = dynamic_cast<document*>(place->get());
          if (wrapper) {
            place = &wrapper->value;
          }
          local_ref_maker make_parent_ref = [&](tstring& ts, string_ref_p&& fallback) {
            return parent->make_local_ref(ts, move(fallback));
          };
          local_ref_maker make_local_ref = [&](tstring& ts, string_ref_p&& fallback) {
            if (!wrapper) {
              wrapper = new document();
              *place = string_ref_p(wrapper);
              place = &wrapper->value;
            }
            return wrapper->make_local_ref(ts, move(fallback));
          };
          if (type == ' ') {
            *place = parse_string(raw, line, make_local_ref);
          } else if (type == '$') {
            *place = parse_ref(raw, line, make_local_ref);
          } else if (type == '^') {
            *place = parse_ref(raw, line, make_parent_ref);
          } else {
            err.report_error(linecount, "Invalid character: " + type);
            continue;
          }
          nodes.emplace_back(indent, wrapper);
        } catch (const std::exception& e) {
          err.report_error(linecount, e.what());
        }
      }
    }
  }
}

void write_yml(std::ostream& os, const container& doc, int indent) {
  doc.iterate_children([&](const string& name, const string_ref& child) {
    // Indent the line
    std::fill_n(std::ostream_iterator<char>(os), indent, ' ');
    write_key(os, name + ": ", child.get());
    
    auto ctn = dynamic_cast<const container*>(&child);
    if(ctn)
      write_yml(os, *ctn, indent + 2);
  });
}

GLOBAL_NAMESPACE_END
