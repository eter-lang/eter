//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// How to compile:
// 1. xcrun -sdk macosx metal -c Kernels.metal -o Kernels.air
// 2. xcrun -sdk macosx metallib Kernels.air -o Kernels.metallib
// 3. clang++ -std=c++17 -fno-objc-arc -I./metal-cpp \
//    -framework Metal -framework Foundation -framework QuartzCore \
//    ShaderAOT.cpp -o ShaderAOT

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include <metal/Metal.hpp>
#include <iostream>

int main() {
    // 1. Access the M4 Pro GPU hardware
    MTL::Device* device = MTL::CreateSystemDefaultDevice();
    if (!device) {
        std::cerr << "Metal is not supported." << std::endl;
        return -1;
    }

    std::cout << "AOT Engine: Initializing " << device->name()->utf8String() << std::endl;

    // 2. Load the Pre-compiled Library (.metallib)
    // AOT compilation avoids the overhead of compiling source code at runtime
    NS::Error* error = nullptr;
    NS::String* path = NS::String::string("./Kernels.metallib", NS::UTF8StringEncoding);
    MTL::Library* lib = device->newLibrary(path, &error);

    if (!lib) {
        std::cerr << "AOT Error: Could not load Kernels.metallib. Ensure it was compiled via xcrun." << std::endl;
        return -1;
    }

    // Identify the pre-compiled function within the library
    MTL::Function* func = lib->newFunction(NS::String::string("multiply_kernel", NS::UTF8StringEncoding));
    if (!func) {
        std::cerr << "AOT Error: Could not find function 'multiply_kernel' in Kernels.metallib." << std::endl;
        lib->release();
        device->release();
        return -1;
    }

    // 3. Create the Pipeline State Object (PSO)
    // Pre-compiled functions are linked here for the M4 Pro's specific execution units
    MTL::ComputePipelineState* pso = device->newComputePipelineState(func, &error);
    if (!pso) {
        std::cerr << "AOT Error: Could not create compute pipeline state: " << error->localizedDescription()->utf8String() << std::endl;
        func->release();
        lib->release();
        device->release();
        return -1;
    }

    // 4. Memory Allocation (Unified Memory Architecture)
    // We allocate shared buffers to eliminate data transfer between CPU and GPU
    const int count = 1024;
    size_t size = count * sizeof(float);
    
    MTL::Buffer* bufferIn = device->newBuffer(size, MTL::ResourceStorageModeShared);
    MTL::Buffer* bufferOut = device->newBuffer(size, MTL::ResourceStorageModeShared);

    // Initialize input data directly in the shared memory
    float* inPtr = (float*)bufferIn->contents();
    for(int i = 0; i < count; i++) inPtr[i] = (float)i;

    // 5. Command Dispatch Pipeline
    MTL::CommandQueue* queue = device->newCommandQueue();
    MTL::CommandBuffer* cmdBuf = queue->commandBuffer();
    MTL::ComputeCommandEncoder* encoder = cmdBuf->computeCommandEncoder();

    encoder->setComputePipelineState(pso);
    encoder->setBuffer(bufferIn, 0, 0); 
    encoder->setBuffer(bufferOut, 0, 1); 

    // 6. Define Workload Grid
    // M4 Pro's GPU architecture is highly efficient with 64-thread groups
    MTL::Size gridSize = MTL::Size(count, 1, 1);
    MTL::Size threadGroupSize = MTL::Size(64, 1, 1); 
    encoder->dispatchThreads(gridSize, threadGroupSize);

    encoder->endEncoding();
    
    // 7. Execution and Synchronization
    cmdBuf->commit();
    cmdBuf->waitUntilCompleted(); 

    // 8. Result Verification
    float* outPtr = (float*)bufferOut->contents();
    std::cout << "AOT Execution successful. Result[10]: " << outPtr[10] << std::endl;

    // 9. Manual Resource Management
    // Explicitly releasing Metal objects to prevent memory leaks in the Engine
    bufferIn->release(); 
    bufferOut->release();
    pso->release(); 
    func->release(); 
    lib->release();
    queue->release(); 
    device->release();

    return 0;
}