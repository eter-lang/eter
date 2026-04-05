// Part of the Eter Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// How to compile:
// xcrun -sdk macosx metal -c Kernels.metal -o Kernels.air
// xcrun -sdk macosx metallib Kernels.air -o Kernels.metallib

#include <metal_stdlib>
using namespace metal;

kernel void multiply_kernel(device const float* in [[ buffer(0) ]],
                           device float* out [[ buffer(1) ]],
                           uint id [[ thread_position_in_grid ]]) {
    out[id] = in[id] * 2.0f;
}