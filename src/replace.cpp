#include "replace.hpp"
#include "tstring.hpp"
#include "node/parse.hpp"

#include <cstring>
#include <vector>

void replace_text(std::istream& is, std::ostream& os, node::wrapper_s& replacements) {
  string raw;
  node::parse_context context;
  context.parent = context.root = replacements;
  context.parent_based_ref = true;
  for (int linecount = 1; std::getline(is, raw); linecount++, raw.clear()) {
    tstring ts(raw);
    size_t start, end;
    while(find_enclosed(ts, raw, "${", "{", "}", start, end)) {
      auto expression = ts.interval(start+2, end-1);
      int offset = -1, length = -1;
      if (auto comp = cut_back(expression, ':'); !comp.untouched()) {
        offset = node::parse<int>(comp.begin(), comp.size());
        if (comp = cut_back(expression, ':'); !comp.untouched()) {
          length = offset;
          offset = node::parse<int>(comp.begin(), comp.size());
        }
      }
      auto node = node::parse_escaped<string>(context, expression);
      if (node) {
        try {
          auto str = node->get();
          if (offset >= 0)
            str = str.substr(offset, length);
          ts.replace(raw, start, end - start, str);
          end = start + str.size();
        } catch (const std::exception& e) {
        }
      }
      ts.erase_front(end);
    }
    os << raw << std::endl;
  }
}
