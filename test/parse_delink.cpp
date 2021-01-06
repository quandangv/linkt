#include "parse_delink.hpp"
#include "test.h"

#include <algorithm>

using namespace std;
using namespace lini;

struct ParseSet {
  string source;
  vector<tuple<string, string, string>> keys;
  vector<int> err;
  string initial_section;
  char line_separator;
};

vector<ParseSet> parse_tests = {
  {
"key-rogue=rogue \n\
[test-excess];excess\n\
[test.forbidden]\n\
[test]\n\
key-a = a \n\
    key b = b\n\
key-cmt = ;cmt\n\
key#forbidden = a\n\
; comment = abc\n\
 key-c=c\n\
key-empty=   \n\
[test-missing\n\
key-test= test \n\
ref-ref-a=${test2.ref-a:failed}\n\
ref-cyclic-1 = ${ref-cyclic-2}\n\
ref-cyclic-2 = ${ref-cyclic-1}\n\
[test2]\n\
key-test2 = test2\n\
key-b='b  '\n\
key-b = 'dup'\n\
key-c = \"  c  \"   \n\
key-c = \"dup\"   \n\
ref-a = ${test.key-a} \n\
ref-rogue= ${.key-rogue} \n\
ref-nexist   = ${test.key-nexist: \" f a i l ' } \n\
ref-fallback-a = ${ test.key-a : failed } \n\
ref-fail   = ${test.key-fail} \n\
ref-fake   = {test.key-a} \n\
\n\
key-a = '    a\"",
    { // keys
      {"", "key-rogue", "rogue"},
      {"test", "key-a", "a"},
      {"test", "key-cmt", ";cmt"},
      {"test", "key-c", "c"},
      {"test", "key-empty", ""},
      {"test", "key-test", "test"},
      {"test2", "key-test2", "test2"},
      {"test2", "key-b", "b  "},
      {"test2", "key-c", "  c  "},
      {"test", "ref-ref-a", "${test2.ref-a:failed}"},
      {"test", "ref-cyclic-1", "${ref-cyclic-2}"},
      {"test", "ref-cyclic-2", "${ref-cyclic-1}"},
      {"test2", "ref-a", "${test.key-a}"},
      {"test2", "ref-rogue", "${.key-rogue}"},
      {"test2", "ref-nexist", "${test.key-nexist: \" f a i l ' }"},
      {"test2", "ref-fallback-a", "${ test.key-a : failed }"},
      {"test2", "ref-fail", "${test.key-fail}"},
      {"test2", "ref-fake", "{test.key-a}"},
      {"test2", "key-a", "'    a\""},
    },
    { 2, 3, 6, 8, 12, 20, 22 }, "", '\n'
  },
  {
    "norm=#abc#123#456 highlight=#000#fff space=5pt",
    {
      {"styles", "norm", "#abc#123#456"},
      {"styles", "highlight", "#000#fff"},
      {"styles", "space", "5pt"},
    }, {}, "styles", ' '
  }
};

TEST(Parse, general) {
  for(auto& parse_test : parse_tests) {
    stringstream ss{parse_test.source};
    document doc;
    errorlist err;
    parse(ss, doc, err, parse_test.initial_section, parse_test.line_separator);
    for(auto& line : parse_test.err) {
      auto pos = find_if(err.begin(), err.end(), [&](auto it) { return it.first == line; });
      EXPECT_NE(pos, err.end()) << "Expected parsing error: " << line;
      if (pos != err.end())
        err.erase(pos);
    }
    for(auto& e : err) {
      EXPECT_FALSE(true)
        << "Excess parsing error, line num: " << e.first << endl
        << "Message: " << e.second;
    }
    vector<string> found;
    for(auto& key : parse_test.keys) {
      auto& section = doc.map[get<0>(key)];
      auto fullkey = get<0>(key) + "." + get<1>(key);
      EXPECT_TRUE(section.find(get<1>(key)) != section.end())
          << "Parse, find: Key: " << fullkey << endl;
      EXPECT_EQ(doc.values[section[get<1>(key)]]->get(), get<2>(key))
          << "Parse, compare: Key: " << fullkey << endl;
      found.emplace_back(fullkey);
    }
    for(auto& section : doc.map) {
      for(auto& keyval : section.second) {
        auto fullkey = section.first + "." + keyval.first;
        EXPECT_NE(std::find(found.begin(), found.end(), fullkey), found.end())
            << "Parse, excess key: " << fullkey << std::endl
            << "Value: " << keyval.second << endl;
      }
    }
  }
}

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
    {"test", "key-a", "a", "a", false},
    {"test2", "ref-a", "${test.key-a}", "a", false},
    {"test", "ref-ref-a", "${test2.ref-a:failed}", "a", false},
    {"test", "ref-self-a", "${key-a:failed}", "a", false},
    {"test2", "ref-fallback-a", "${ test.key-a : fail }", "a", false},
    {"test2", "ref-nexist", "${test.key-nexist: \" f a i l ' }", "\" f a i l '", false},
    {"test2", "ref-fail", "${test.key-fail}", "${test.key-fail}", true},
    {"test2", "ref-fake", "{test.key-a}", "{test.key-a}", false},
  },
  {
    {"test", "ref-cyclic-1", "${ref-cyclic-2}", "${ref-cyclic-1}", false},
    {"test", "ref-cyclic-2", "${ref-cyclic-1}", "${ref-cyclic-1}", true}
  },
  {{"test2", "file-delink", "${file: delink_file.txt }", "content", false}},
  {{"test2", "file-default", "${file:delink_file.txt:fail}", "content", false}},
  {{"test2", "file-nexist", "${file:nexist.txt: \" f a i l ' }", "\" f a i l '", false}},
  {{"test2", "file-fail", "${file:nexist.txt}", "${file:nexist.txt}", true}},
  {{"test2", "env", "${env: test_env: fail}", "test_env"},    {"test2", "env-nexist", "${env:nexist: \" f a i l \" }", " f a i l ", false}},
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
    } catch (const exception&) {
      EXPECT_TRUE(test.fail)
          << "Key: " << fullkey << endl
          << "Exception got thrown, but the test is not expected to fail";
    }
  }
}
