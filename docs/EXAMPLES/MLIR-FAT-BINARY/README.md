# MLIR Fat Binary Example

This example demonstrates a real MLIR fat binary: **one kernel** compiled for
**multiple GPU targets** (NVIDIA + AMD) plus a **CPU fallback**, producing a
single executable binary.

## How it works

```
                        matmul-fat-binary.mlir
                     (one generic gpu.func kernel)
                                 │
              ┌──────────────────┼──────────────────┐
              ▼                  ▼                   ▼
        NVVM path          ROCDL path           (CPU path)
      convert-gpu-        convert-gpu-              │
        to-nvvm             to-rocdl                │
              │                  │                   │
              ▼                  ▼                   │
      gpu-module-to-      gpu-module-to-             │
         binary              binary                  │
       (PTX blob)         (HSACO blob)               │
              │                  │                   │
              └────────┬─────────┘                   │
                       ▼                             │
                  merge blobs               symbol-privatize
                       │                      + symbol-dce
                       ▼                             │
                  gpu-to-llvm               ┌────────┴────────┐
                  + host lower              ▼                  ▼
                       │              @matmul_cpu      @main (driver)
                       ▼              (from source)   (separate file)
                 mlir-translate             │                  │
                 (MLIR → LLVM IR)          ▼                  ▼
                       │              mlir-translate    mlir-translate
                       ▼             (MLIR → LLVM IR) (MLIR → LLVM IR)
                     llc                   │                  │
                 (LLVM IR → .o)            ▼                  ▼
                       │                 llc                llc
                       ▼             (LLVM IR → .o)    (LLVM IR → .o)
                    clang                  │                  │
                (link → binary)            └────────┬─────────┘
                       │                            ▼
                       ▼                         clang
                matmul-fat-binary            (link → binary)
            (NVIDIA+AMD+CPU in one)                 │
                                                    ▼
                                            matmul-cpu-only
                                         (CPU fallback binary)
```

### Why two separate paths?

`gpu-module-to-binary` does **not** lower `gpu.func` / `gpu.thread_id`
automatically.  The module must already be in LLVM dialect before
serialization, and NVVM vs ROCDL require **different** conversion passes
(`convert-gpu-to-nvvm` produces `nvvm.read.ptx.sreg.*` intrinsics;
`convert-gpu-to-rocdl` produces `rocdl.workitem.*` intrinsics).

The script runs both paths from the same high-level kernel, producing separate
intermediate files so you can inspect both dialects side by side.

## Files

| File | Description |
|------|-------------|
| `matmul-fat-binary.mlir` | Single kernel + CPU fallback + host dispatcher |
| `matmul-cpu-driver.mlir` | CPU driver: `@main` entry point (declares `@matmul_cpu` as extern) |
| `emit-fat-binary.sh` | Full pipeline: MLIR → executable fat binary |
| `run-validate.sh` | Quick MLIR syntax validation |

## Generated intermediate files

| File | Content |
|------|---------|
| `matmul.targets.mlir` | Both targets attached: `#nvvm.target` + `#rocdl.target` |
| `matmul.nvvm-lowered.mlir` | Kernel lowered to NVVM + LLVM dialect (`nvvm.read.ptx.sreg.*`) |
| `matmul.rocdl-lowered.mlir` | Kernel lowered to ROCDL + LLVM dialect (`rocdl.workitem.*`) |
| `matmul.nvvm-bin.mlir` | NVVM binary blob (requires `ptxas`) |
| `matmul.rocdl-bin.mlir` | ROCDL binary blob (requires ROCm device libs) |
| `matmul.cpu-stripped-llvm.mlir` | `@matmul_cpu` stripped from source + lowered to LLVM |
| `matmul.cpu-driver-llvm.mlir` | Driver (`@main`) lowered to LLVM |
## Architecture

The MLIR module contains:

- **`gpu.module @kernels`** with a single `@matmul_kernel` — written using
  target-agnostic `gpu` dialect ops (no nvvm/rocdl intrinsics).
- **`@matmul_cpu`** — scalar CPU triple-loop fallback.
- **`@matmul_dispatch`** — launches the GPU kernel or falls back to CPU.

The compiler (not the programmer) produces multiple ISAs from the **same**
kernel source.  `--nvvm-attach-target` and `--rocdl-attach-target` tell MLIR
which backends to compile for, and `--gpu-module-to-binary` serialises the
lowered module into binary blobs.  At runtime the GPU runtime picks the right
one.

## Quick start

### Validate

```sh
cd docs/examples/mlir-fat-binary
./run-validate.sh
```

### Generate the fat binary

```sh
cd docs/examples/mlir-fat-binary
chmod +x emit-fat-binary.sh
./emit-fat-binary.sh
```

The script always attaches **both** targets (pure MLIR, no external tools
needed) and lowers for both.  External toolchains are only needed at the
serialization step:

| Toolchain | Required tool | What it enables |
|-----------|---------------|-----------------|
| NVIDIA CUDA | `ptxas` | PTX / SASS blob serialization |
| AMD ROCm | `ld.lld` + device libs | HSACO blob serialization |
| (none) | — | Lowering works; **CPU-only executable** produced as fallback |

### Customise

```sh
# Different chips
NVIDIA_CHIP=sm_90 AMD_CHIP=gfx1100 ./emit-fat-binary.sh

# Dry run — print all commands without executing
DRY_RUN=1 ./emit-fat-binary.sh
```

## Pipeline details (step by step)

If you prefer to run the commands manually:

```sh
MLIR_OPT=/opt/homebrew/opt/llvm/bin/mlir-opt
MLIR_TRANSLATE=/opt/homebrew/opt/llvm/bin/mlir-translate
LLC=/opt/homebrew/opt/llvm/bin/llc

# 1. Validate
$MLIR_OPT matmul-fat-binary.mlir -o /dev/null

# 2. Attach BOTH GPU targets (pure MLIR, no external tools)
$MLIR_OPT matmul-fat-binary.mlir \
    --nvvm-attach-target="module=kernels chip=sm_90" \
    --rocdl-attach-target="module=kernels chip=gfx1100" \
    -o matmul.targets.mlir
```

### NVVM path (NVIDIA)

Each target needs its own lowering because `--nvvm-attach-target` can't mix
with `--pass-pipeline`, and `finalize-memref-to-llvm` / `convert-cf-to-llvm`
can't nest inside `gpu.module` via CLI.

```sh
# 3a. Attach nvvm target
$MLIR_OPT matmul-fat-binary.mlir \
    --nvvm-attach-target="module=kernels chip=sm_90" \
    -o matmul.nvvm-attach.mlir

# 3b. Lower gpu.module → NVVM + LLVM dialect
$MLIR_OPT matmul.nvvm-attach.mlir \
    --pass-pipeline="builtin.module(gpu.module( \
        convert-gpu-to-nvvm, \
        convert-index-to-llvm, \
        convert-scf-to-cf, \
        convert-arith-to-llvm, \
        expand-strided-metadata, \
        reconcile-unrealized-casts))" \
    -o matmul.nvvm-kernel.mlir

# 3c. Finalize cf + memref at module level
$MLIR_OPT matmul.nvvm-kernel.mlir \
    --convert-cf-to-llvm \
    --finalize-memref-to-llvm \
    --reconcile-unrealized-casts \
    -o matmul.nvvm-lowered.mlir

# 3d. Serialize to PTX binary blob (requires ptxas)
$MLIR_OPT matmul.nvvm-lowered.mlir \
    --gpu-module-to-binary \
    -o matmul.nvvm-bin.mlir
```

### ROCDL path (AMD)

Same structure as NVVM, but uses `convert-gpu-to-rocdl`:

```sh
# 4a. Attach rocdl target
$MLIR_OPT matmul-fat-binary.mlir \
    --rocdl-attach-target="module=kernels chip=gfx1100" \
    -o matmul.rocdl-attach.mlir

# 4b. Lower gpu.module → ROCDL + LLVM dialect
$MLIR_OPT matmul.rocdl-attach.mlir \
    --pass-pipeline="builtin.module(gpu.module( \
        convert-gpu-to-rocdl, \
        convert-index-to-llvm, \
        convert-scf-to-cf, \
        convert-arith-to-llvm, \
        expand-strided-metadata, \
        reconcile-unrealized-casts))" \
    -o matmul.rocdl-kernel.mlir

# 4c. Finalize cf + memref at module level
$MLIR_OPT matmul.rocdl-kernel.mlir \
    --convert-cf-to-llvm \
    --finalize-memref-to-llvm \
    --reconcile-unrealized-casts \
    -o matmul.rocdl-lowered.mlir

# 4d. Serialize to HSACO binary blob (requires ROCm device libs)
$MLIR_OPT matmul.rocdl-lowered.mlir \
    --gpu-module-to-binary \
    -o matmul.rocdl-bin.mlir
```

### Host lowering + final binary

```sh
# 5. Lower host code to LLVM dialect
$MLIR_OPT matmul.nvvm-bin.mlir \
    --gpu-to-llvm \
    --convert-scf-to-cf \
    --convert-cf-to-llvm \
    --convert-arith-to-llvm \
    --convert-func-to-llvm \
    --finalize-memref-to-llvm \
    --reconcile-unrealized-casts \
    -o matmul.llvm.mlir

# 6. Translate to LLVM IR
$MLIR_TRANSLATE --mlir-to-llvmir matmul.llvm.mlir -o matmul.ll

# 7. Compile to object
$LLC -filetype=obj matmul.ll -o matmul.o

# 8. Link (add -lcuda / -lamdhip64 depending on targets)
clang matmul.o -o matmul-fat-binary -lm -lcuda -lamdhip64
```

## Toolchain setup

### NVIDIA (CUDA)

```sh
export CUDA_HOME=/usr/local/cuda
export PATH="$CUDA_HOME/bin:$PATH"
```

### AMD (ROCm)

```sh
export PATH=/opt/rocm/llvm/bin:/opt/rocm/bin:$PATH
# Device libraries must exist at /opt/rocm/amdgcn/bitcode
```

## CPU-only fallback

When neither CUDA nor ROCm toolchains are available, the script automatically
builds a **CPU-only executable** (`matmul-cpu-only`) instead of stopping.

The fallback compiles **directly from the source files** (no heredoc / text
generation).  `@matmul_cpu` is extracted from the original MLIR with
`--symbol-privatize` + `--symbol-dce`, and `@main` lives in a separate
driver file (`matmul-cpu-driver.mlir`).  The two are compiled independently
and linked at the object level:

1. Strip GPU symbols from `matmul-fat-binary.mlir` → keep only `@matmul_cpu`
2. Lower `@matmul_cpu` → LLVM IR → object
3. Lower `matmul-cpu-driver.mlir` (`@main`) → LLVM IR → object
4. Link both objects → `matmul-cpu-only`

```sh
./emit-fat-binary.sh
# ... Steps 1–10 (GPU lowering succeeds, serialization fails) ...
# ... Steps 11–17 (CPU-only pipeline) ...
#
# Binary: ./matmul-cpu-only

./matmul-cpu-only; echo $?
# → 16  (C[0,0] = 16.0 — correct result for 16×16 all-ones matmul)
```

This means the pipeline **always produces a runnable binary**, regardless of
whether GPU toolchains are installed.

## Notes

- The fat-binary mechanism is a core MLIR feature (`gpu-module-to-binary`),
  not a custom extension.
- On a machine without any GPU toolkit, the pipeline still lowers and shows
  the target-specific MLIR for both backends — only serialization to binary
  blobs requires external tools.  A CPU-only executable is produced as
  fallback.
- The `gpu.module` can hold targets for **any** number of GPU architectures
  simultaneously (e.g., `sm_80`, `sm_90`, `gfx908`, `gfx1100`).

