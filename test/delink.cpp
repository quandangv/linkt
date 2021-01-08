#include "parse_delink.hpp"
#include "test.h"

using namespace std;
using namespace lini;

struct delink_test_single {
  string section, key, value, delinked;
  bool fail;
};
using delink_test = vector<delink_test_single>;
vector<delink_test> delink_tests = {
  {
    {"", "key-rogue", "rogue", "rogue", false},
    {"test", "ref-rogue", "${.key-rogue}", "rogue", false},
  },
  {
    {"test2", "ref-file-default-before", "${file: nexist.txt ? ${test.ref-ref-a}}", "a", false},
    {"test2", "ref-before", "${test2.ref-a}", "a", false},
    {"test", "key-a", "a", "a", false},
    {"test2", "ref-a", "${test.key-a}", "a", false},
    {"test2", "ref-default-a", "${test.key-nexist?${test.key-a}}", "a", false},
    {"test2", "ref-file-default", "${file: nexist.txt ? ${test.key-a}}", "a", false},
    {"test", "ref-ref-a", "${test2.ref-a?failed}", "a", false},
    {"test", "ref-self-a", "${key-a?failed}", "a", false},
    {"test2", "ref-fallback-a", "${ test.key-a ? fail }", "a", false},
    {"test2", "ref-nexist", "${test.key-nexist? \" f a i l ' }", "\" f a i l '", false},
    {"test2", "ref-fail", "${test.key-fail}", "${test.key-fail}", true},
    {"test2", "ref-fake", "{test.key-a}", "{test.key-a}", false},
  },
  {
    {"test", "ref-cyclic-1", "${ref-cyclic-2}", "${ref-cyclic-1}", false},
    {"test", "ref-cyclic-2", "${ref-cyclic-1}", "${ref-cyclic-1}", true}
  },
  {{"test2", "file-delink", "${file: delink_file.txt }", "content", false}},
  {{"test2", "file-default", "${file:delink_file.txt?fail}", "content", false}},
  {{"test2", "file-double-default", "${file:nexist.txt ? ${file:delink_file.txt}}", "content", false}},
  {{"test2", "file-nexist", "${file:nexist.txt ? \" f a i l ' }", "\" f a i l '", false}},
  {{"test2", "file-fail", "${file:nexist.txt}", "${file:nexist.txt}", true}},
  {{"test2", "cmd", "${cmd:echo hello world}", "hello world", false}},
  {{"test2", "cmd", "${cmd:nexist}", "", true}},
  {{"test2", "env", "${env: test_env? fail}", "test_env"}},  {{"test2", "env-nexist", "${env:nexist? \" f a i l \" }", " f a i l ", false}},
  {
    {"test2", "color", "${color: #123456 }", "#123456", false},
    {"test2", "color-hsv", "${color: hsv(180, 1, 0.75)}", "#00BFBF", false},
    {"test2", "color-ref", "${color: $color}", "#123456", false},
    {"test2", "color-mod", "${color: cielch: lum * 1.5, hue + 60; $color}", "#633E5C", false},
  },
};
class DelinkTest : public ::testing::Test, public ::testing::WithParamInterface<delink_test> {};
INSTANTIATE_TEST_SUITE_P(TString, DelinkTest, ::testing::ValuesIn(delink_tests));

TEST_P(DelinkTest, general) {
  str_errlist err;
  setenv("test_env", "test_env", true);
  unsetenv("nexist");
  document doc;
  auto testset = GetParam();
  for(auto test : testset)
    doc.add_onetime(test.section, test.key, move(test.value));
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
