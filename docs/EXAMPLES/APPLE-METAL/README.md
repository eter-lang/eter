# Apple Metal Examples

This folder contains Apple Metal example applications that demonstrate Metal and Metal C++ usage on macOS.

There are three example cases:

- `GetGPU.cpp`: Shows how to query and print GPU device information using the Metal API.
- `ShaderAOT.cpp`: Demonstrates Ahead-Of-Time shader compilation and execution using precompiled Metal shaders.
- `ShaderJIT.cpp`: Demonstrates Just-In-Time shader compilation and execution using runtime Metal shader compilation.


## Metal C++ SDK

The `metal-cpp` subfolder contains third-party Apple Metal C++ SDK files. These files should not be committed to the repository and are intended to be downloaded locally when needed.

Use the included `Makefile` to download the SDK and build all examples:

```sh
cd docs/EXAMPLES/APPLE-METAL
make download
make all
```

If `make all` fails because the Metal toolchain is missing, install it with:

```sh
xcodebuild -downloadComponent MetalToolchain
```

If you prefer to download manually:

```sh
cd docs/EXAMPLES/APPLE-METAL
curl -L -o metal-cpp_26.4.zip https://developer.apple.com/metal/cpp/files/metal-cpp_26.4.zip
unzip -o metal-cpp_26.4.zip
```

Once downloaded, keep `metal-cpp/` locally and excluded from git.


## Example details

### GetGPU

`GetGPU.cpp` is the simplest example. It initializes the Metal runtime and queries the available GPU device(s). The example prints information about the selected Metal device, making it useful as a minimal test that the Metal environment is available.

### ShaderAOT

`ShaderAOT.cpp` demonstrates how to use `eter` with a precompiled Metal shader. This example loads an Ahead-Of-Time compiled shader binary and uses it to run a compute or rendering operation. It is useful for workflows where shader compilation happens before application launch.

### ShaderJIT

`ShaderJIT.cpp` demonstrates a Just-In-Time shader workflow. It compiles Metal shader source at runtime and executes it immediately. This is useful when shader code is generated or modified dynamically and compilation must happen while the application is running.

## Notes

- These examples rely on Apple Metal and Metal C++ support.
- They are intended for macOS systems with a supported GPU and the Metal framework installed.

## References

- Apple Metal C++ documentation: https://developer.apple.com/metal/cpp/
- Apple Metal programming documentation: https://developer.apple.com/metal/
- Apple Metal Shading Language specification: https://developer.apple.com/metal/Metal-Shading-Language-Specification.pdf
- Apple Metal Performance Shaders documentation: https://developer.apple.com/documentation/metalperformanceshaders?language=objc
