// Part of the Eter Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// How to compile:
// clang++ -std=c++17 -fno-objc-arc \
//     -I./metal-cpp \
//     -framework Metal -framework Foundation -framework QuartzCore \
//     GetGPU.cpp -o GetGPU

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include <Metal/Metal.hpp>
#include <iostream>

int main() {
    // 1. Get access to the GPU (your M4 Pro chip)
    // This creates an interface to the hardware.
    MTL::Device *device = MTL::CreateSystemDefaultDevice();

    if (!device) {
        std::cerr << "Metal is not supported on this system." << std::endl;
        return -1;
    }

    // Print the name of the GPU to the console (e.g., "Apple M4 Pro")
    std::cout << "GPU detected: " << device->name()->utf8String() << std::endl;

    // 2. Create a command queue
    // This is the "lane" where you will submit your GPU tasks/commands.
    MTL::CommandQueue *queue = device->newCommandQueue();

    // 3. Cleanup
    // Metal-cpp requires manual memory management using the ->release() method.
    queue->release();
    device->release();

    return 0;
}