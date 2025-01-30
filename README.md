# Competitive Programming Helper CLI

## Overview

`cph` is a command-line tool designed to streamline competitive programming workflows by managing templates, running test cases, and submitting solutions efficiently.

## Features

- **Template Management**: Quickly generate new problem templates.
- **Testing Framework**: Run and verify test cases automatically.

## Installation

### Prerequisites

- macOS or Linux
- C++ compiler (GCC/Clang)
- Make

### Build from Source

```sh
# Clone the repository
git clone https://github.com/yourrepo/cph.git
cd cph

# Build with Make
make
```

### Install

```sh
sudo make install
```

This places `cph` in `/usr/local/bin`, allowing you to run it from anywhere.

### Uninstall

```sh
sudo make uninstall
```

## Usage

### Creating a New Problem

```sh
cph new problem_name.cpp
```

This creates a new folder with a default template.

### Running Test Cases

```sh
cph test
```

Reads input from `input.txt` and compares output with `output.txt`.

## Contribution

- Fork the repository.
- Create a feature branch.
- Submit a pull request.
