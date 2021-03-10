#include "test.h"

#include <fstream>
#include <thread>
#include <unistd.h>

struct file_test_param {
  struct expectation { string path, value; };

  string path, language;
  vector<expectation> expectations;
  vector<string> err;
};

void test_language(file_test_param testset) {
  std::ifstream ifs{testset.path + ".txt"};
  ASSERT_FALSE(ifs.fail());

  // Parse the file
  node::errorlist err;
  node::wrapper_s doc;
  if (testset.language == "ini")
    doc = parse_ini(ifs, err);
  else if (testset.language == "yml")
    doc = parse_yml(ifs, err);

  // Check for unexpected errors
  for(auto& e : err) {
    EXPECT_NE(find(testset.err.begin(), testset.err.end(), e.first), testset.err.end())
      << "Unexpected parsing error, at: " << e.first << endl << "Message: " << e.second;
  }
  // Check for expected errors
  for(auto& e : testset.err) {
    auto pos = find_if(err.begin(), err.end(), [&](auto it) { return it.first == e; });
    EXPECT_NE(pos, err.end()) << "Expected parsing error at: " << e;
  }

  // Check the keys
  auto test_doc = [&](node::base_s node, const node::errorlist& errs) {
    auto doc = dynamic_cast<node::wrapper*>(node.get());
    for(auto& err : errs)
      ADD_FAILURE() << "Error while cloning: " << err.first << ':' << err.second;
    for(auto& pair : testset.expectations)
      check_key(*doc, pair.path, pair.value, false);
  };
  triple_node_test(doc, test_doc);

  // Export the result
  std::ofstream ofs{testset.path + "_export.txt"};
  if (testset.language == "ini")
    write_ini(ofs, doc);
  else if (testset.language == "yml")
    write_yml(ofs, doc);

  // Check the export result
  auto command = "diff '" + testset.path + "_output.txt' '" + testset.path + "_export.txt' 2>&1";
  auto file = popen(command.data(), "r");
  ASSERT_TRUE(file);
  EXPECT_EQ(fgetc(file), EOF) << "Output of command not empty: " << command;
  pclose(file);
}

TEST(Language, Plain_ini) {
  test_language({"ini_test", "ini",
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
    { "line 2", "line 7", "line 9", "line 13", "line 19", "line 21", }
  });
}

TEST(Language, Functional_ini) {
  test_language({"lemonbar_test", "ini",
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
  });
}

TEST(Language, Yml) {
  test_language({"yml_test", "yml",
    {
      {"bar.base.B.dumb", "hello"},
      {"bar.bat.B.L.value", "60"},
      {"bar.bat.B.L", "0.800000"},
      {"bar.bat.stat", "60"},
      {"bar.bat.unit", "%"},
      {"bar.bat.B.dumb", "hello"},
      {"bar.bat.B", "#FF54CB"},
      {"bar.bat", "%{B#FF54CB} BAT 60%"},
      {"bar.F", "#fff"},
      {"bar", "%{F#fff}%{B#FF54CB} BAT 60% "},
    },
    { "line 13, key B", "line 14", "line 15, key label", "line 17, key dumb2" }
  });
}

node::wrapper_s load_doc() {
  // Load the test file
  std::ifstream ifs{"misc_test.txt"};
  EXPECT_FALSE(ifs.fail());
  node::errorlist err;
  auto doc = parse_ini(ifs, err);
  ifs.close();

  if (!err.empty()) {
    for(auto& e : err)
      ADD_FAILURE() << "At " << e.first << ": " << e.second << endl;
    throw std::logic_error("Failed loading document");
  }
  return doc;
}
node::wrapper_s load_optimized_doc() {
  auto doc = load_doc();
  node::clone_context context;
  //context.no_dependency = true;
  doc->optimize(context);

  if (!context.errors.empty()) {
    for(auto& e : context.errors)
      ADD_FAILURE() << "At " << e.first << ": " << e.second << endl;
    throw std::logic_error("Failed loading document");
  }
  return doc;
}

vector<node::wrapper_s> tests{load_doc(), load_optimized_doc()};
struct Misc : TestWithParam<node::wrapper_s> {};
INSTANTIATE_TEST_SUITE_P(wrapper, Misc, ValuesIn(tests));

// Set a key and check if it was successful
void set_key(node::wrapper_s& doc, const string& key, const string& newval) {
  auto last_count = get_test_part_count();
  EXPECT_TRUE(doc->set(key, newval));
  ASSERT_EQ(newval, doc->get_child(key)) << "Unexpected value after assignment";
  if (last_count != get_test_part_count())
    cerr << "Key: " << key << endl;
}

TEST_P(Misc, wrapper) {
  auto doc = GetParam();
  // Test wrapper functionalities
  EXPECT_FALSE(doc->get_child("nexist"_ts));
  EXPECT_EQ(doc->get_child("nexist"_ts, "fallback"), "fallback");
  EXPECT_EQ(doc->get_child_place("nexist"_ts), nullptr);
  EXPECT_EQ(doc->get_child("key-a"_ts, "fallback"), "a");
  EXPECT_EQ(doc->get(), "");
  EXPECT_ANY_THROW(doc->get_child("ref-fail"_ts));
  doc->add("manual"_ts, std::make_shared<node::plain>("hello"));
  EXPECT_EQ(doc->get_child("manual"_ts, "fail"), "hello");
}

TEST_P(Misc, parse_errors) {
  auto doc = GetParam();
  node::parse_context test_context{nullptr, nullptr, nullptr};
  EXPECT_THROW(test_context.get_current(), node::parse_error);
  EXPECT_THROW(test_context.get_place(), node::parse_error);
  EXPECT_THROW(test_context.get_parent(), node::parse_error);
  EXPECT_THROW({
    node::throwing_clone_context context;
    context.no_dependency = true;
    doc->get_child_ptr("ref-a"_ts)->clone(context);
  }, node::node_error);
}

TEST_P(Misc, save_cache) {
  auto doc = GetParam();
  EXPECT_EQ(doc->get_child("appender.last"_ts, "fail"), "I");
  EXPECT_EQ(doc->get_child("appender"_ts, "fail"), "I eat.");
  EXPECT_EQ(doc->get_child("appender.last"_ts, "fail"), "I eat");
  EXPECT_EQ(doc->get_child("appender"_ts, "fail"), "I eat eat.");
  EXPECT_EQ(doc->get_child("appender.last"_ts, "fail"), "I eat eat");
  EXPECT_EQ(doc->get_child("cache"_ts, "fail"), "I eat eat eat.");
  EXPECT_EQ(doc->get_child("cache"_ts, "fail"), "I eat eat eat.");
  EXPECT_EQ(doc->get_child("cache"_ts, "fail"), "I eat eat eat.");
  EXPECT_EQ(doc->get_child("cache_too_short"_ts, "fail"), "I eat eat eat eat.");
  EXPECT_EQ(doc->get_child("cache_too_short"_ts, "fail"), "I eat eat eat eat eat.");
}

TEST_P(Misc, array_cache) {
  auto doc = GetParam();
  EXPECT_EQ(doc->get_child("multiplier"_ts, "fail"), "0 10");
  EXPECT_EQ(doc->get_child("array_cache"_ts, "fail"), "0 10 10");
  EXPECT_EQ(doc->get_child("array_cache"_ts, "fail"), "0 10 10");
  set_key(doc, "multiplier.source", "2");
  EXPECT_EQ(doc->get_child("array_cache"_ts, "fail"), "0 10 10 20");
  EXPECT_EQ(doc->get_child("array_cache2"_ts, "fail"), "0 10 10 20");
}

TEST_P(Misc, clock) {
  auto doc = GetParam();
  auto clock = stoi(doc->get_child("clock"_ts, "fail"));
  EXPECT_LT(clock, 1000);
  EXPECT_GE(clock, 0);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  auto clock2 = stoi(doc->get_child("clock"_ts, "fail"));
  EXPECT_NE(clock, clock2);
}

TEST_P(Misc, assign_ref) {
  auto doc = GetParam();
  // Test local_ref assignments
  set_key(doc, "key-a", "b");
  set_key(doc, "key-a", "a");
  set_key(doc, "ref-a", "foo");
  set_key(doc, "ref-ref-a", "bar");
  EXPECT_EQ("bar", *doc->get_child("key-a"_ts));

  // Test fallback assignments
  set_key(doc, "ref-default-a", "foobar");
  EXPECT_EQ("foobar", *doc->get_child("key-a"_ts));
  EXPECT_FALSE(doc->set("cmd-ref"_ts, "hello"));
}

TEST_P(Misc, assign_file_env) {
  auto doc = GetParam();
  // Test file_ref assignments
  set_key(doc, "ref-nexist", "barfoo");
  set_key(doc, "env-nexist", "barbar");
  set_key(doc, "file-parse", "foo");
  std::ifstream ifs("key_file.txt");
  string content;
  getline(ifs, content);
  ifs.close();
  EXPECT_EQ("foo", content);

  // Test env_ref assignments
  set_key(doc, "env", "foo");
  set_key(doc, "file-parse", "content");
}
