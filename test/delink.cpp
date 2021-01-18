#include "parse_delink.hpp"
#include "test.h"

#include <fstream>

using namespace lini;

struct delink_test_single {
  string section, key, value, delinked;
  bool fail;
};
using delink_test = vector<delink_test_single>;
vector<delink_test> delink_tests = {
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
    {"test", "ref-self-a", "${key-a?failed}", "a"},
    {"test2", "ref-fallback-a", "${ test.key-a ? fail }", "a"},
    {"test2", "ref-nexist", "${test.key-nexist? \" f a i l ' }", "\" f a i l '"},
    {"test2", "ref-fail", "${test.key-fail}", "${test.key-fail}", true},
    {"test2", "ref-fake", "{test.key-a}", "{test.key-a}"},
    {"test2", "interpolation", "This is ${test.key-a} test", "This is a test"},
    {"test2", "interpolation-trick", "$ ${test.key-a}", "$ a"},
    {"test2", "interpolation-trick-2", "} ${test.key-a}", "} a"},
    {"test2", "escape", "\\${test.key-a}", "${test.key-a}"},
    {"test2", "not-escape", "\\$${test.key-a}", "\\$a"},
  },
  {
    {"test", "ref-cyclic-1", "${ref-cyclic-2}", "${ref-cyclic-1}"},
    {"test", "ref-cyclic-2", "${ref-cyclic-1}", "${ref-cyclic-1}", true},
  },
  {
    {"test", "ref-cyclic-1", "${ref-cyclic-2}", "${ref-cyclic-1}"},
    {"test", "ref-cyclic-2", "${ref-cyclic-3}", "${ref-cyclic-1}"},
    {"test", "ref-cyclic-3", "${ref-cyclic-1}", "${ref-cyclic-1}", true},
    {"test", "ref-not-cyclic-1", "${ref-not-cyclic-2}", ""},
    {"test", "ref-not-cyclic-2", "", ""}
  },
  {{"", "file", "${file: delink_file.txt }", "content"}},
  {{"", "file", "${file:delink_file.txt?fail}", "content"}},
  {
    {"", "ext", "txt", "txt"},
    {"", "file", "${file:delink_file.${ext} ? fail}", "content"},
    {"", "file-fail", "${file:nexist.${ext} ? Can't find ${ext} file}", "Can't find txt file"},
  },
  {{"", "file", "${file:nexist.txt ? ${file:delink_file.txt}}", "content"}},
  {{"", "file", "${file:nexist.txt ? \" f a i l ' }", "\" f a i l '"}},
  {{"", "file", "${file:nexist.txt}", "${file:nexist.txt}", true}},
  {{"", "dumb", "${dumb:nexist.txt}", "${dumb:nexist.txt}", true}},
  {{"", "dumb", "", ""}},
  {{"", "cmd", "${cmd:echo hello world}", "hello world"}},
  {{"", "cmd", "${cmd:nexist}", "", true}},
  {
    {"", "msg", "foo bar", "foo bar"},
    {"", "cmd", "${cmd:echo ${msg}}", "foo bar"},
  },
  {{"", "env", "${env: test_env? fail}", "test_env"}},
  {{"", "env", "${env:nexist? \" f a i l \" }", " f a i l "}},
  {
    {"", "color", "${color: #123456 }", "#123456"},
    {"", "color-hsv", "${color: hsv(180, 1, 0.75)}", "#00BFBF"},
    {"", "color-ref", "${color: ${color}}", "#123456"},
    {"", "color-mod", "${color: cielch: lum * 1.5, hue + 60; ${color}}", "#633E5C"},
  },
};
class DelinkTest : public ::testing::Test, public ::testing::WithParamInterface<delink_test> {};
INSTANTIATE_TEST_SUITE_P(Delink, DelinkTest, ::testing::ValuesIn(delink_tests));

TEST_P(DelinkTest, general) {
  setenv("test_env", "test_env", true);
  unsetenv("nexist");
  document doc;
  str_errlist err;
  auto testset = GetParam();
  for(auto test : testset)
    EXPECT_TRUE(doc.add_onetime(test.section, test.key, move(test.value)))
        << "Duplicate key: " << test.section << "." << test.key;
  delink(doc, err);
  for(auto test : testset) {
    auto fullkey = test.section + "." + test.key;
    auto pos = find_if(err.begin(), err.end(), [&](auto it) { return it.first == fullkey; });
    try {
      EXPECT_TRUE(doc.get(test.section, test.key))
          << "Key: " << fullkey;
      EXPECT_EQ(*doc.get(test.section, test.key), test.delinked)
          << "Key: " << fullkey;
      EXPECT_EQ(pos != err.end(), test.fail)
          << "Key: " << fullkey << endl
          << (pos == err.end() ? "Expected error" : "Unexpected error: " + pos->second);
    } catch (const exception& e) {
      EXPECT_TRUE(test.fail)
          << "Key: " << fullkey << endl
          << "Exception: " << e.what() << endl
          << "Exception thrown, but the test is not expected to fail";
    }
  }
}

struct document_key {
  string section, key, value;
};
vector<document_key> set_document = {
  {"", "key-a", "a"},
  {"", "ref-ref-a", "${ref-a?failed}"},
  {"", "ref-self-a", "${key-a?failed}"},
  {"", "ref-a", "${key-a}"},
  {"", "ref-default-a", "${key-nexist?${key-a}}"},
  {"", "ref-fallback-a", "${ key-a ? fail }"},
  {"", "ref-nexist", "${key-nexist? \" f a i l ' }"},
  {"", "file-delink", "${file: delink_file.txt }"},
  {"", "env", "${env: test_env? fail}"},
  {"", "env-nexist", "${env:nexist? \" f a i l \" }"},
};

TEST(Delink, set_test) {
  document doc;
  str_errlist err;
  auto set_key = [&](const string& key, const string& newval) {
    doc.get_ref("", key).set(newval);
    EXPECT_EQ(newval, doc.get("", key));
  };
  for(auto test : set_document)
    doc.add_onetime(test.section, test.key, move(test.value));
  delink(doc, err);
  set_key("ref-a", "foo");
  set_key("ref-ref-a", "bar");
  EXPECT_EQ("bar", *doc.get("", "key-a"));
  set_key("ref-default-a", "foobar");
  EXPECT_EQ("foobar", *doc.get("", "key-a"));
  set_key("ref-nexist", "barfoo");
  set_key("env-nexist", "barbar");
  
  set_key("file-delink", "foo");
  ifstream ifs("delink_file.txt");
  string content;
  getline(ifs, content);
  EXPECT_EQ("foo", content);
  
  set_key("env", "foo");

  // Revert values back to its original
  set_key("file-delink", "content");
}
