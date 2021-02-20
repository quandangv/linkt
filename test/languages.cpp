#include "languages.hpp"
#include "test.h"

#include <fstream>
#include <unistd.h>

using namespace lini;

struct file_test_param {
  struct expectation { string path, value; };

  string path, language;
  vector<expectation> expectations;
  vector<string> err;
};

vector<file_test_param> parse_tests = {
  {
    "ini_test", "ini",
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
      {"test.ref-ref-a", "${test2.ref-a ? failed}"},
      {"test2.ref-a", "${test.key-a}"},
      {"test2.ref-rogue", "${.key-rogue}"},
      {"test2.ref-nexist", "${test.key-nexist ? \" f a i l ' }"},
      {"test2.key-a", "'    a\""},
    },
    {
      "line 2",
      "line 7",
      "line 9",
      "line 13",
      "line 19",
      "line 21",
    }
  },
  {
    "lemonbar_test", "ini",
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
  },
  {
    "yml_test", "yml",
    {
      {"bar.bat.B.L.value", "60"},
      {"bar.bat.B", "#FF54CB"},
      {"bar.bat", "%{B#FF54CB} BAT 60%"},
      {"bar.F", "#fff"},
      {"bar", "%{F#fff}%{B#FF54CB} BAT 60% "},
    },
    {}
  },
};

struct file_test : public TestWithParam<file_test_param> {
  void test() {
    auto testset = GetParam();
    std::ifstream ifs{testset.path + ".txt"};
    ASSERT_FALSE(ifs.fail());

    wrapper doc;
    errorlist err;
    if (testset.language == "ini")
      parse_ini(ifs, doc, err);
    else if (testset.language == "yml")
      parse_yml(ifs, doc, err);

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
      } catch (const std::exception& e) {
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

    // Check wrapper export
    std::ofstream ofs{testset.path + "_export.txt"};
    if (testset.language == "ini")
      write_ini(ofs, doc);
    else if (testset.language == "yml")
      write_yml(ofs, doc);
    auto command = "diff -Z '" + testset.path + "_output.txt' '" + testset.path + "_export.txt' 2>&1";
    auto file = popen(command.data(), "r");
    ASSERT_TRUE(file);
    std::array<char, 2> buf;
    fgets(buf.data(), 2, file);
    EXPECT_TRUE(feof(file))
        << "Output of command not empty: " << command;
    pclose(file);
  }
};

INSTANTIATE_TEST_SUITE_P(parse, file_test, ValuesIn(parse_tests));

TEST_P(file_test, general) {
  test();
}

wrapper doc;
void set_key(const string& key, const string& newval) {
  EXPECT_TRUE(doc.set(key, newval));
  ASSERT_EQ(newval, doc.get_child(key)) << "Unexpected value after assignment";
}

TEST(assign_test, load_doc) {
  std::ifstream ifs{"assign_test.txt"};
  ASSERT_FALSE(ifs.fail());
  errorlist err;
  parse_ini(ifs, doc, err);
  ASSERT_TRUE(err.empty());
  ifs.close();
}

TEST(assign_test, doc) {
  // Test wrapper functionalities
  EXPECT_FALSE(doc.get_child("nexist"_ts));
  EXPECT_FALSE(doc.has_child("nexist"_ts));
  EXPECT_EQ(doc.get_child("nexist"_ts, "fallback"), "fallback");
  EXPECT_THROW(doc.get_child_ref("nexist"_ts), string_ref::error);
  EXPECT_EQ(doc.get_child("key-a"_ts, "fallback"), "a");
  EXPECT_EQ(doc.get_child_ref("key-a"_ts).get(), "a");
  EXPECT_EQ(doc.get(), "");
  EXPECT_THROW(doc.get_child("ref-fail"_ts), container::error);
}

TEST(assign_test, local_ref) {
  // Test local_ref assignments
  EXPECT_NO_FATAL_FAILURE(set_key("key-a", "a"));
  EXPECT_NO_FATAL_FAILURE(set_key("ref-a", "foo"));
  EXPECT_NO_FATAL_FAILURE(set_key("ref-ref-a", "bar"));
  EXPECT_NO_FATAL_FAILURE(EXPECT_EQ("bar", *doc.get_child("key-a"_ts)));

  // Test fallback assignments
  EXPECT_NO_FATAL_FAILURE(set_key("ref-default-a", "foobar"));
  EXPECT_NO_FATAL_FAILURE(EXPECT_EQ("foobar", *doc.get_child("key-a"_ts)));
}

TEST(assign_test, file_env_ref) {
  // Test file_ref assignments
  EXPECT_NO_FATAL_FAILURE(set_key("ref-nexist", "barfoo"));
  EXPECT_NO_FATAL_FAILURE(set_key("env-nexist", "barbar"));
  EXPECT_NO_FATAL_FAILURE(set_key("file-parse", "foo"));
  std::ifstream ifs("key_file.txt");
  string content;
  getline(ifs, content);
  ifs.close();
  EXPECT_NO_FATAL_FAILURE(EXPECT_EQ("foo", content));

  // Test env_ref assignments
  EXPECT_NO_FATAL_FAILURE(set_key("env", "foo"));

  // Revert file contents back to its original
  EXPECT_NO_FATAL_FAILURE(set_key("file-parse", "content"));
}
