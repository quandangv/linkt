#include "strsub.hpp"
#include "common.hpp"
#include "wrapper.hpp"

namespace node {

strsub::operator string() const {
  size_t base_i = 0;
  bool copied = false;
  for (auto& spot : spots) {
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
  if (context.optimize && is_fixed())
    return std::make_shared<plain<string>>(get());
  auto result = std::make_unique<strsub>();
  result->base = base;
  result->spots.reserve(spots.size());
  for(auto& spot : spots)
    result->spots.emplace_back(spot.start, spot.length, checked_clone<string>(spot.replacement, context, "strsub::clone"));
  return result;
}

bool strsub::is_fixed() const {
  for(auto& spot : spots)
    if (!spot.replacement->is_fixed())
      return false;
  return true;
}

}
