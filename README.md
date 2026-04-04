<div align="center">

The Quar Programming Language
=============================

**Quar** [/kwɔːr/] — *Derived from Quasar: an extremely luminous active galactic nucleus*.

[![Apple Silicon](https://github.com/quar-lang/quar/actions/workflows/apple-silicon.yml/badge.svg)](https://github.com/quar-lang/quar/actions/workflows/apple-silicon.yml)
[![Apple x86](https://github.com/quar-lang/quar/actions/workflows/apple-x86.yml/badge.svg)](https://github.com/quar-lang/quar/actions/workflows/apple-x86.yml)
[![Ubuntu x86](https://github.com/quar-lang/quar/actions/workflows/ubuntu-x86.yml/badge.svg)](https://github.com/quar-lang/quar/actions/workflows/ubuntu-x86.yml)
[![Ubuntu AArch64](https://github.com/quar-lang/quar/actions/workflows/ubuntu-aarch64.yml/badge.svg)](https://github.com/quar-lang/quar/actions/workflows/ubuntu-aarch64.yml)
[![Fedora x86](https://github.com/quar-lang/quar/actions/workflows/fedora-x86.yml/badge.svg)](https://github.com/quar-lang/quar/actions/workflows/fedora-x86.yml)
[![Debian x86](https://github.com/quar-lang/quar/actions/workflows/debian-x86.yml/badge.svg)](https://github.com/quar-lang/quar/actions/workflows/debian-x86.yml)
[![Arch Linux x86](https://github.com/quar-lang/quar/actions/workflows/arch-x86.yml/badge.svg)](https://github.com/quar-lang/quar/actions/workflows/arch-x86.yml)
[![openSUSE x86](https://github.com/quar-lang/quar/actions/workflows/opensuse-x86.yml/badge.svg)](https://github.com/quar-lang/quar/actions/workflows/opensuse-x86.yml)

[Why Quar?](#why-quar) • [Getting the Source Code and Building Quar](#getting-the-source-code-and-building-quar) • [Project Layout](#project-layout) • [License](#license)

</div>

---

## Why Quar?

**Heterogeneous computing** is increasingly important in modern software development, but it can be difficult to write performant code that runs across a variety of hardware targets.
Existing approaches tries to solve this problem with libraries, compiler extensions, runtime systems, domain-specific languages, and system programming languages, but these solutions often have limitations in some technical or practical dimension.
**Quar** is a new programming language designed to address these challenges by providing a high-level, expressive syntax for writing code that can be efficiently compiled to a wide range of hardware targets.
Quar is built on top of the LLVM and MLIR compiler infrastructure, which allows it to leverage the powerful optimization and code generation capabilities of these frameworks.

For additional information, the [Motivation](docs/MOTIVATION.md) document provides a detailed overview of the technical and practical challenges in heterogeneous computing.

## Getting the Source Code and Building Quar

Consult the [Getting Started with Quar](docs/GETTING_STARTED.md) page for information on building and running Quar.
Quar currently expects **LLVM/MLIR 22.x** and a **C++23-capable** compiler toolchain.

For information on how to contribute to the Quar project, please take a look at the [Contributing to Quar](docs/CONTRIBUTING.md) guide.

Contributors may also want to enable the repository Git hooks described in [Contributing to Quar](docs/CONTRIBUTING.md) for local formatting, linting, and pre-push validation.

For IDE/LSP setup tips, including the recommended root `.clangd` file, see [Getting Started with Quar](docs/GETTING_STARTED.md).

## Project Layout

- `include/quar/` — public headers
- `lib/` — core library implementation
- `tools/` — executables and developer tools
- `test/` — `lit` / `FileCheck` regression tests, including `Smoke/`
- `unittests/` — unit-test targets
- `docs/` — guides such as `GETTING_STARTED.md`, `CONTRIBUTING.md`, and `CI` documentation
- `.githooks/` — pre-commit and pre-push Git hooks for formatting, linting, and validation
- `.github/workflows/` — CI workflow definitions for GitHub Actions

## License

Quar is licensed under the Apache License v2.0 with LLVM Exceptions. See the [LICENSE](LICENSE) file for more information.