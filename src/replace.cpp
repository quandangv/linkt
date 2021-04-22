#include "node/parse.hpp"
#include "replace.hpp"
#include "tstring.hpp"

void replace_text(std::istream& is, std::ostream& os, node::wrapper_s& replacements) {
  string raw;
  // Initialize a parse context where all paths are based on the `replacements` tree root
  node::parse_context context;
  context.parent = context.root = replacements;

  // Read input stream line by line
  for (int linecount = 1; std::getline(is, raw); linecount++, raw.clear()) {
    // Tstring provides constant-time erase front operations for strings
    tstring ts(raw);
    size_t start, end;

    // Find escaped expressions in the line
    while(find_enclosed(ts, raw, "${", "{", "}", start, end)) {
      auto expression = ts.interval(start+2, end-1);

      // Recognize bash-style substring expressions: `${expr:position:length}` or `${expr:position}`
      int pos = -1, length = -1;
      if (auto comp = cut_back(expression, ':'); !comp.untouched()) {
        // Parse the last component, which is the `length` in the first case, or `position` in the second case
        pos = node::parse<int>(comp.begin(), comp.size());
        if (comp = cut_back(expression, ':'); !comp.untouched()) {
          // Parse the penultimate component, which must be the `position`. It also means that the last component is actually the `length`
          length = pos;
          pos = node::parse<int>(comp.begin(), comp.size());
        }
      }
      // Parse the expression and replace it with the value
      auto node = node::parse_escaped<string>(context, expression);
      if (node) {
        try {
          auto str = node->get();
          if (pos >= 0)
            str = str.substr(pos, length);
          ts.replace(raw, start, end - start, str);
          end = start + str.size();
        } catch (const std::exception& e) {
        }
      }
      // Move the processing range forward
      ts.erase_front(end);
    }
    os << raw << std::endl;
  }
}
