#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
using namespace std::chrono;

std::string template_dir() {
  const char *home = std::getenv("HOME");
  if (home == nullptr || *home == '\0') {
    return "./.cp_helper/templates/";
  }
  return std::string(home) + "/.cp_helper/templates/";
}

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

input = sys.stdin.readline

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

std::string default_template(const std::string &ext) {
  if (ext == "cpp") return get_cpp_template();
  if (ext == "py") return get_py_template();
  if (ext == "java") return get_java_template();
  return "";
}

std::optional<std::string> load_custom_template(const std::string &ext) {
  fs::path path = fs::path(template_dir()) / ("template." + ext);
  std::error_code ec;
  if (!fs::exists(path, ec)) return std::nullopt;
  std::ifstream f(path);
  if (!f) return std::nullopt;
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

std::string extension_of(const std::string &filename) {
  auto pos = filename.find_last_of('.');
  if (pos == std::string::npos || pos == filename.size() - 1) return "";
  return filename.substr(pos + 1);
}

std::string stem_of(const std::string &filename) {
  auto pos = filename.find_last_of('.');
  if (pos == std::string::npos) return filename;
  return filename.substr(0, pos);
}

std::string resolve_extension(const CommandArgs &args) {
  if (!args.language.empty()) return args.language;
  return extension_of(args.filename);
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
  std::string ext = resolve_extension(args);
  if (ext.empty()) {
    std::cerr << "Could not determine file extension for: " << args.filename
              << " (pass -l <cpp|py|java>)" << std::endl;
    return;
  }

  std::string content;
  if (auto custom = load_custom_template(ext)) {
    content = *custom;
  } else {
    content = default_template(ext);
  }
  if (content.empty()) {
    std::cerr << "Unsupported file type: " << ext << std::endl;
    return;
  }

  if (fs::exists(args.filename)) {
    std::cerr << "Refusing to overwrite existing file: " << args.filename
              << std::endl;
    return;
  }
  create_file(args.filename, content);

  std::string base = stem_of(fs::path(args.filename).filename().string());
  fs::path test_dir = fs::path("tests") / base;
  std::error_code ec;
  fs::create_directories(test_dir, ec);
  if (ec) {
    std::cerr << "Warning: could not create test directory " << test_dir
              << ": " << ec.message() << std::endl;
    return;
  }
  std::ofstream(test_dir / "input_1.txt");
  std::ofstream(test_dir / "output_1.txt");
  std::cout << "Created test directory: " << test_dir << std::endl;
}

bool compile_source(const CommandArgs &args, bool announce) {
  std::string ext = resolve_extension(args);
  std::string stem = stem_of(args.filename);
  std::string cmd;

  if (ext == "cpp") {
    cmd = "g++-14 -O2 -Wall " + args.filename + " -o " + stem;
  } else if (ext == "java") {
    cmd = "javac " + args.filename;
  } else {
    if (announce) std::cout << "No compilation needed" << std::endl;
    return true;
  }

  int result = std::system(cmd.c_str());
  if (result != 0) {
    std::cerr << "Compilation failed!" << std::endl;
    return false;
  }
  if (announce) std::cout << "Compilation successful" << std::endl;
  return true;
}

void handle_compile(const CommandArgs &args) { compile_source(args, true); }

std::string execute_program(const std::string &cmd, const std::string &input) {
  char tmpl[] = "/tmp/cph_input_XXXXXX";
  int fd = mkstemp(tmpl);
  if (fd == -1) return "ERROR";
  if (!input.empty()) {
    ssize_t written = write(fd, input.data(), input.size());
    if (written < 0 || static_cast<size_t>(written) != input.size()) {
      close(fd);
      unlink(tmpl);
      return "ERROR";
    }
  }
  close(fd);

  std::string full_cmd = cmd + " < " + tmpl + " 2>&1";
  FILE *pipe = popen(full_cmd.c_str(), "r");
  if (!pipe) {
    unlink(tmpl);
    return "ERROR";
  }

  char buffer[4096];
  std::string result;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    result += buffer;
  }
  pclose(pipe);
  unlink(tmpl);
  return result;
}

std::string normalize_output(const std::string &s) {
  std::vector<std::string> lines;
  std::istringstream iss(s);
  std::string line;
  while (std::getline(iss, line)) {
    while (!line.empty() &&
           std::isspace(static_cast<unsigned char>(line.back()))) {
      line.pop_back();
    }
    lines.push_back(line);
  }
  while (!lines.empty() && lines.back().empty()) lines.pop_back();

  std::string out;
  for (size_t i = 0; i < lines.size(); ++i) {
    if (i > 0) out += '\n';
    out += lines[i];
  }
  return out;
}

std::string build_exec_command(const CommandArgs &args) {
  std::string ext = resolve_extension(args);
  std::string base_name = stem_of(args.filename);
  if (ext == "cpp") return "./" + base_name;
  if (ext == "py") return "python3 " + args.filename;
  if (ext == "java")
    return "java -cp . " + fs::path(base_name).filename().string();
  return "";
}

void handle_run(const CommandArgs &args) {
  std::string ext = resolve_extension(args);
  std::string exec_cmd = build_exec_command(args);
  if (exec_cmd.empty()) {
    std::cerr << "Unsupported file type: " << ext << std::endl;
    return;
  }

  if (!compile_source(args, false)) return;

  int result = std::system(exec_cmd.c_str());
  if (result != 0) {
    std::cerr << "Program exited with non-zero status: " << result << std::endl;
  }

  std::string stem = stem_of(args.filename);
  std::error_code ec;
  if (ext == "cpp") {
    fs::remove(stem, ec);
  } else if (ext == "java") {
    fs::remove(stem + ".class", ec);
  }
}

void handle_test(const CommandArgs &args) {
  std::string base_name = stem_of(args.filename);
  fs::path test_dir = fs::path("tests") / fs::path(base_name).filename();

  if (!fs::exists(test_dir)) {
    std::cerr << "Test directory not found: " << test_dir << std::endl;
    return;
  }

  std::string exec_cmd = build_exec_command(args);
  if (exec_cmd.empty()) {
    std::cerr << "Unsupported file type: " << resolve_extension(args)
              << std::endl;
    return;
  }

  std::vector<fs::path> inputs;
  for (const auto &entry : fs::directory_iterator(test_dir)) {
    if (entry.path().filename().string().find("input") != std::string::npos) {
      inputs.push_back(entry.path());
    }
  }
  std::sort(inputs.begin(), inputs.end());

  if (inputs.empty()) {
    std::cerr << "No input files found in " << test_dir << std::endl;
    return;
  }

  int passed = 0, total = 0;
  for (const auto &input_path : inputs) {
    std::string input_file = input_path.string();
    std::string output_file =
        (input_path.parent_path() /
         ("output" + input_path.filename().string().substr(5)))
            .string();

    std::ifstream in(input_file);
    std::ifstream out(output_file);
    if (!in || !out) {
      std::cerr << "Skipping " << input_path.filename()
                << " (missing matching output file)" << std::endl;
      continue;
    }

    std::string input((std::istreambuf_iterator<char>(in)),
                      std::istreambuf_iterator<char>());
    std::string expected((std::istreambuf_iterator<char>(out)),
                         std::istreambuf_iterator<char>());

    auto start = high_resolution_clock::now();
    std::string actual = execute_program(exec_cmd, input);
    auto duration =
        duration_cast<milliseconds>(high_resolution_clock::now() - start);

    std::string actual_norm = normalize_output(actual);
    std::string expected_norm = normalize_output(expected);
    bool ok = actual_norm == expected_norm;

    total++;
    if (ok) passed++;

    std::cout << "Test Case: " << input_path.filename() << std::endl;
    std::cout << "Status: " << (ok ? "PASS" : "FAIL") << std::endl;
    std::cout << "Time: " << duration.count() << "ms\n";

    if (!ok) {
      std::cout << "Expected:\n" << expected_norm << "\n";
      std::cout << "Actual:\n" << actual_norm << "\n";
    }
    std::cout << std::string(40, '-') << std::endl;
  }
  std::cout << "Summary: " << passed << "/" << total << " passed" << std::endl;
}

void print_usage(const char *prog) {
  std::cerr << "Usage: " << prog << " <command> <filename> [-l <lang>]\n"
            << "Commands:\n"
            << "  new      Create a new source file and tests/<stem>/\n"
            << "  compile  Compile a source file\n"
            << "  run      Run the program interactively (stdin from terminal)\n"
            << "  test     Run tests in tests/<stem>/\n"
            << "Languages: cpp, py, java (detected from extension, or -l)\n";
}

CommandArgs parse_arguments(int argc, char *argv[]) {
  CommandArgs args;
  if (argc < 3) {
    print_usage(argv[0]);
    exit(1);
  }

  args.command = argv[1];
  args.filename = argv[2];
  for (int i = 3; i < argc; ++i) {
    std::string flag = argv[i];
    if (flag == "-l" && i + 1 < argc) {
      args.language = argv[++i];
    } else {
      std::cerr << "Unknown argument: " << flag << std::endl;
      print_usage(argv[0]);
      exit(1);
    }
  }

  if (args.command != "new" && args.command != "compile" &&
      args.command != "run" && args.command != "test") {
    std::cerr << "Unknown command: " << args.command << std::endl;
    print_usage(argv[0]);
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
  } else if (args.command == "run") {
    handle_run(args);
  } else if (args.command == "test") {
    handle_test(args);
  }

  return 0;
}
