#include "parse.hpp"
#include "test.h"

#include <fstream>

using namespace lini;

struct parse_test_single {
  string section, key, value, parsed;
  bool fail, exception;
};
using parse_test = vector<parse_test_single>;
vector<parse_test> parse_tests = {
  {
    {"", "key-rogue", "rogue", "rogue"},
    {"test", "ref-rogue", "${.key-rogue}", "rogue"},
  },
  {
    {"test2", "ref-file-default-before", "${file: nexist.txt ? ${test.ref-ref-a}}", "a"},
    {"test2", "ref-before", "${test2.ref-a}", "a"},
    {"test", "key-a", "a", "a"},
    {"test2", "ref-a", "${test.key-a}", "a"},
    {"test2", "ref-default-a", "${test.key-nexist?${test.key-a}}", "a"},
    {"test2", "ref-file-default", "${file: nexist.txt ? ${test.key-a}}", "a"},
    {"test", "ref-ref-a", "${test2.ref-a?failed}", "a"},
    {"test2", "ref-fallback-a", "${ test.key-a ? fail }", "a"},
    {"test2", "ref-nexist", "${test.key-nexist? \" f a i l ' }", "\" f a i l '"},
    {"test2", "ref-fail", "${test.key-fail}", "${test.key-fail}", false, true},
    {"test2", "ref-fake", "{test.key-a}", "{test.key-a}"},
    {"test2", "interpolation", "This is ${test.key-a} test", "This is a test"},
    {"test2", "interpolation-trick", "$ ${test.key-a}", "$ a"},
    {"test2", "interpolation-trick-2", "} ${test.key-a}", "} a"},
    {"test2", "escape", "\\${test.key-a}", "${test.key-a}"},
    {"test2", "not-escape", "\\$${test.key-a}", "\\$a"},
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
  {{"", "file", "${file: key_file.txt }", "content"}},
  {{"", "file", "${file:key_file.txt?fail}", "content"}},
  {
    {"", "ext", "txt", "txt"},
    {"", "file", "${file:key_file.${.ext} ? fail}", "content"},
    {"", "file-fail", "${file:nexist.${.ext} ? Can't find ${.ext} file}", "Can't find txt file"},
  },
  {{"", "file", "${file:nexist.txt ? ${file:key_file.txt}}", "content"}},
  {{"", "file", "${file:nexist.txt ? \" f a i l ' }", "\" f a i l '"}},
  {{"", "file", "${file:nexist.txt}", "${file:nexist.txt}", false, true}},
  {{"", "dumb", "${dumb:nexist.txt}", "${dumb:nexist.txt}", true}},
  {{"", "dumb", "", ""}},
  {{"", "cmd", "${cmd:echo hello world}", "hello world"}},
  {{"", "cmd", "${cmd:nexist}", "", false, true}},
  {
    {"", "msg", "foo bar", "foo bar"},
    {"", "cmd", "${cmd:echo ${.msg}}", "foo bar"},
  },
  {{"", "env", "${env: test_env? fail}", "test_env"}},
  {{"", "env", "${env:nexist? \" f a i l \" }", " f a i l "}},
  {
    {"", "color", "${color: #123456 }", "#123456"},
    {"", "color-fallback", "${color: nexist(1) ? #ffffff }", "#ffffff"},
    {"", "color-hsv", "${color: hsv(180, 1, 0.75)}", "#00BFBF"},
    {"", "color-ref", "${color: ${.color}}", "#123456"},
    {"", "color-mod", "${color: cielch: lum * 1.5, hue + 60; ${.color}}", "#633E5C"},
  },
};
class ParseTest : public ::testing::Test, public ::testing::WithParamInterface<parse_test> {};
INSTANTIATE_TEST_SUITE_P(parse, ParseTest, ::testing::ValuesIn(parse_tests));

TEST_P(ParseTest, general) {
  // Prepare the environment variables
  setenv("test_env", "test_env", true);
  unsetenv("nexist");

  document doc;
  auto testset = GetParam();
  // Add keys and check errors
  for(auto test : testset) {
    auto fullkey = test.section + "." + test.key;
    if (test.fail)
      EXPECT_THROW(doc.add(test.section, test.key, move(test.value)), document::error)
          << "Key: " << fullkey << endl << "Expected error";
    else {
      EXPECT_NO_THROW(doc.add(test.section, test.key, move(test.value)))
          << "Key: " << fullkey << endl << "Unexpected error";
    }
  }
  // Optimize document and check content of keys
  for(auto test : testset) {
    // Skip key if it is expected to fail in the previous step
    if (test.fail) continue;
    auto fullkey = test.section + "." + test.key;
    try {
      // Check for existence
      auto index = doc.find(test.section, test.key);
      ASSERT_TRUE(index) << "Key: " << fullkey << endl << "parsed key doesn't exist";
      auto& ref = *doc.values[*index];
      ASSERT_TRUE(ref) << "Key: " << fullkey << endl << "parsed key is null";

      // Optimize the reference
      if (auto op = ref->get_optimized(); op) ref = move(op);

      // Check the content of the key
      ASSERT_EQ(ref->get(), test.parsed)
          << "Key: " << fullkey << endl << "Value of parsed key doesn't match expectation";
    } catch (const exception& e) {
      EXPECT_TRUE(test.exception)
          << "Key: " << fullkey << endl
          << "Exception: " << e.what() << endl
          << "Exception thrown, but the test is not expected to fail";
    }
  }
}
