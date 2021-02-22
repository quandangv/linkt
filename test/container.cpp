#include "wrapper.hpp"
#include "test.h"

#include <fstream>

using namespace lini;

struct parse_test_single {
  string path, value, parsed;
  bool fail, exception;
};
using parse_test = vector<parse_test_single>;
vector<parse_test> parse_tests = {
  {
    {".key", "foo", "foo"},
    {"a.ref", "${.key}", "foo"},
    {"a.ref-space", "${ .key }", "foo"},
  },
  {
    {"test2.ref-file-default-before", "${file nexist.txt ? ${test.ref-ref-a}}", "a"},
    {"test2.ref-before", "${test2.ref-a}", "a"},
    {"test.key-a", "a", "a"},
    {"test2.ref-a", "${test.key-a}", "a"},
    {"test2.ref-default-a", "${test.key-nexist?${test.key-a}}", "a"},
    {"test2.ref-file-default", "${file nexist.txt ? ${test.key-a}}", "a"},
    {"test.ref-ref-a", "${test2.ref-a?failed}", "a"},
    {"test2.ref-fallback-a", "${ test.key-a ? fail }", "a"},
    {"test2.ref-nexist", "${test.key-nexist? \" f a i l ' }", "\" f a i l '"},
    {"test2.ref-fail", "${test.key-fail}", "${test.key-fail}", false, true},
    {"test2.ref-fake", "{test.key-a}", "{test.key-a}"},
    {"test2.interpolation", "This is ${test.key-a} test", "This is a test"},
    {"test2.interpolation-trick", "$ ${test.key-a}", "$ a"},
    {"test2.interpolation-trick-2", "} ${test.key-a}", "} a"},
    {"test2.escape", "\\${test.key-a}", "${test.key-a}"},
    {"test2.not-escape", "\\$${test.key-a}", "\\$a"},
  },
//  {
//    {"", "ref-cyclic-1", "${.ref-cyclic-2}", "${ref-cyclic-1}"},
//    {"", "ref-cyclic-2", "${.ref-cyclic-1}", "${ref-cyclic-1}", true},
//  },
//  {
//    {"", "ref-cyclic-1", "${.ref-cyclic-2}", "${ref-cyclic-1}"},
//    {"", "ref-cyclic-2", "${.ref-cyclic-3}", "${ref-cyclic-1}"},
//    {"", "ref-cyclic-3", "${.ref-cyclic-1}", "${ref-cyclic-1}", true},
//    {"", "ref-not-cyclic-1", "${.ref-not-cyclic-2}", ""},
//    {"", "ref-not-cyclic-2", "", ""}
//  },
  {{".file", "${file key_file.txt }", "content"}},
  {{".file", "${file key_file.txt?fail}", "content"}},
  {
    {".ext", "txt", "txt"},
    {".file", "${file key_file.${.ext} ? fail}", "content"},
    {".file-fail", "${file nexist.${.ext} ? Can't find ${.ext} file}", "Can't find txt file"},
  },
  {{".file", "${file nexist.txt ? ${file key_file.txt}}", "content"}},
  {{".file", "${file nexist.txt ? \" f a i l ' }", "\" f a i l '"}},
  {{".file", "${file nexist.txt}", "${file nexist.txt}", false, true}},
  {{".interpolate", "%{${color hsv(0, 1, 0.5)}}", "%{#800000}"}},
  {{".dumb", "${dumb nexist.txt}", "${dumb nexist.txt}", true}},
  {{".dumb", "", ""}},
  {{".cmd", "${cmd echo hello world}", "hello world"}},
  {{".cmd", "${cmd nexist}", "", false, true}},
  {
    {".msg", "foo bar", "foo bar"},
    {".cmd", "${cmd echo ${.msg}}", "foo bar"},
  },
  {{".env", "${env test_env? fail}", "test_env"}},
  {{".env", "${env nexist? \" f a i l \" }", " f a i l "}},
  {{".map", "${map 5:10 0:2 7.5}", "1.000000"}},
  {{".map", "${map 5:10 2 7.5}", "1.000000"}},
  {
    {".color", "${color #123456 }", "#123456"},
    {".color-fallback", "${color nexist(1) ? #ffffff }", "#ffffff"},
    {".color-hsv", "${color hsv(180, 1, 0.75)}", "#00BFBF"},
    {".color-ref", "${color ${.color}}", "#123456"},
    {".color-mod", "${color cielch 'lum * 1.5, hue + 60' ${.color}}", "#633E5C"},
  },
};
class container_test : public ::testing::Test, public ::testing::WithParamInterface<parse_test> {};
INSTANTIATE_TEST_SUITE_P(parse, container_test, ::testing::ValuesIn(parse_tests));

void test_wrapper(const node::wrapper& doc, parse_test testset) {
  // Optimize wrapper and check content of keys
  for(auto test : testset) {
    // Skip key if it is expected to fail in the previous step
    if (test.fail) continue;
    try {
      // Check the content of the key
      auto result = doc.get_child(test.path);
      ASSERT_TRUE(result)
          << "Key: " << test.path << endl << "Can't retrieve key";
      ASSERT_EQ(*result, test.parsed)
          << "Key: " << test.path << endl << "Value of parsed key doesn't match expectation";
    } catch (const std::exception& e) {
      EXPECT_TRUE(test.exception)
          << "Key: " << test.path << endl
          << "Unexpected exception thrown" << endl
          << "Exception: " << e.what();
    }
  }
}

TEST_P(container_test, general) {
  // Prepare the environment variables
  setenv("test_env", "test_env", true);
  unsetenv("nexist");

  node::wrapper doc;
  auto testset = GetParam();
  // Add keys and check errors
  for(auto test : testset) {
    if (test.fail)
      EXPECT_THROW(doc.add(test.path, move(test.value)), node::container::error)
          << "Key: " << test.path << endl << "Expected error";
    else {
      EXPECT_NO_THROW(doc.add(test.path, move(test.value)))
          << "Key: " << test.path << endl << "Unexpected error";
    }
  }
  test_wrapper(doc, testset);
  auto cloned_doc = clone(doc);
  test_wrapper(dynamic_cast<node::wrapper&>(*cloned_doc), testset);
}
