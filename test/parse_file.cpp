#include "parse.hpp"
#include "test.h"

#include <fstream>
#include <unistd.h>

using namespace lini;

struct file_test_param {
  struct expectation { string path, value; };

  string path;
  vector<expectation> expectations;
  vector<string> err;
};

vector<file_test_param> parse_tests = {
  {
    "parse_test",
    {
      {"key-rogue", "rogue"},
      {"test.key-a", "a"},
      {"test.key-cmt", ";cmt"},
      {"test.key-c", "c"},
      {"test.key-empty", ""},
      {"test.key-test", "test"},
      {"test.key", "key"},
      {"test2.key-test2", "test2"},
      {"test2.key-b", "b  "},
      {"test2.key-c", "  c  "},
      {"test.ref-ref-a", "${test2.ref-a:failed}"},
      {"test2.ref-a", "${test.key-a}"},
      {"test2.ref-rogue", "${.key-rogue}"},
      {"test2.ref-nexist", "${test.key-nexist: \" f a i l ' }"},
      {"test2.key-a", "'    a\""},
    },
    {
      "line 2",
      "line 3",
      "line 7",
      "line 9",
      "line 13",
      "line 19",
      "line 21",
      "line 31",
    }
  },
  {
    "lemonbar_test",
    {
      {"msg", "hello"},
      {"simple", " 69 96 0 "},
      {"compact", " %{F#f00}CPU 69% %{F#ff0}RAM 96% %{F#0f0}TEMP 99*C %{F#0ff}BAT 0% "},
      {"mod.cpu", "%{F#f00}CPU 69%"},
      {"mod.ram", "%{F#ff0}RAM 96%"},
      {"mod.temp", "%{F#0f0}TEMP 99*C"},
      {"mod.bat", "%{F#0ff}BAT 0%"},
      {"stat.cpu", "69"},
      {"stat.ram", "96"},
      {"stat.temp-c", "99"},
      {"stat.bat", "0"},
    },
    {}
  }
};

struct file_test : public TestWithParam<file_test_param> {
  void test() {
    auto testset = GetParam();
    ifstream ifs{testset.path + ".txt"};
    ASSERT_FALSE(ifs.fail());

    document doc;
    errorlist err;
    parse(ifs, doc, err);

    // Check for unexpected errors
    for(auto& e : err) {
      EXPECT_NE(find(testset.err.begin(), testset.err.end(), e.first), testset.err.end())
        << "Excess parsing error, at: " << e.first << endl
        << "Message: " << e.second;
    }
    // Check the keys
    vector<string> found;
    for(auto& pair : testset.expectations) {
      try {
        auto result = doc.get_child(pair.path);
        EXPECT_TRUE(result)
            << "Key doesn't exist: " << pair.path << endl;
        EXPECT_EQ(*result, pair.value)
            << "Key have wrong value: " << pair.path << endl;
      } catch (const exception& e) {
        ADD_FAILURE() << "Key: " << pair.path << endl
            << "Exception while checking: " << e.what();
      }
      found.emplace_back(pair.path);
    }
    // Check for expected errors
    for(auto& e : testset.err) {
      auto pos = find_if(err.begin(), err.end(), [&](auto it) { return it.first == e; });
      EXPECT_NE(pos, err.end()) << "Expected parsing error at: " << e;
    }

    //// Check document export
    //ofstream ofs{testset.path + "_export.txt"};
    //write(ofs, doc);
    //auto file = popen(("diff " + testset.path + ".txt " + testset.path + "_export.txt").data(), "r");
    //ASSERT_TRUE(file);
    //array<char, 2> buf;
    //fgets(buf.data(), 1, file);
    //EXPECT_TRUE(feof);
    //pclose(file);
  }
};

INSTANTIATE_TEST_SUITE_P(parse, file_test, ValuesIn(parse_tests));

TEST_P(file_test, general) {
  test();
}

TEST(parse, assign_test) {
  document doc;
  auto set_key = [&](const string& key, const string& newval) {
    // Make sure the key is assignable
    doc.set(key, newval);
    ASSERT_EQ(newval, doc.get_child(key)) << "Unexpected value after assignment";
  };

  ifstream ifs{"assign_test.txt"};
  ASSERT_FALSE(ifs.fail());
  errorlist err;
  parse(ifs, doc, err);
  ASSERT_TRUE(err.empty());
  ifs.close();

  // Test document functionalities
  EXPECT_NO_FATAL_FAILURE(EXPECT_FALSE(doc.get_child("nexist")));
  EXPECT_NO_FATAL_FAILURE(EXPECT_EQ(doc.get_child("nexist", "fallback"), "fallback"));
  EXPECT_NO_FATAL_FAILURE(EXPECT_EQ(doc.get_child("key-a", "fallback"), "a"));

  // Test local_ref assignments
  EXPECT_NO_FATAL_FAILURE(set_key("key-a", "a"));
  EXPECT_NO_FATAL_FAILURE(set_key("ref-a", "foo"));
  EXPECT_NO_FATAL_FAILURE(set_key("ref-ref-a", "bar"));
  EXPECT_NO_FATAL_FAILURE(EXPECT_EQ("bar", *doc.get_child("key-a")));

  // Test fallback assignments
  EXPECT_NO_FATAL_FAILURE(set_key("ref-default-a", "foobar"));
  EXPECT_NO_FATAL_FAILURE(EXPECT_EQ("foobar", *doc.get_child("key-a")));

  // Test file_ref assignments
  EXPECT_NO_FATAL_FAILURE(set_key("ref-nexist", "barfoo"));
  EXPECT_NO_FATAL_FAILURE(set_key("env-nexist", "barbar"));
  EXPECT_NO_FATAL_FAILURE(set_key("file-parse", "foo"));
  ifs.open("key_file.txt");
  string content;
  getline(ifs, content);
  ifs.close();
  EXPECT_NO_FATAL_FAILURE(EXPECT_EQ("foo", content));
  
  // Test env_ref assignments
  EXPECT_NO_FATAL_FAILURE(set_key("env", "foo"));

  // Revert file contents back to its original
  EXPECT_NO_FATAL_FAILURE(set_key("file-parse", "content"));

  // Test document key
  EXPECT_NO_FATAL_FAILURE(EXPECT_EQ(doc.get_child("doc.foo", "fail"), "hello"));
  EXPECT_NO_FATAL_FAILURE(EXPECT_EQ(doc.get_child("doc.bar", "fail"), "world"));
}

