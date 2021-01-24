#include "parse.hpp"
#include "test.h"

#include <fstream>
#include <unistd.h>

using namespace lini;

struct ParseSet {
  string path;
  vector<tuple<string, string, string>> keys;
  vector<string> err;
  string initial_section;
};

vector<ParseSet> parse_tests = {
  {
    "parse_test",
    {
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
      {"test2", "ref-a", "${test.key-a}"},
      {"test2", "ref-rogue", "${.key-rogue}"},
      {"test2", "ref-nexist", "${test.key-nexist: \" f a i l ' }"},
      {"test2", "key-a", "'    a\""},
    },
    {
      "line 2",
      "line 3",
      "line 7",
      "line 9",
      "line 13",
      "key test2.key-b",
      "key test2.key-c"
    }, ""
  },
};

TEST(Parse, general) {
  for(auto& parse_test : parse_tests) {
    ifstream ifs{parse_test.path + ".txt"};
    ASSERT_FALSE(ifs.fail());

    document doc;
    errorlist err;
    parse(ifs, doc, err, parse_test.initial_section);
    for(auto& e : parse_test.err) {
      auto pos = find_if(err.begin(), err.end(), [&](auto it) { return it.first == e; });
      EXPECT_NE(pos, err.end()) << "Expected parsing error at: " << e;
    }
    for(auto& e : err) {
      EXPECT_NE(find(parse_test.err.begin(), parse_test.err.end(), e.first), parse_test.err.end())
        << "Excess parsing error, at: " << e.first << endl
        << "Message: " << e.second;
    }
    vector<string> found;
    for(auto& key : parse_test.keys) {
      auto& section = doc.map[get<0>(key)];
      auto fullkey = get<0>(key) + "." + get<1>(key);
      EXPECT_TRUE(section.find(get<1>(key)) != section.end())
          << "Parse, find: Key: " << fullkey << endl;
      EXPECT_EQ((*doc.values[section[get<1>(key)]])->get(), get<2>(key))
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

    // Check document export
    ofstream ofs{parse_test.path + "_export.txt"};
    write(ofs, doc);
    auto file = popen(("diff " + parse_test.path + ".txt " + parse_test.path + "_export.txt").data(), "r");
    ASSERT_TRUE(file);
    array<char, 2> buf;
    fgets(buf.data(), 1, file);
    EXPECT_TRUE(feof);
    pclose(file);
  }
}

TEST(parse, assign_test) {
  document doc;
  auto set_key = [&](const string& key, const string& newval) {
    // Make sure the key is assignable
    auto index = doc.find("", key);
    ASSERT_TRUE(index) << "Key not found";
    auto& ref = **doc.values[*index];
    ASSERT_FALSE(ref.readonly()) << "Key is readonly";

    ref.set(newval);
    ASSERT_EQ(newval, doc.get("", key));
  };

  ifstream ifs{"assign_test.txt"};
  ASSERT_FALSE(ifs.fail());
  errorlist err;
  parse(ifs, doc, err);
  ASSERT_TRUE(err.empty());
  ifs.close();

//  // Test document functionalities
//  EXPECT_FALSE(doc.get("", "nexist"));
//  EXPECT_EQ(doc.get("", "nexist", "fallback"), "fallback");
//  EXPECT_EQ(doc.get("", "key-a", "fallback"), "a");
//
//  // Test local_ref assignments
//  EXPECT_NO_FATAL_FAILURE(set_key("key-a", "a"));
//  EXPECT_NO_FATAL_FAILURE(set_key("ref-a", "foo"));
//  EXPECT_NO_FATAL_FAILURE(set_key("ref-ref-a", "bar"));
//  EXPECT_EQ("bar", *doc.get("", "key-a"));
//
//  // Test fallback assignments
//  EXPECT_NO_FATAL_FAILURE(set_key("ref-default-a", "foobar"));
//  EXPECT_EQ("foobar", *doc.get("", "key-a"));
//
//  // Test file_ref assignments
//  EXPECT_NO_FATAL_FAILURE(set_key("ref-nexist", "barfoo"));
//  EXPECT_NO_FATAL_FAILURE(set_key("env-nexist", "barbar"));
//  EXPECT_NO_FATAL_FAILURE(set_key("file-parse", "foo"));
//  ifs.open("key_file.txt");
//  string content;
//  getline(ifs, content);
//  ifs.close();
//  EXPECT_EQ("foo", content);
//  
//  // Test env_ref assignments
//  EXPECT_NO_FATAL_FAILURE(set_key("env", "foo"));
//
//  // Revert file contents back to its original
//  EXPECT_NO_FATAL_FAILURE(set_key("file-parse", "content"));
}

