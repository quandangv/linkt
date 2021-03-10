#pragma once

#include "base.hpp"
#include "tstring.hpp"

#include <map>
#include <vector>
#include <string>
#include <optional>
#include <memory>

namespace node {
  using std::string;
  using ancestor_processor = std::function<void(tstring& path, wrapper* ancestor)>;
  struct wrapper_error : std::logic_error { using logic_error::logic_error; };

  struct wrapper : base, std::enable_shared_from_this<wrapper> {
    using map_type = std::map<string, base_s>;

    map_type map{};

    explicit wrapper(const base_s& value) : map{{"", value}} {}
    wrapper() {}

    static wrapper_s wrap  (base_s& node);

    base_s& add  (tstring path, ancestor_processor* processor = nullptr);
    base_s& add  (tstring path, const base_s& value);
    base_s& add  (tstring path, string& raw, tstring value, parse_context& context);

    base_s get_child_ptr  (tstring path) const;
    base_s* get_child_place  (tstring path);
    string get_child  (const tstring& path, string&& fallback) const;
    std::optional<string> get_child  (const tstring& path) const;

    void iterate_children  (std::function<void(const string&, const base_s&)> processor) const;
    void iterate_children  (std::function<void(const string&, const base&)> processor) const;

    bool set  (const tstring& path, const string& value);
    wrapper& optimize  (clone_context&);
    void merge  (const wrapper& source, clone_context&);
    string get  () const;
    base_s clone  (clone_context&) const;
  };
}
