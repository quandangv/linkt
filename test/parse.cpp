#include "parse.hpp"
#include "delink.hpp"
#include "test.h"

#include <algorithm>

#include "logger.hpp"

using namespace std;
struct parse_result {
  vector<tuple<string, string, string>> keys;
  vector<tuple<string, string, string>> delinked_keys;
  vector<int> err;
};
using TestSet = pair<string, parse_result>;

class GetTest : public ::testing::Test, public ::testing::WithParamInterface<TestSet> {};

vector<TestSet> parse_tests = {
  {"key-rogue=rogue \n\
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
ref-nexist   = ${test.key-nexist:failed} \n\
ref-fallback-a = ${test.key-a:failed} \n\
ref-fail   = ${test.key-fail} \n\
ref-fake   = {test.key-a} \n\
key-a = '    a\"",
    {
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
        {"test2", "ref-nexist", "${test.key-nexist:failed}"},
        {"test2", "ref-fallback-a", "${test.key-a:failed}"},
        {"test2", "ref-fail", "${test.key-fail}"},
        {"test2", "ref-fake", "{test.key-a}"},
        {"test2", "key-a", "'    a\""},
      },
      { // delinked_keys
        {"", "key-rogue", "rogue"},
        {"test", "key-a", "a"},
        {"test", "key-cmt", ";cmt"},
        {"test", "key-c", "c"},
        {"test", "key-empty", ""},
        {"test", "key-test", "test"},
        {"test2", "key-test2", "test2"},
        {"test2", "key-b", "b  "},
        {"test2", "key-c", "  c  "},
        {"test", "ref-ref-a", "a"},
        {"test", "ref-cyclic-1", ""},
        {"test", "ref-cyclic-2", ""},
        {"test2", "ref-a", "a"},
        {"test2", "ref-rogue", "rogue"},
        {"test2", "ref-nexist", "failed"},
        {"test2", "ref-fallback-a", "a"},
        {"test2", "ref-fail", "${test.key-fail}"},
        {"test2", "ref-fake", "{test.key-a}"},
        {"test2", "key-a", "'    a\""},
        {"test2", "ref-a", "a"},
      },
      { // err
        2, 3, 6, 8, 12, 20, 22
      }
    }
  }
};
INSTANTIATE_TEST_SUITE_P(Parse, GetTest, ::testing::ValuesIn(parse_tests));

TEST_P(GetTest, parse_string) {
  stringstream ss{GetParam().first};
  document doc;
  errorlist err;
  parse(ss, doc, err);
  // cout << to_string(doc);
  auto expected = GetParam().second;
  for(auto& line : expected.err) {
    auto pos = find_if(err.begin(), err.end(), [&](auto it) { return it.first == line; });
    EXPECT_NE(pos, err.end()) << "line = " << line;
    if (pos != err.end())
      err.erase(pos);
  }
  for(auto& e : err) {
    EXPECT_FALSE(true)
      << "Line num: " << e.first << endl
      << "Message: " << e.second;
  }
  vector<string> found;
  for(auto& key : expected.keys) {
    auto& section = doc[get<0>(key)];
    auto fullkey = get<0>(key) + "." + get<1>(key);
    EXPECT_TRUE(section.find(get<1>(key)) != section.end())
        << "Parse, find: Key: " << fullkey << endl;
    EXPECT_EQ(section[get<1>(key)], get<2>(key))
        << "Parse, compare: Key: " << fullkey << endl;
    found.emplace_back(fullkey);
  }
  for(auto& section : doc) {
    for(auto& keyval : section.second) {
      auto fullkey = section.first + "." + keyval.first;
      EXPECT_NE(std::find(found.begin(), found.end(), fullkey), found.end())
          << "Parse, excess: Key: " << fullkey << std::endl
          << "Value: " << keyval.second << endl;
    }
  }
  found.clear();

  str_errlist str_err;
  delink(doc, str_err);
  for(auto& key : expected.delinked_keys) {
    auto& section = doc[get<0>(key)];
    auto fullkey = get<0>(key) + "." + get<1>(key);
    EXPECT_TRUE(section.find(get<1>(key)) != section.end())
        << "Delink, find: Key: " << fullkey << endl;
    EXPECT_EQ(section[get<1>(key)], get<2>(key))
        << "Delink, compare: Key: " << fullkey << endl;
    found.emplace_back(fullkey);
  }
  for(auto& section : doc) {
    for(auto& keyval : section.second) {
      auto fullkey = section.first + "." + keyval.first;
      EXPECT_NE(std::find(found.begin(), found.end(), fullkey), found.end())
          << "Delink, excess: Key: " << fullkey<< std::endl
          << "Value: " << keyval.second << endl;
    }
  }
  found.clear();
}
