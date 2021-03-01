#pragma once

#include "base.hpp"
#include "tstring.hpp"

#include <map>
#include <vector>
#include <string>
#include <optional>

namespace linked_nodes::node {
  using std::string;

  using ancestor_processor = std::function<void(tstring& path, wrapper* ancestor)>;
  struct wrapper_error : std::logic_error { using logic_error::logic_error; };
  struct wrapper : public base {
    using map_type = std::map<string, base_p>;

    map_type map;

    wrapper(const base_p& value) : map{{"", value}} {}
    wrapper() {}

    static wrapper& wrap  (base_p& node);

    base_p& add  (tstring path, ancestor_processor* processor = nullptr);
    base_p& add  (tstring path, const base_p& value);
    base_p& add  (tstring path, string& raw, tstring value);
    base_p& add  (tstring path, string raw);

    base_p get_child_ptr  (tstring path) const;
    base_p* get_child_place  (tstring path);
    string get_child  (const tstring& path, string&& fallback) const;
    std::optional<string> get_child  (const tstring& path) const;

    void iterate_children  (std::function<void(const string&, const base_p&)> processor) const;
    void iterate_children  (std::function<void(const string&, const base&)> processor) const;

    string get  () const;
    bool set  (const tstring& path, const string& value);
    std::shared_ptr<address_ref> make_address_ref  (const tstring&, const base_p&);
    base_p clone  (clone_context& context) const;
  };
}
