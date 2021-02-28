#pragma once

#include "base.hpp"
#include "error.hpp"
#include "tstring.hpp"

#include <map>
#include <vector>
#include <string>
#include <optional>

namespace lini::node {
  using std::string;

  using ancestor_processor = std::function<void(tstring& path, wrapper* ancestor)>;
  struct wrapper : public base {
    struct error : error_base { using error_base::error_base; };
    using map_type = std::map<string, base_p>;

    map_type map;
    base_p value;

    wrapper(const base_p& value) : value(value) {}
    wrapper() {}

    static wrapper& wrap  (base_p& node);

    base_p& add  (tstring path, ancestor_processor* processor = nullptr);
    base_p& add  (tstring path, const base_p& value);
    base_p& add  (tstring path, string& raw, tstring value);
    base_p& add  (tstring path, string raw);

    base_p get_child_ptr  (tstring path) const;
    base_p& get_child_ref  (tstring path);
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
