#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using namespace std::chrono;

const std::string TEMPLATE_DIR =
    std::string(getenv("HOME")) + "/.cp_helper/templates/";

struct CommandArgs {
  std::string command;
  std::string filename;
  std::string language;
};

std::string get_cpp_template() {
  return R"(#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    return 0;
})";
}

std::string get_py_template() {
  return R"(import sys

input = sys.stdin.read

def main():
    pass

if __name__ == '__main__':
    main())";
}

std::string get_java_template() {
  return R"(public class Main {
    public static void main(String[] args) {
        System.out.println("Hello World");
    }
})";
}

void create_file(const std::string &filename, const std::string &content) {
  std::ofstream file(filename);
  if (file) {
    file << content;
    std::cout << "Created " << filename << std::endl;
  } else {
    std::cerr << "Error creating file: " << filename << std::endl;
  }
}

void handle_new(const CommandArgs &args) {
  std::string ext = args.filename.substr(args.filename.find_last_of(".") + 1);
  std::string content;

  if (ext == "cpp")
    content = get_cpp_template();
  else if (ext == "py")
    content = get_py_template();
  else if (ext == "java")
    content = get_java_template();
  else {
    std::cerr << "Unsupported file type: " << ext << std::endl;
    return;
  }

  create_file(args.filename, content);
}

void handle_compile(const CommandArgs &args) {
  std::string cmd;
  std::string ext = args.filename.substr(args.filename.find_last_of(".") + 1);

  if (ext == "cpp") {
    cmd = "g++-14 -O2 -Wall " + args.filename + " -o " +
          args.filename.substr(0, args.filename.find_last_of("."));
  } else if (ext == "java") {
    cmd = "javac " + args.filename;
  } else {
    std::cout << "No compilation needed" << std::endl;
    return;
  }

  int result = std::system(cmd.c_str());
  if (result != 0) {
    std::cerr << "Compilation failed!" << std::endl;
  } else {
    std::cout << "Compilation successful" << std::endl;
  }
}

std::string execute_program(const std::string &cmd, const std::string &input) {
  std::string full_cmd = "echo \"" + input + "\" | " + cmd;
  FILE *pipe = popen(full_cmd.c_str(), "r");
  if (!pipe)
    return "ERROR";

  char buffer[128];
  std::string result;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    result += buffer;
  }
  pclose(pipe);
  return result;
}

void handle_test(const CommandArgs &args) {
  std::string base_name =
      args.filename.substr(0, args.filename.find_last_of("."));
  std::string ext = args.filename.substr(args.filename.find_last_of(".") + 1);
  fs::path test_dir = "tests" / fs::path(base_name);

  if (!fs::exists(test_dir)) {
    std::cerr << "Test directory not found: " << test_dir << std::endl;
    return;
  }

  std::string exec_cmd;
  if (ext == "cpp")
    exec_cmd = "./" + base_name;
  else if (ext == "py")
    exec_cmd = "python3 " + args.filename;
  else if (ext == "java")
    exec_cmd = "java -cp . " + base_name;
  else {
    std::cerr << "Unsupported file type: " << ext << std::endl;
    return;
  }

  for (const auto &entry : fs::directory_iterator(test_dir)) {
    if (entry.path().filename().string().find("input") != std::string::npos) {
      std::string input_file = entry.path().string();
      std::string output_file =
          std::regex_replace(input_file, std::regex("input"), "output");

      std::ifstream in(input_file);
      std::ifstream out(output_file);
      if (!in || !out)
        continue;

      std::string input((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());
      std::string expected((std::istreambuf_iterator<char>(out)),
                           std::istreambuf_iterator<char>());

      auto start = high_resolution_clock::now();
      std::string actual = execute_program(exec_cmd, input);
      auto duration =
          duration_cast<milliseconds>(high_resolution_clock::now() - start);

      // Remove trailing newlines for comparison
      actual.erase(std::remove(actual.begin(), actual.end(), '\n'),
                   actual.end());
      expected.erase(std::remove(expected.begin(), expected.end(), '\n'),
                     expected.end());

      std::cout << "Test Case: " << entry.path().filename() << std::endl;
      std::cout << "Status: " << (actual == expected ? "PASS" : "FAIL")
                << std::endl;
      std::cout << "Time: " << duration.count() << "ms\n";

      if (actual != expected) {
        std::cout << "Expected:\n" << expected << "\n";
        std::cout << "Actual:\n" << actual << "\n";
      }
      std::cout << std::string(40, '-') << std::endl;
    }
  }
}

CommandArgs parse_arguments(int argc, char *argv[]) {
  CommandArgs args;
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <command> [options]" << std::endl;
    exit(1);
  }

  args.command = argv[1];
  if (args.command == "new" && argc >= 3) {
    args.filename = argv[2];
    if (argc >= 4)
      args.language = argv[3];
  } else if ((args.command == "compile" || args.command == "test") &&
             argc >= 3) {
    args.filename = argv[2];
  } else {
    std::cerr << "Invalid command or arguments" << std::endl;
    exit(1);
  }
  return args;
}

int main(int argc, char *argv[]) {
  CommandArgs args = parse_arguments(argc, argv);

  if (args.command == "new") {
    handle_new(args);
  } else if (args.command == "compile") {
    handle_compile(args);
  } else if (args.command == "test") {
    handle_test(args);
  } else {
    std::cerr << "Unknown command: " << args.command << std::endl;
    return 1;
  }

  return 0;
}
