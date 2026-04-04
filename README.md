<div align="center">

The Quar Programming Language
=============================

**Quar** [/kwɔːr/] — *Derived from Quasar: an extremely luminous active galactic nucleus*.

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

For information on how to contribute to the Quar project, please take a look at the [Contributing to Quar](docs/CONTRIBUTING.md) guide.

Contributors may also want to enable the repository Git hooks described in [Contributing to Quar](docs/CONTRIBUTING.md) for local formatting, linting, and pre-push validation.

For IDE/LSP setup tips, including the recommended root `.clangd` file, see [Getting Started with Quar](docs/GETTING_STARTED.md).

## Project Layout

- `include/quar/` — public headers
- `lib/` — core library implementation
- `tools/` — executables and developer tools
- `test/` — `lit` / `FileCheck` regression tests, including `Smoke/`
- `unittests/` — unit-test targets
- `docs/` — guides such as `GETTING_STARTED.md` and `CONTRIBUTING.md`
- `.githooks/` — pre-commit and pre-push Git hooks for formatting, linting, and validation

## License

Quar is licensed under the Apache License v2.0 with LLVM Exceptions. See the [LICENSE](LICENSE) file for more information.