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

> [!WARNING]
> Eter is currently in the early stages of development. The language design and implementation are subject to significant changes, and the implementation is not started yet.

While [The Eter Reference](https://eter-lang.github.io/eter/) serves as the primary source of language specification ([source code](./docs)), including its syntax and semantics, the [API Reference](https://eter-lang.github.io/eter/api/) provides the doxygen-generated documentation for the Eter compiler's C++ API, which is intended for contributors and advanced users interested in extending or interfacing with the compiler.

```rust
// ML models as first-class citizens. The @model attribute orchestrates the native 
// loading and linking of pre-trained assets directly into the program's binary.
// The function signature acts as a static hardware contract: the compiler enforces 
// tensor shape integrity and memory residency (e.g., @host, @gpu), ensuring 
// zero-overhead data transitions and eliminating runtime shape mismatches.
@model<TF /* TensorFlow */, version = V1>("mobilenet_v2")
extern fn infer(x: tsor[f32; [1, 224, 224, 3]] @host) -> tsor[f32; [1, 1000]] @host;

fn main() {
    let input tsor[f32; [1, 224, 224, 3]] @host = tsor::from_image("dog.jpg");
    let output tsor[f32; [1, 1000]] = infer(input);
    print("Inference completed. Output shape: ", output.shape());
}
```

<!--
```rust
// 1. Hardware Infrastructure Definition
// Define a logical grid of 8 GPUs. The compiler uses this 'Mesh' 
// to automatically handle data distribution and collective communications.
let cluster = Mesh(devices=8, topology="1d");



// 3. Custom Tiling Kernel (TileIR-inspired)
// We eliminate manual thread management (no threadIdx). 
// The language operates on "Tiles" (sub-matrices).
@gpu_tile(128, 128)
fn custom_preprocess_kernel(mut data: tsor<f32; [4096, 1024]> @global) {
    // The compiler generates an optimized memory pipeline:
    // Global Memory (VRAM) -> Shared Memory (SRAM) -> Registers -> Compute.
    for tile in data.tiles() {
        // Explicitly load a region into fast on-chip Shared Memory.
        // This is a zero-copy operation if the hardware supports asynchronous DMA.
        let mut sram_tile: Tile<f32, [128, 128]> @shared = tile.load();
        
        // SIMD mapping: apply tanh to every element in the tile simultaneously.
        sram_tile = tanh(sram_tile);
        
        // Write the processed tile back to Global Memory.
        tile.store(sram_tile);
    }
}

// 4. Main Program Orchestration
fn main() {
    // Sharded Allocation: The system partitions 4096 rows across 8 GPUs.
    // Each physical device allocates only 512 rows in its local VRAM.
    let mut states tsor[f32; [4096, 1024]] @sharded(cluster, 0) = tsor[0.0; 4096, 1024];

    // Launch the tiling kernel. Because 'states' is sharded, 
    // this executes in parallel across all 8 GPUs in the cluster.
    custom_preprocess_kernel(mut states);

    // Call the External Model (Inference Step).
    // The runtime passes the sharded memory pointers to the StableHLO executor.
    transformer_layer(mut states);

    // Implicit Barrier: The program waits for the cluster to synchronize 
    // before executing host-side logic.
    print("Inference pipeline completed across 8 GPUs.");
}
```
-->

## Why Eter?

Modern software development increasingly relies on **heterogeneous computing**, yet writing performant code across diverse hardware remains a significant challenge. 
Existing solutions—ranging from libraries and compiler extensions to domain-specific and system programming languages—often face technical limitations or practical trade-offs.
Currently, **machine learning** (ML) **models** are compiled via specialized tools like XLA, Glow, or TVM, making their integration into general-purpose languages such as Python, C++, or Rust difficult and often requiring wrappers that introduce overhead and complexity. 
Furthermore, achieving high performance across different architectures such as **GPUs** and **specialized accelerators** often demands a deep understanding of hardware-specific models, which compromises both efficiency and portability.

**Eter** is a new programming language designed to bridge these gaps. It provides a high-level, expressive syntax that compiles efficiently to a wide range of targets, including CPUs, GPUs, and specialized accelerators. 
Eter empowers developers to write native GPU kernels and manage distributed resources—such as device meshes and sharded tensors—directly within the language. 
In Eter, machine learning models are first-class citizens, making inference on a pre-trained model as seamless as a standard function call. 
Built on the LLVM and MLIR infrastructure, Eter leverages industry-leading optimization and code generation capabilities to deliver native performance with high-level elegance.

## Getting the Source Code and Building Eter

Consult the [Getting Started with Eter](./GETTING_STARTED.md) page for information on building and running Eter.
Eter currently expects **LLVM/MLIR 22.x** and a **C++17-capable** compiler toolchain.


For information on how to contribute to the Eter project, please take a look at the [Contributing to Eter](./CONTRIBUTING.md) guide.

Contributors may also want to enable the repository Git hooks described in [Contributing to Eter](./CONTRIBUTING.md) for local formatting, linting, and pre-push validation.

For IDE/LSP setup tips, including the recommended root `.clangd` file, see [Getting Started with Eter](./GETTING_STARTED.md).

## License

Eter is licensed under the Apache License v2.0 with LLVM Exceptions. See the [LICENSE](LICENSE) file for more information.

