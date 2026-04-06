Eter Language Specification v0.1.0
==================================

## Design 

Eter is designed to bridge the gap between high-level machine learning frameworks and low-level systems programming. It is built upon three pillars:

1. Performance: Native execution via MLIR and LLVM.
2. Safety: Memory safety through Mutable Value Semantics (MVS) without a garbage collector or complex lifetime annotations.
3. Heterogeneity: First-class support for CPU, GPU, and TPU execution within a single source file.

## Syntax and Lexical Structure

- **Statement Terminator**: The semicolon (`;`) is mandatory for all expressions and declarations.
- **Comments**: Single-line (`//`) and multi-line (`/* ... */`).
- **Entry Point**: Execution begins at the `fn main()` function.
- **Attributes**: The `@` prefix is used for hardware-specific metadata and type qualifiers.

## Variable Bindings and Mutability

Eter enforces strict ownership rules to guarantee memory safety without Runtime Reference Counting (ARC).

- **let**: Defines an immutable binding. The value is "frozen" in its current scope.
- **var**: Defines a mutable binding. It acts as an explicit write-permission. A var can only be modified if the compiler can prove exclusive access (no other active reads or writes to the same memory).

**Implementation Note**: Unlike Swift, var in Eter does not imply Copy-on-Write (CoW) at runtime. If a var is shared, the compiler will either perform a static copy or emit a compile-time error, ensuring zero runtime overhead for GPU buffers.

## Memory Management: Static Ownership

Eter achieves high-performance memory management through a Static Ownership Model instead of ARC or Garbage Collection.

- **Law of Exclusivity**: A mutable value (var) can be accessed for mutation only if no other part of the program is currently accessing it. This is verified at compile-time.

- **No Implicit RC**: There are no hidden reference counters. Allocation and deallocation are inserted by the compiler at the start and end of the variable's lifetime (Deterministic Destruction).

**Hardware Mapping**:

- **let** tensor is mapped to a Read-Only descriptor on the GPU/TPU.

- **var** tensor is mapped to a Read-Write descriptor.

This allows the hardware to optimize cache usage (e.g., using the Constant Cache for let data).

## Mutable Value Semantics (MVS)

Eter treats all types as values. To achieve performance without pointers, it utilizes _Parameter Passing Modes_ to define how data interacts with functions.

Parameter Passing Modes:

| Mode | Keywords | Description |
|---|---|---|
| Read-Only | `let` | The function receives an immutable view. Optimized as a constant reference. |
| Mutation | `inout` | The function gains exclusive access to modify the caller's value in-place. |
| Consumption | `sink` | Ownership is transferred to the function. The caller's variable becomes invalid. |

## Hardware-Aware Type System

The type system is augmented with Address Space Qualifiers and Distribution Schemas to handle heterogeneous memory hierarchies.

**Address Spaces** (`@` Qualifiers):

- `@host`: Standard CPU RAM (Default).
- `@global`: Device-wide Video RAM (VRAM).
- `@shared`: High-speed, on-chip scratchpad memory (SRAM/L1).
- `@constant`: Read-only device memory optimized for broadcasting.

Distributed Computing (Sharding):

The `@sharded` attribute enables SPMD (Single Program, Multiple Data) execution across clusters.

- Syntax: `Tensor[...] @sharded(mesh, dim)`   
- Effect: Automatically generates collective communication primitives (AllReduce, AllGather) via the _GSPMD_ (Generalized Single Program, Multiple Data) compiler pass.

## Native Inference Integration

Eter treats pre-trained models as first-class functions.

- `@import_model`: Directives used to ingest external model formats (ONNX, StableHLO).
- **Zero-Overhead Call**: Imported models are compiled Ahead-of-Time (AOT) into the Eter binary, eliminating the need for a Python runtime or heavy C++ inference engines.

## Examples

### Single-Source GPU Kernels

This example demonstrates the `@gpu` attribute and the use of `@shared` memory for optimized tiled matrix operations.

```rust
@gpu
fn tiled_matrix_mul(let A: [f32] @global, let B: [f32] @global, inout C: [f32] @global) {
    // Allocation of fast on-chip scratchpad memory
    var tile_A: [f32, 256] @shared;
    var tile_B: [f32, 256] @shared;

    let tx: u32 = get_local_id(0);
    let ty: u32 = get_local_id(1);

    // Synchronous data movement from VRAM to SRAM
    tile_A[tx] = A[get_global_id(0)];
    tile_B[ty] = B[get_global_id(1)];
    
    barrier(); // Synchronize threads within the block

    // Perform computation on @shared memory
    C[get_global_id()] = tile_A[tx] * tile_B[ty];
}
```

### Distributed Training (Automatic Sharding)

This demonstrates how Eter hides the complexity of Collective Communications (AllReduce/AllGather) behind type annotations.

```rust
let GPU_COUNT: u32 = 8;
let mesh: Mesh = Mesh(devices=GPU_COUNT, topology="1d");

fn train_step(inout weights: Tensor @sharded(mesh, dim=0), let grad: Tensor @global) {
    // 'weights' is physically split across 8 GPUs.
    // 'grad' might be local or replicated.
    
    // The compiler detects the sharding mismatch and inserts 
    // the necessary communication primitives automatically.
    weights -= grad * 0.001; 
    
    print("Weights updated and synchronized;");
}
```

### Embedded Inference (Zero-Retrain Integration)

```rust
// Linking an external compute graph as a typed function
@import_model("resnet50.onnx", format="onnx")
fn resnet_forward(let input: Tensor @global) -> Tensor @global;

fn main() {
    // Load data directly to GPU memory
    var image_tensor: Tensor = load_image("image.jpg") @global;
    
    // Inference is treated as a standard function call.
    // No Python interpreter or external runtime management required.
    let prediction: Tensor = resnet_forward(image_tensor);
    
    let top_class: u32 = prediction.argmax();
    print("Class detected: {top_class};");
}
```

### Error Handling and Resource Management

Eter uses a Result type for safety, ensuring that hardware failures (like GPU out-of-memory) are handled explicitly without exceptions.

```rust
fn initialize_gpu_buffer(size: u64) -> Result<Tensor @global, GPUError> {
    if (size > get_available_vram()) {
        return Error(GPUError.OutOfMemory);
    };
    
    return Ok(Tensor.uninitialized(size) @global);
}

fn main() {
    // Using the '?' operator to propagate errors
    let buffer: Tensor @global = initialize_gpu_buffer(1024 * 1024)?; 
    
    print("Buffer allocated successfully;");
}
```



### A Complete Example: Multi-GPU Transformer Inference

```rust
let NUM_GPUS: u32 = 8;
let cluster: Mesh = Mesh(devices=NUM_GPUS, topology="1d");

// Import a pre-trained Transformer block
@import_model("llama_block.stablehlo")
fn transformer_layer(inout x: Tensor @sharded(cluster, 0));

@gpu
fn preprocess_kernel(inout data: [f32] @global) {
    let id: u32 = get_global_id();
    data[id] = tanh(data[id]);
}

fn main() {
    // Allocation on GPU with Sharding
    var states: Tensor @global @sharded(cluster, 0) = Tensor.zeros([4096, 1024]);
    
    // Explicit mutation call
    transformer_layer(inout states);
    
    print("Inference step completed across cluster;");
}
```

---

##### Posteponed Features

- **Generics and Templates**: Planned for v0.2.0 to enable code reuse and abstraction.
- **Type Inference**: Basic type inference is available, but full support is planned for v0.2.0.
- **Standard Library**: A minimal set of utilities for I/O, math, and data structures is planned for v0.2.0.
- **Metaprogramming**: The `#` prefix is reserved for compile-time directives and macros.
- **#let**: A compile-time constant. The value must be resolvable during the compilation phase.
- **Automated Barrier Inference**

    In version 0.1, the barrier() statement is mandatory for synchronizing threads accessing @shared memory. For version 0.2, Eter aims to introduce Static Data-Flow Analysis to infer synchronization points.

    - Objective: Reduce boilerplate and prevent common deadlocks or race conditions in GPU kernels.
    - Mechanism: The compiler will trace the "Write-after-Read" (WaR) and "Read-after-Write" (RaW) patterns on variables qualified with @shared.
    - Opt-in Attribute: This behavior will be controlled via a function-level attribute to maintain backward compatibility and expert control.

    ```rust
    // Proposed v0.2 Syntax
    @gpu
    @infer_barriers // The compiler automatically inserts barrier() where needed
    fn auto_synced_kernel(let input: [f32] @global, inout cache: [f32] @shared) {
        let id: u32 = get_local_id();
        cache[id] = input[id]; 
        
        // NO MANUAL BARRIER REQUIRED HERE
        // The compiler detects that 'cache' is read in the next line 
        // and was written by all threads, so it inserts a physical fence.
        
        let val: f32 = cache[id ^ 1] * 2.0; 
    }
    ```