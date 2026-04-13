#===----------------------------------------------------------------------===#
#
# Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
# See /LICENSE for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

METAL_CPP_URL := https://developer.apple.com/metal/cpp/files/metal-cpp_26.4.zip
METAL_CPP_ZIP := metal-cpp_26.4.zip
METAL_CPP_DIR := metal-cpp

CXX := clang++
CXXFLAGS := -std=c++17 -fno-objc-arc -I./$(METAL_CPP_DIR)
FRAMEWORKS := -framework Metal -framework Foundation -framework QuartzCore

SOURCES := GetGPU.cpp ShaderAOT.cpp ShaderJIT.cpp
TARGETS := GetGPU ShaderAOT ShaderJIT

.PHONY: all download clean distclean help

help:
	@echo "Available targets:"
	@echo "  help      - Show this help message"
	@echo "  download  - Download the Metal C++ SDK locally"
	@echo "  all       - Download SDK and build all examples"
	@echo "  GetGPU    - Build the GetGPU example"
	@echo "  ShaderAOT - Build the ShaderAOT example"
	@echo "  ShaderJIT - Build the ShaderJIT example"
	@echo "  clean     - Remove built binaries and generated files"
	@echo "  distclean - Remove downloaded SDK and build artifacts"

all: download GetGPU ShaderAOT ShaderJIT

download: $(METAL_CPP_DIR)

$(METAL_CPP_DIR):
	@echo "Downloading Metal C++ SDK..."
	@if [ ! -d "$(METAL_CPP_DIR)" ]; then \
		curl -L -o "$(METAL_CPP_ZIP)" "$(METAL_CPP_URL)" && \
		unzip -o "$(METAL_CPP_ZIP)"; \
	fi
	@echo "Metal C++ SDK ready."

GetGPU: GetGPU.cpp
	$(CXX) $(CXXFLAGS) $(FRAMEWORKS) GetGPU.cpp -o GetGPU

ShaderAOT: Kernels.metallib ShaderAOT.cpp
	$(CXX) $(CXXFLAGS) $(FRAMEWORKS) ShaderAOT.cpp -o ShaderAOT

ShaderJIT: ShaderJIT.cpp
	$(CXX) $(CXXFLAGS) $(FRAMEWORKS) ShaderJIT.cpp -o ShaderJIT

Kernels.metallib: Kernels.metal
	xcrun -sdk macosx metal -c Kernels.metal -o Kernels.air
	xcrun -sdk macosx metallib Kernels.air -o Kernels.metallib

clean:
	rm -f GetGPU ShaderAOT ShaderJIT Kernels.air Kernels.metallib

distclean: clean
	rm -rf $(METAL_CPP_DIR) $(METAL_CPP_ZIP)
