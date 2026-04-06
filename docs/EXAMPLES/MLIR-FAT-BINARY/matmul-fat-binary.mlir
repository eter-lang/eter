//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// =============================================================================
// MLIR Fat Binary Example — matmul 16×16
//
// A single GPU kernel compiled for MULTIPLE targets (NVIDIA + AMD) plus
// a CPU fallback.  The fat-binary mechanism in MLIR works by:
//   1. Writing ONE kernel using the generic `gpu` dialect.
//   2. Attaching multiple target attributes (nvvm, rocdl) to the gpu.module.
//   3. Using `--gpu-module-to-binary` to compile the SAME kernel for ALL
//      attached targets, embedding every ISA into a single module.
//   4. At runtime the GPU runtime picks the correct binary blob.
// =============================================================================

module attributes {gpu.container_module} {

  // ---------------------------------------------------------------------------
  // GPU kernel — target-agnostic.
  // The SAME function will be compiled to PTX (NVIDIA) AND HSACO (AMD).
  // ---------------------------------------------------------------------------
  gpu.module @kernels {
    gpu.func @matmul_kernel(
        %A: memref<16x16xf32>,
        %B: memref<16x16xf32>,
        %C: memref<16x16xf32>) kernel {
      %c0  = arith.constant 0 : index
      %c1  = arith.constant 1 : index
      %c16 = arith.constant 16 : index
      %zero = arith.constant 0.0 : f32

      %tx  = gpu.thread_id x
      %ty  = gpu.thread_id y
      %bx  = gpu.block_id x
      %by  = gpu.block_id y
      %bdx = gpu.block_dim x
      %bdy = gpu.block_dim y

      %rowBase = arith.muli %by, %bdy : index
      %colBase = arith.muli %bx, %bdx : index
      %row = arith.addi %rowBase, %ty : index
      %col = arith.addi %colBase, %tx : index

      %sum = scf.for %k = %c0 to %c16 step %c1
                 iter_args(%acc = %zero) -> (f32) {
        %a = memref.load %A[%row, %k] : memref<16x16xf32>
        %b = memref.load %B[%k, %col] : memref<16x16xf32>
        %prod   = arith.mulf %a, %b : f32
        %newAcc = arith.addf %acc, %prod : f32
        scf.yield %newAcc : f32
      }
      memref.store %sum, %C[%row, %col] : memref<16x16xf32>
      gpu.return
    }
  }

  // ---------------------------------------------------------------------------
  // CPU fallback — pure scalar triple-loop matmul.
  // ---------------------------------------------------------------------------
  func.func @matmul_cpu(
      %A: memref<16x16xf32>,
      %B: memref<16x16xf32>,
      %C: memref<16x16xf32>) {
    %c0  = arith.constant 0 : index
    %c1  = arith.constant 1 : index
    %c16 = arith.constant 16 : index
    scf.for %i = %c0 to %c16 step %c1 {
      scf.for %j = %c0 to %c16 step %c1 {
        scf.for %k = %c0 to %c16 step %c1 {
          %a   = memref.load %A[%i, %k] : memref<16x16xf32>
          %b   = memref.load %B[%k, %j] : memref<16x16xf32>
          %c   = memref.load %C[%i, %j] : memref<16x16xf32>
          %mul = arith.mulf %a, %b : f32
          %sum = arith.addf %c, %mul : f32
          memref.store %sum, %C[%i, %j] : memref<16x16xf32>
        }
      }
    }
    return
  }

  // ---------------------------------------------------------------------------
  // Host dispatcher — launches the GPU kernel or falls back to CPU.
  // In a real fat binary the GPU runtime resolves the correct ISA blob
  // (PTX vs HSACO) automatically from the embedded binary objects.
  // ---------------------------------------------------------------------------
  func.func @matmul_dispatch(
      %A: memref<16x16xf32>,
      %B: memref<16x16xf32>,
      %C: memref<16x16xf32>) {
    %use_gpu = arith.constant 1 : i1
    scf.if %use_gpu {
      %c1  = arith.constant 1 : index
      %c16 = arith.constant 16 : index
      gpu.launch_func @kernels::@matmul_kernel
          blocks in (%c1, %c1, %c1)
          threads in (%c16, %c16, %c1)
          args(%A : memref<16x16xf32>,
               %B : memref<16x16xf32>,
               %C : memref<16x16xf32>)
    } else {
      func.call @matmul_cpu(%A, %B, %C)
          : (memref<16x16xf32>, memref<16x16xf32>, memref<16x16xf32>) -> ()
    }
    return
  }
}

