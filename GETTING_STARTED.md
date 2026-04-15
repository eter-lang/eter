# Getting Started with Eter

## Overview

Eter is an LLVM/MLIR-oriented language project. This guide explains how to get a
fresh checkout configured, built, tested, and ready for local development.

## Requirements

### Software

Before building Eter, make sure the following tools are available:

- **CMake 3.20+**
- **Ninja** as the recommended generator
- **A modern C++-17-capable toolchain** such as AppleClang, Clang, or GCC
- **LLVM/MLIR 22.x**
- **`lit`** and **`FileCheck`** for the regression test suite

Eter is configured to require **C++-17** by default.

### Installing LLVM, MLIR, `lit`, and `FileCheck`

Eter currently expects **LLVM/MLIR 22.x**. The setup differs slightly by
platform.

#### macOS

On **macOS** with Homebrew, a practical setup is:

```bash
brew install llvm
python3 -m pip install --user lit
```

Notes:

- the Homebrew `llvm` package provides **LLVM**, **MLIR**, and tools such as
  **`FileCheck`**
- `lit` is often easiest to install with `pip`
- on Apple Silicon, the LLVM tools are typically under
  `/opt/homebrew/opt/llvm/bin`

If needed, make the LLVM tools visible in your shell:

```bash
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
```

#### Linux

On **Ubuntu/Debian**, a practical setup for **LLVM/MLIR 22.x** is to use the
`apt.llvm.org` repository and then install the required packages:

```bash
sudo apt update
sudo apt install -y wget lsb-release software-properties-common gnupg python3-pip
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 22
sudo apt install -y clang-22 lld-22 llvm-22-dev libmlir-22-dev mlir-22-tools
python3 -m pip install --user lit
```
Notes:

- `mlir-22-tools` provides utilities such as **`FileCheck`**
- on many systems, the LLVM tools are under `/usr/lib/llvm-22/bin`
- on non-Debian distributions, install the equivalent LLVM, MLIR, and
  **`FileCheck`** packages from your package manager or use an official LLVM
  binary release

If needed, make the LLVM tools visible in your shell:

```bash
export PATH="/usr/lib/llvm-22/bin:$PATH"
```

On **ArchLinux** all packages can be easily installed using **pacman** package manager except for **mlir** package which must be installed via yay. \
(If some packages are not found update the mirrors using ```sudo pacman -Syyu```)

```bash
sudo pacman -S ninja
sudo pacman -S cmake
sudo pacman -S llvm clang
yay -S --needed mlir
sudo pacman -S python-pip
python3 -m pip install --user lit

```
You can verify the install with:

```bash
clang --version
FileCheck --version
lit --version
```

If your Linux distribution installs versioned binaries only, use the versioned
commands directly, such as `clang-22 --version`.

If CMake does not automatically find LLVM and MLIR, pass their config paths
explicitly.

For **macOS** with Homebrew:

```bash
cmake -S . -B build -G Ninja \
  -DLLVM_DIR=/opt/homebrew/opt/llvm/lib/cmake/llvm \
  -DMLIR_DIR=/opt/homebrew/opt/llvm/lib/cmake/mlir
```

For **Linux** with `apt.llvm.org` packages:

```bash
cmake -S . -B build -G Ninja \
  -DLLVM_DIR=/usr/lib/llvm-22/lib/cmake/llvm \
  -DMLIR_DIR=/usr/lib/llvm-22/lib/cmake/mlir
```

### Source Checkout

If you do not already have a local checkout, clone the repository and enter the
source tree:

```bash
git clone https://github.com/eter-lang/eter
cd eter
```

## Getting the Source Code and Building Eter

Eter uses an out-of-tree CMake build. A typical development build looks like
this:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

This configures the project in `build/` and then builds the default targets.

To enable runtime sanitizer checks for supported Clang/GCC builds, add the
sanitizer options at configure time:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DETER_ENABLE_SANITIZERS=ON
```

By default, Eter enables `address`, `undefined`, and `leak`. To customize the
sanitizers, set `-DETER_SANITIZERS="address;undefined;leak;thread"` or use
`memory` for MemorySanitizer.

### Pointing CMake at LLVM and MLIR

If CMake does not discover your LLVM/MLIR installation automatically, specify
it explicitly:

```bash
cmake -S . -B build -G Ninja \
  -DLLVM_DIR=/path/to/lib/cmake/llvm \
  -DMLIR_DIR=/path/to/lib/cmake/mlir
```

Eter currently expects **LLVM/MLIR 22.x**.

## IDE and LSP Integration

For `clangd`-based IDE or LSP services, it is recommended to keep a `.clangd`
file in the repository root with the following content:

```yaml
CompileFlags:
  CompilationDatabase: build/ # Search build/ directory for compile_commands.json
```

This tells `clangd` to use the compilation database generated in `build/`.

The repository `.gitignore` already ignores `build*`, so in normal use there is
no need to add `build/` to a global Git ignore file. If you prefer a broader
machine-local rule, you can still add it to your `~/.gitignore_global`.

## Running the Test Suite

Eter uses `lit` and `FileCheck` for fast regression and smoke tests. Run them
with:

```bash
cmake --build build --target check-eter
```

## Running the Driver

To run the current driver executable directly:

```bash
./build/tools/eter/eter --version
```

This is also a simple smoke check that the build completed successfully.

## API documentation

The project supports Doxygen-based API documentation generation via the
`doc-doxygen` CMake target.

After configuring the project, run from the build directory:

```sh
cmake --build build --target doc-doxygen
```

The generated HTML output is written to:

```sh
build/doxygen/html/index.html
```

If Doxygen is not installed, CMake will configure normally and print a status
message indicating that `doc-doxygen` is disabled.

## Common Problems

### LLVM/MLIR version mismatch

If you have multiple LLVM installations, ensure that `LLVM_DIR` and `MLIR_DIR`
point to the same 22.x install.

### Missing `lit` or `FileCheck`

Make sure those tools are installed and reachable through your `PATH`, or
provided by your LLVM installation.

### Linker warnings on macOS

Depending on the local Xcode or Homebrew setup, you may see warnings about LLVM
search paths. These should be investigated if they turn into hard failures.

## Next Steps

Once Eter builds cleanly, the next natural steps are to add more regression
tests, grow the driver, and introduce the first MLIR dialects and passes.
