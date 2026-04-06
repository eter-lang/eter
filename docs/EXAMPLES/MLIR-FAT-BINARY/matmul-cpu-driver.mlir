//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// =============================================================================
// CPU-only driver — @main entry point for matmul_cpu
//
// This module declares @matmul_cpu as an external function and provides the
// @main entry point that allocates 16×16 matrices, fills A/B with 1.0,
// C with 0.0, calls matmul_cpu, and returns C[0,0] as the exit code.
//
// Compiled separately and linked with the matmul_cpu object extracted from
// matmul-fat-binary.mlir via --symbol-privatize + --symbol-dce.
// =============================================================================
module {
  func.func private @matmul_cpu(memref<16x16xf32>, memref<16x16xf32>, memref<16x16xf32>)

  func.func @main() -> i32 {
    %A = memref.alloc() : memref<16x16xf32>
    %B = memref.alloc() : memref<16x16xf32>
    %C = memref.alloc() : memref<16x16xf32>
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c16 = arith.constant 16 : index
    %one = arith.constant 1.0 : f32
    %zero_f = arith.constant 0.0 : f32
    scf.for %i = %c0 to %c16 step %c1 {
      scf.for %j = %c0 to %c16 step %c1 {
        memref.store %one, %A[%i, %j] : memref<16x16xf32>
        memref.store %one, %B[%i, %j] : memref<16x16xf32>
        memref.store %zero_f, %C[%i, %j] : memref<16x16xf32>
      }
    }
    func.call @matmul_cpu(%A, %B, %C)
        : (memref<16x16xf32>, memref<16x16xf32>, memref<16x16xf32>) -> ()
    %result = memref.load %C[%c0, %c0] : memref<16x16xf32>
    %res_i32 = arith.fptosi %result : f32 to i32
    memref.dealloc %A : memref<16x16xf32>
    memref.dealloc %B : memref<16x16xf32>
    memref.dealloc %C : memref<16x16xf32>
    return %res_i32 : i32
  }
}
