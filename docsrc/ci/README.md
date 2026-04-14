# Continuous Integration

This document describes the CI pipelines used by the Eter project.

## Overview

All CI workflows live in `.github/workflows/` and run on [GitHub Actions](https://docs.github.com/en/actions).
Every workflow builds Eter in both **Debug** and **Release** configurations and
runs the `lit` test suite.

### Triggers

| Trigger | Scope |
|---|---|
| `push` | Every push to any branch |
| `pull_request` | PRs targeting `main` |
| `schedule` | First day of each month (midnight UTC) — catches breakage from external updates (runner images, rolling-release packages, action deprecations) |
| `workflow_dispatch` | Manual trigger from the Actions UI |

## Workflows

### macOS

| Workflow | File | Runner | Architecture | Matrix |
|---|---|---|---|---|
| Apple Silicon | `apple-silicon.yml` | `macos-14` | AArch64 | Debug, Release |
| Apple x86 | `apple-x86.yml` | `macos-15-intel` | x86-64 | Debug, Release |

**Dependencies** are installed via Homebrew: `llvm@22` (provides both LLVM and
MLIR), `ninja`, and `lit`.  The LLVM prefix differs per architecture:

- AArch64: `/opt/homebrew/opt/llvm@22/`
- x86-64: `/usr/local/opt/llvm@22/`

Both workflows include the "Unbreak Python in Github Actions" workaround for
conflicting Python framework installations on macOS runners.

### Ubuntu (native)

| Workflow | File | Runner | Architecture | Matrix |
|---|---|---|---|---|
| Ubuntu x86 | `ubuntu-x86.yml` | `ubuntu-24.04` | x86-64 | (GCC, Clang) × (Debug, Release) |
| Ubuntu AArch64 | `ubuntu-aarch64.yml` | `ubuntu-24.04-arm` | AArch64 | (GCC, Clang) × (Debug, Release) |

LLVM 22 and MLIR packages are installed from the official
[apt.llvm.org](https://apt.llvm.org) repository.  `lit` is installed via
`pip3`.

### Container-based distributions

The following workflows run inside Docker containers on an `ubuntu-24.04` host
runner.  This lets us test against distribution-native toolchains without
dedicated runner images.

| Workflow | File | Container | Matrix |
|---|---|---|---|
| Fedora x86 | `fedora-x86.yml` | `fedora:42` | (GCC, Clang) × (Debug, Release) |
| Debian x86 | `debian-x86.yml` | `debian:trixie` | (GCC, Clang) × (Debug, Release) |
| Arch Linux x86 | `arch-x86.yml` | `archlinux:latest` | (GCC, Clang) × (Debug, Release) |
| openSUSE x86 | `opensuse-x86.yml` | `opensuse/tumbleweed:latest` | (GCC, Clang) × (Debug, Release) |

Package managers and LLVM installation:

- **Debian** — `apt` with the `apt.llvm.org` LLVM 22 repo (same as Ubuntu):
  `libmlir-22-dev`, `mlir-22-tools`
- **Fedora** — `dnf` for base deps; LLVM 22 from official
  [GitHub release tarball](https://github.com/llvm/llvm-project/releases)
- **Arch Linux** — `pacman` for base deps; LLVM 22 from official release tarball
- **openSUSE Tumbleweed** — `zypper` for base deps; LLVM 22 from official
  release tarball

Fedora, Arch Linux, and openSUSE do not yet ship LLVM 22 in their
repositories.  These workflows download the pre-built
`LLVM-<version>-Linux-X64.tar.xz` tarball from the LLVM project's GitHub
releases, extract it to `/opt/llvm-22/`, and point `CMAKE_PREFIX_PATH` at it.
The `LLVM_VERSION` environment variable at the top of each workflow controls
which release is fetched.

## Build process

All workflows follow the same steps:

```bash
mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH="<llvm-prefix>" \
      -DCMAKE_BUILD_TYPE=<Debug|Release> \
      -G Ninja ../
ninja -j2
```

`CMAKE_PREFIX_PATH` is set so that CMake's `find_package(LLVM CONFIG)` and
`find_package(MLIR CONFIG)` locate the correct installation.

## Running tests

```bash
lit -va ./test/
```

This executes the `lit` / `FileCheck` regression test suite located under
`test/`.

## Adding a new workflow

1. Create a new `.yml` file in `.github/workflows/`.
2. Follow the same trigger block (`push`, `pull_request`, `schedule`,
   `workflow_dispatch`).
3. Install LLVM 22 + MLIR either from the distro's package manager (if
   available) or by downloading the official release tarball.
4. Install `ninja` and `lit`.
   package manager.
4. Use the same CMake / Ninja / lit invocation pattern.
5. Add the corresponding badge to `README.md`.
6. Update this document.
