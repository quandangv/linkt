#pragma once

#include "gtest/gtest.h"
#include "gtest/gtest-test-part.h"
#include "wrapper.hpp"

#include <iostream>

using namespace ::testing;
using namespace lini;

using std::cerr, std::cout, std::vector, std::endl, std::string;

// Failed ASSERT/EXPECT would cause the value returned to increase
// This can be used to track new failures
int get_current_test_part_count() {
  auto error = std::runtime_error("Null pointer! Are we in a test?");
  return (((UnitTest::GetInstance() ?: throw error)->current_test_info() ?: throw error)->result() ?: throw error)->total_part_count();
}

void check_key(const node::wrapper& w, string path, string expected, bool exception) {
  auto last_count = get_current_test_part_count();
  try {
    auto result = w.get_child(path);
    EXPECT_TRUE(result) << "Can't retrieve key";
    if (last_count == get_current_test_part_count())
      EXPECT_EQ(*result, expected) << "Unexpected value";
  } catch (const std::exception& e) {
    EXPECT_TRUE(exception) << "Unexpected exception thrown: " << e.what();
  }
  if (last_count != get_current_test_part_count())
    cerr << "Key: " << path << endl;
}
