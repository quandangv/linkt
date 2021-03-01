#include "languages.hpp"
#include "common.hpp"

NAMESPACE(linked_nodes)

constexpr const char excluded_chars[] = "\t \"'=;#[](){}:$\\%";

void write_key  (std::ostream&, const std::string& prefix, std::string&& value);

NAMESPACE_END
