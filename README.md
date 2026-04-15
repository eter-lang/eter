<div align="center">

<img src="docsrc/logo/logo.png" alt="Eter logo" width="128" />

The Eter Programming Language
=============================

**Eter** — *A wordplay on Heterogeneous and Ether*.

<!-- [![Apple Silicon](https://github.com/quar-lang/quar/actions/workflows/apple-silicon.yml/badge.svg)](https://github.com/quar-lang/quar/actions/workflows/apple-silicon.yml)
[![Apple x86](https://github.com/quar-lang/quar/actions/workflows/apple-x86.yml/badge.svg)](https://github.com/quar-lang/quar/actions/workflows/apple-x86.yml)
[![Ubuntu x86](https://github.com/quar-lang/quar/actions/workflows/ubuntu-x86.yml/badge.svg)](https://github.com/quar-lang/quar/actions/workflows/ubuntu-x86.yml)
[![Ubuntu AArch64](https://github.com/quar-lang/quar/actions/workflows/ubuntu-aarch64.yml/badge.svg)](https://github.com/quar-lang/quar/actions/workflows/ubuntu-aarch64.yml)
[![Fedora x86](https://github.com/quar-lang/quar/actions/workflows/fedora-x86.yml/badge.svg)](https://github.com/quar-lang/quar/actions/workflows/fedora-x86.yml)
[![Debian x86](https://github.com/quar-lang/quar/actions/workflows/debian-x86.yml/badge.svg)](https://github.com/quar-lang/quar/actions/workflows/debian-x86.yml)
[![Arch Linux x86](https://github.com/quar-lang/quar/actions/workflows/arch-x86.yml/badge.svg)](https://github.com/quar-lang/quar/actions/workflows/arch-x86.yml)
[![openSUSE x86](https://github.com/quar-lang/quar/actions/workflows/opensuse-x86.yml/badge.svg)](https://github.com/quar-lang/quar/actions/workflows/opensuse-x86.yml) -->

[Why Eter?](#why-eter) • [Getting the Source Code and Building Eter](#getting-the-source-code-and-building-eter) • [License](#license)

</div>

---

## Why Eter?

Modern software development increasingly relies on **heterogeneous computing**, yet writing performant code across diverse hardware remains a significant challenge. 
Existing solutions—ranging from libraries and compiler extensions to domain-specific and system programming languages—often face technical limitations or practical trade-offs.
Currently, **machine learning** (ML) **models** are compiled via specialized tools like XLA, Glow, or TVM, making their integration into general-purpose languages such as Python, C++, or Rust difficult and often requiring wrappers that introduce overhead and complexity. 
Furthermore, achieving high performance across different architectures such as **GPUs** and **specialized accelerators** often demands a deep understanding of hardware-specific models, which compromises both efficiency and portability.

**Eter** is a new programming language designed to bridge these gaps. It provides a high-level, expressive syntax that compiles efficiently to a wide range of targets, including CPUs, GPUs, and specialized accelerators. 
Eter empowers developers to write native GPU kernels and manage distributed resources—such as device meshes and sharded tensors—directly within the language. 
In Eter, machine learning models are first-class citizens, making inference on a pre-trained model as seamless as a standard function call. 
Built on the LLVM and MLIR infrastructure, Eter leverages industry-leading optimization and code generation capabilities to deliver native performance with high-level elegance.

## Current state

A full [language specification](https://eter-lang.github.io/eter/) is currenly being developed. The source code can be found [here](./docs/)

## Getting the Source Code and Building Eter

Consult the [Getting Started with Eter](./GETTING_STARTED.md) page for information on building and running Eter.
Eter currently expects **LLVM/MLIR 22.x** and a **C++17-capable** compiler toolchain.


For information on how to contribute to the Eter project, please take a look at the [Contributing to Eter](./CONTRIBUTING.md) guide.

Contributors may also want to enable the repository Git hooks described in [Contributing to Eter](./CONTRIBUTING.md) for local formatting, linting, and pre-push validation.

For IDE/LSP setup tips, including the recommended root `.clangd` file, see [Getting Started with Eter](./GETTING_STARTED.md).

## License

Eter is licensed under the Apache License v2.0 with LLVM Exceptions. See the [LICENSE](LICENSE) file for more information.

