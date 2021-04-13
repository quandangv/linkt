#include "strsub.hpp"
#include "common.hpp"
#include "wrapper.hpp"
#include "reference.hpp"

namespace node {

strsub::operator string() const {
  return substitute(true);
}

string strsub::substitute(bool full) const {
  size_t base_i = 0;
  bool copied = false;
  for (auto& spot : spots) {
    if (!full && !spot.replacement->is_fixed()) {
      auto old_base_i = base_i;
      base_i = spot.start + spot.length;
      if (copied) {
        tmp.append(base, old_base_i, base_i - old_base_i);
        spot.start = tmp.size() - spot.length;
      }
      continue;
    }
    auto replacement = spot.replacement->get();
    if (copied) {
      tmp.append(base, base_i, spot.start - base_i);
    } else if (replacement.size() != spot.length) {
      tmp.assign(base, 0, spot.start);
      copied = true;
    }
    base_i = spot.start + spot.length;
    if (copied) {
      spot.start = tmp.size();
      spot.length = replacement.size();
      tmp.append(replacement);
    } else {
      base.replace(spot.start, spot.length, replacement);
    }
  }
  if (copied) {
    tmp.append(base, base_i, string::npos);
    base.swap(tmp);
  }
  return base;
}

base_s strsub::clone(clone_context& context) const {
  auto result = std::make_unique<strsub>();

  if (context.optimize) {
    substitute(false);
    for (auto& spot : spots) {
      if (!spot.replacement->is_fixed()) {
        auto replacement = checked_clone<string>(spot.replacement, context, "strsub::clone");
        while (auto repref = std::dynamic_pointer_cast<ref_base<string>>(replacement))
          replacement = repref->get_source();
        if (auto repsub = std::dynamic_pointer_cast<strsub>(replacement)) {
          for (auto& repspot : repsub->spots)
            result->spots.emplace_back(repspot.start + spot.start, repspot.length, repspot.replacement);
        } else {
          result->spots.emplace_back(spot.start, spot.length, replacement);
        }
      }
    }
    if (result->spots.empty())
      return std::make_shared<plain<string>>(string(base));
  } else {
    for(auto& spot : spots)
      result->spots.emplace_back(spot.start, spot.length, checked_clone<string>(spot.replacement, context, "strsub::clone"));
  }

  result->base = base;
  result->spots.reserve(spots.size());
  return result;
}

bool strsub::is_fixed() const {
  for(auto& spot : spots)
    if (!spot.replacement->is_fixed())
      return false;
  return true;
}

}
