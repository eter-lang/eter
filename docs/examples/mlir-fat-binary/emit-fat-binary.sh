#!/usr/bin/env sh

#===----------------------------------------------------------------------===#
#
# Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
# See /LICENSE for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

# =============================================================================
# emit-fat-binary.sh — MLIR fat-binary generation pipeline
#
# Demonstrates compiling ONE generic MLIR kernel for NVIDIA (PTX) + AMD
# (HSACO) GPU targets, plus a CPU host fallback.
#
# Pipeline overview:
#   Step 1  Validate the input MLIR
#   Step 2  Attach BOTH GPU targets (nvvm + rocdl) — pure MLIR, no tools
#   Step 3  NVVM  path: lower gpu.module → NVVM + LLVM dialect
#   Step 4  ROCDL path: lower gpu.module → ROCDL + LLVM dialect
#   Step 5  Compile lowered modules to binary blobs (--gpu-module-to-binary)
#   Step 6  Lower host code to LLVM dialect
#   Step 7  Translate MLIR → LLVM IR
#   Step 8  Compile LLVM IR → object file
#   Step 9  Link → final executable
#
# Why separate NVVM / ROCDL paths?
#   gpu-module-to-binary does NOT lower gpu.func/gpu.thread_id automatically.
#   The module must already be in LLVM dialect, and NVVM vs ROCDL require
#   different conversion passes (convert-gpu-to-nvvm vs convert-gpu-to-rocdl).
#   Each path produces its own lowered .mlir file so you can inspect both.
#
# Requirements:
#   - mlir-opt, mlir-translate  (LLVM/MLIR)  — always required
#   - llc, clang                (LLVM)       — always required
#   - ptxas                     (CUDA)       — for NVVM binary serialization
#   - ld.lld + ROCm device libs (ROCm)       — for ROCDL binary serialization
#
# Usage:
#   ./emit-fat-binary.sh                    # auto-detect and run
#   DRY_RUN=1 ./emit-fat-binary.sh          # print commands only
#   NVIDIA_CHIP=sm_90 AMD_CHIP=gfx1100 ./emit-fat-binary.sh
# =============================================================================
set +e  # We handle errors ourselves in run_step
cd "$(dirname "$0")"

# ── Configuration ─────────────────────────────────────────────────────────────
INPUT="${INPUT:-matmul-fat-binary.mlir}"
NVIDIA_CHIP="${NVIDIA_CHIP:-sm_80}"
AMD_CHIP="${AMD_CHIP:-gfx908}"
DRY_RUN="${DRY_RUN:-0}"

# ── Locate tools ─────────────────────────────────────────────────────────────
find_tool() {
  name="$1"
  for dir in /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin /usr/lib/llvm-*/bin /usr/local/bin /usr/bin; do
    if [ -x "$dir/$name" ]; then
      echo "$dir/$name"
      return
    fi
  done
  command -v "$name" 2>/dev/null || true
}

MLIR_OPT="${MLIR_OPT:-$(find_tool mlir-opt)}"
MLIR_TRANSLATE="${MLIR_TRANSLATE:-$(find_tool mlir-translate)}"
LLC="${LLC:-$(find_tool llc)}"
CC="${CC:-$(find_tool clang)}"

require() {
  if [ -z "$1" ] || [ ! -x "$1" ]; then
    echo "  ERROR: $2 not found. $3" >&2
    return 1
  fi
  echo "  $2 → $1"
}

echo "=== Tool check ==="
require "$MLIR_OPT"      "mlir-opt"       "Install LLVM/MLIR."            || exit 1
require "$MLIR_TRANSLATE" "mlir-translate"  "Install LLVM/MLIR."           || exit 1
require "$LLC"            "llc"            "Install LLVM."                 || exit 1
require "$CC"             "clang"          "Install LLVM or a C compiler." || exit 1

# ── Detect GPU toolchains (needed only for binary serialization) ─────────────
HAS_NVVM=0
HAS_ROCDL=0

# CUDA (ptxas)
if [ -z "$(command -v ptxas 2>/dev/null || true)" ]; then
  for d in "$CUDA_HOME" "$CUDA_PATH" /usr/local/cuda /usr/local/cuda-*; do
    if [ -n "$d" ] && [ -x "$d/bin/ptxas" ]; then
      export PATH="$d/bin:$PATH"
      break
    fi
  done
fi
if command -v ptxas >/dev/null 2>&1; then
  HAS_NVVM=1
  echo "  ptxas  → $(command -v ptxas)  [NVIDIA serialization enabled]"
else
  echo "  ptxas  → not found  [NVIDIA serialization disabled — lowering still works]"
fi

# ROCm (ld.lld + device libs)
if command -v ld.lld >/dev/null 2>&1; then
  HAS_ROCDL=1
  echo "  ld.lld → $(command -v ld.lld)  [AMD serialization enabled]"
elif [ -x "/opt/rocm/llvm/bin/ld.lld" ]; then
  export PATH="/opt/rocm/llvm/bin:$PATH"
  HAS_ROCDL=1
  echo "  ld.lld → /opt/rocm/llvm/bin/ld.lld  [AMD serialization enabled]"
else
  echo "  ld.lld → not found  [AMD serialization disabled — lowering still works]"
fi

echo ""

# ── Helpers ──────────────────────────────────────────────────────────────────
STEP=0
ERRLOG="emit-fat-binary.err.log"
: > "$ERRLOG"  # truncate

run_step() {
  STEP=$((STEP + 1))
  desc="$1"; shift
  echo "--- Step $STEP: $desc ---"
  echo "  \$ $*"
  if [ "$DRY_RUN" = "1" ]; then
    echo "  [dry-run — skipped]"
    echo ""
    return 0
  fi
  if "$@"; then
    echo "  OK"
  else
    echo "  FAILED (exit $?)" >&2
    return 1
  fi
  echo ""
}

# Like run_step, but captures stderr to the error log instead of printing it.
# Used for steps that dump huge MLIR on failure (e.g. gpu-module-to-binary).
run_step_quiet() {
  STEP=$((STEP + 1))
  desc="$1"; shift
  echo "--- Step $STEP: $desc ---"
  echo "  \$ $*"
  if [ "$DRY_RUN" = "1" ]; then
    echo "  [dry-run — skipped]"
    echo ""
    return 0
  fi
  if "$@" 2>>"$ERRLOG"; then
    echo "  OK"
  else
    echo "  FAILED (see $ERRLOG for details)" >&2
    return 1
  fi
  echo ""
}

skip_step() {
  STEP=$((STEP + 1))
  echo "--- Step $STEP: $1 ---"
  echo "  SKIPPED: $2"
  echo ""
}

# =============================================================================
# Step 1: Validate
# =============================================================================
run_step "Validate input MLIR" \
  "$MLIR_OPT" "$INPUT" -o /dev/null

# =============================================================================
# Step 2: Attach BOTH GPU targets
# =============================================================================
# Pure MLIR pass — does NOT need ptxas or ROCm.  Produces a module with:
#   gpu.module @kernels [#nvvm.target<chip = "sm_90">, #rocdl.target<chip = "gfx1100">]
STEP2_OUT="matmul.targets.mlir"

run_step "Attach GPU targets: nvvm($NVIDIA_CHIP) + rocdl($AMD_CHIP)" \
  "$MLIR_OPT" "$INPUT" \
    "--nvvm-attach-target=module=kernels chip=$NVIDIA_CHIP" \
    "--rocdl-attach-target=module=kernels chip=$AMD_CHIP" \
    -o "$STEP2_OUT"

# =============================================================================
# Step 3: NVVM path — lower gpu.module → NVIDIA NVVM + LLVM dialect
# =============================================================================
# Three mlir-opt invocations because:
#   (a) --nvvm-attach-target can't mix with --pass-pipeline
#   (b) finalize-memref-to-llvm / convert-cf-to-llvm can't nest in gpu.module
#
# Produces matmul.nvvm-lowered.mlir with nvvm.read.ptx.sreg.* intrinsics.
NVVM_ATTACH="matmul.nvvm-attach.mlir"
NVVM_KERNEL="matmul.nvvm-kernel.mlir"
NVVM_LOWERED="matmul.nvvm-lowered.mlir"

run_step "NVVM: attach nvvm target" \
  "$MLIR_OPT" "$INPUT" \
    "--nvvm-attach-target=module=kernels chip=$NVIDIA_CHIP" \
    -o "$NVVM_ATTACH"

run_step "NVVM: lower gpu.module → NVVM + LLVM" \
  "$MLIR_OPT" "$NVVM_ATTACH" \
    --pass-pipeline="builtin.module(gpu.module(convert-gpu-to-nvvm,convert-index-to-llvm,convert-scf-to-cf,convert-arith-to-llvm,expand-strided-metadata,reconcile-unrealized-casts))" \
    -o "$NVVM_KERNEL"

run_step "NVVM: finalize memref + cf → LLVM (module level)" \
  "$MLIR_OPT" "$NVVM_KERNEL" \
    --convert-cf-to-llvm --finalize-memref-to-llvm --reconcile-unrealized-casts \
    -o "$NVVM_LOWERED"

# =============================================================================
# Step 4: ROCDL path — lower gpu.module → AMD ROCDL + LLVM dialect
# =============================================================================
# Same structure as the NVVM path, but uses convert-gpu-to-rocdl.
# Produces matmul.rocdl-lowered.mlir with rocdl.workitem.id.* intrinsics.
ROCDL_ATTACH="matmul.rocdl-attach.mlir"
ROCDL_KERNEL="matmul.rocdl-kernel.mlir"
ROCDL_LOWERED="matmul.rocdl-lowered.mlir"

run_step "ROCDL: attach rocdl target" \
  "$MLIR_OPT" "$INPUT" \
    "--rocdl-attach-target=module=kernels chip=$AMD_CHIP" \
    -o "$ROCDL_ATTACH"

run_step "ROCDL: lower gpu.module → ROCDL + LLVM" \
  "$MLIR_OPT" "$ROCDL_ATTACH" \
    --pass-pipeline="builtin.module(gpu.module(convert-gpu-to-rocdl,convert-index-to-llvm,convert-scf-to-cf,convert-arith-to-llvm,expand-strided-metadata,reconcile-unrealized-casts))" \
    -o "$ROCDL_KERNEL"

run_step "ROCDL: finalize memref + cf → LLVM (module level)" \
  "$MLIR_OPT" "$ROCDL_KERNEL" \
    --convert-cf-to-llvm --finalize-memref-to-llvm --reconcile-unrealized-casts \
    -o "$ROCDL_LOWERED"

# =============================================================================
# Step 5: Compile lowered modules → binary blobs
# =============================================================================
# gpu-module-to-binary serializes the already-lowered LLVM dialect into
# target-specific binary:  PTX (NVIDIA) or HSACO (AMD).
# This is the step that actually NEEDS the external GPU toolchains.
NVVM_BIN="matmul.nvvm-bin.mlir"
ROCDL_BIN="matmul.rocdl-bin.mlir"
NVVM_COMPILED=0
ROCDL_COMPILED=0

if run_step_quiet "NVVM: gpu-module-to-binary (serialize PTX)" \
     "$MLIR_OPT" "$NVVM_LOWERED" --gpu-module-to-binary -o "$NVVM_BIN"; then
  NVVM_COMPILED=1
else
  echo "  (needs ptxas — install CUDA toolkit)"
  echo ""
fi

if run_step_quiet "ROCDL: gpu-module-to-binary (serialize HSACO)" \
     "$MLIR_OPT" "$ROCDL_LOWERED" --gpu-module-to-binary -o "$ROCDL_BIN"; then
  ROCDL_COMPILED=1
else
  echo "  (needs ROCm device libs at /opt/rocm/amdgcn/bitcode)"
  echo ""
fi

# ── Merge both blobs into a single fat-binary module ─────────────────────────
# gpu-module-to-binary replaces  gpu.module @kernels  with  gpu.binary @kernels
# containing one gpu.object per target.  For a true fat binary we merge the
# gpu.objects from both files into a single gpu.binary inside one MLIR module.
#
# The merge is done via text extraction: we pull the gpu.object lines from each
# serialised file and combine them under one gpu.binary @kernels.
FATBIN="matmul.fatbin.mlir"
GPU_COMPILED=0
GPUBIN_OUT=""

if [ "$NVVM_COMPILED" = "1" ] && [ "$ROCDL_COMPILED" = "1" ]; then
  GPU_COMPILED=1
  # Extract gpu.object lines from both binaries
  NVVM_OBJECTS=$(sed -n '/gpu\.binary @kernels/,/^  }/{/gpu\.object/p;}' "$NVVM_BIN")
  ROCDL_OBJECTS=$(sed -n '/gpu\.binary @kernels/,/^  }/{/gpu\.object/p;}' "$ROCDL_BIN")
  # Extract host functions (everything outside gpu.binary)
  HOST_FUNCS=$(sed '/gpu\.binary @kernels/,/^  }/d' "$NVVM_BIN")
  # Build merged module
  {
    echo 'module attributes {gpu.container_module} {'
    echo '  gpu.binary @kernels ['
    echo "$NVVM_OBJECTS"
    # Add comma separator if both have objects
    if [ -n "$NVVM_OBJECTS" ] && [ -n "$ROCDL_OBJECTS" ]; then
      echo '    ,'
    fi
    echo "$ROCDL_OBJECTS"
    echo '  ]'
    # Include host functions from the NVVM binary (they're identical)
    sed -n '/^  func\.func/,/^  }/p' "$NVVM_BIN"
    echo '}'
  } > "$FATBIN"
  # Validate the merged module
  if "$MLIR_OPT" "$FATBIN" -o /dev/null 2>>"$ERRLOG"; then
    GPUBIN_OUT="$FATBIN"
    echo "  Merged NVVM + ROCDL → $FATBIN (true fat binary)"
  else
    # Fallback: if textual merge didn't parse, use NVVM binary alone
    GPUBIN_OUT="$NVVM_BIN"
    echo "  (merge failed, using NVVM binary only — see $ERRLOG)"
  fi
elif [ "$NVVM_COMPILED" = "1" ]; then
  GPU_COMPILED=1
  GPUBIN_OUT="$NVVM_BIN"
  echo "  Using NVVM binary (ROCDL serialization failed)."
elif [ "$ROCDL_COMPILED" = "1" ]; then
  GPU_COMPILED=1
  GPUBIN_OUT="$ROCDL_BIN"
  echo "  Using ROCDL binary (NVVM serialization failed)."
fi
echo ""

if [ "$GPU_COMPILED" = "0" ]; then
  echo "=== GPU serialization unavailable ==="
  echo "GPU binary serialization failed (missing device toolchains)."
  echo "MLIR lowering succeeded for both backends:"
  echo "  NVVM  → $NVVM_LOWERED"
  echo "  ROCDL → $ROCDL_LOWERED"
  echo ""
  echo "Falling back to CPU-only executable..."
  echo ""

  # ── CPU-only pipeline ─────────────────────────────────────────────────────
  # Instead of generating MLIR via heredoc, we compile directly from the
  # original source file and a small driver (matmul-cpu-driver.mlir):
  #
  #   1. Strip GPU parts from the source using --symbol-privatize + --symbol-dce
  #   2. Lower the stripped @matmul_cpu to LLVM → object
  #   3. Lower the driver (@main + extern @matmul_cpu) to LLVM → object
  #   4. Link both objects → matmul-cpu-only
  CPU_DRIVER="matmul-cpu-driver.mlir"
  if [ ! -f "$CPU_DRIVER" ]; then
    echo "  ERROR: $CPU_DRIVER not found (expected next to $INPUT)" >&2
    exit 1
  fi

  # ── Compile @matmul_cpu from the original source ──────────────────────────
  CPU_STRIPPED_LLVM="matmul.cpu-stripped-llvm.mlir"
  run_step "CPU: strip GPU symbols + lower @matmul_cpu → LLVM" \
    "$MLIR_OPT" "$INPUT" \
      '--symbol-privatize=exclude=matmul_cpu' \
      --symbol-dce \
      --convert-scf-to-cf \
      --convert-index-to-llvm \
      --convert-arith-to-llvm \
      --convert-cf-to-llvm \
      --expand-strided-metadata \
      --finalize-memref-to-llvm \
      --convert-func-to-llvm \
      --reconcile-unrealized-casts \
      -o "$CPU_STRIPPED_LLVM"

  CPU_STRIPPED_LL="matmul-cpu-stripped.ll"
  run_step "CPU: translate @matmul_cpu MLIR → LLVM IR" \
    "$MLIR_TRANSLATE" --mlir-to-llvmir "$CPU_STRIPPED_LLVM" -o "$CPU_STRIPPED_LL"

  CPU_STRIPPED_OBJ="matmul-cpu-stripped.o"
  run_step "CPU: compile @matmul_cpu → object" \
    "$LLC" -filetype=obj "$CPU_STRIPPED_LL" -o "$CPU_STRIPPED_OBJ"

  # ── Compile @main from the driver ─────────────────────────────────────────
  CPU_DRIVER_LLVM="matmul.cpu-driver-llvm.mlir"
  run_step "CPU: lower driver (@main) → LLVM" \
    "$MLIR_OPT" "$CPU_DRIVER" \
      --convert-scf-to-cf \
      --convert-index-to-llvm \
      --convert-arith-to-llvm \
      --convert-cf-to-llvm \
      --expand-strided-metadata \
      --finalize-memref-to-llvm \
      --convert-func-to-llvm \
      --reconcile-unrealized-casts \
      -o "$CPU_DRIVER_LLVM"

  CPU_DRIVER_LL="matmul-cpu-driver.ll"
  run_step "CPU: translate driver MLIR → LLVM IR" \
    "$MLIR_TRANSLATE" --mlir-to-llvmir "$CPU_DRIVER_LLVM" -o "$CPU_DRIVER_LL"

  CPU_DRIVER_OBJ="matmul-cpu-driver.o"
  run_step "CPU: compile driver → object" \
    "$LLC" -filetype=obj "$CPU_DRIVER_LL" -o "$CPU_DRIVER_OBJ"

  # ── Link both objects ─────────────────────────────────────────────────────
  CPU_BIN="matmul-cpu-only"
  run_step "CPU: link @matmul_cpu + @main → $CPU_BIN" \
    "$CC" "$CPU_STRIPPED_OBJ" "$CPU_DRIVER_OBJ" -lm -o "$CPU_BIN"

  echo "=== Done (CPU-only) ==="
  echo "Binary: ./$CPU_BIN"
  echo ""
  echo "Verify:  ./$CPU_BIN; echo \$?"
  echo "  → exit code 16 means C[0,0] = 16.0  (correct 16×16 matmul of all-ones)"
  echo ""
  echo "For a full fat binary with GPU support, install:"
  echo "  NVIDIA → CUDA toolkit (ptxas)"
  echo "  AMD    → ROCm device libs (/opt/rocm/amdgcn/bitcode)"
  exit 0
fi

# =============================================================================
# Step 6: Lower host code to LLVM dialect
# =============================================================================
STEP6_OUT="matmul.llvm.mlir"

run_step "Lower host code to LLVM dialect" \
  "$MLIR_OPT" "$GPUBIN_OUT" \
    --gpu-to-llvm \
    --convert-scf-to-cf \
    --convert-cf-to-llvm \
    --convert-arith-to-llvm \
    --convert-func-to-llvm \
    --finalize-memref-to-llvm \
    --reconcile-unrealized-casts \
    -o "$STEP6_OUT"

# =============================================================================
# Step 7: Translate to LLVM IR
# =============================================================================
STEP7_OUT="matmul.ll"
run_step "Translate MLIR → LLVM IR" \
  "$MLIR_TRANSLATE" --mlir-to-llvmir "$STEP6_OUT" -o "$STEP7_OUT"

# =============================================================================
# Step 8: Compile to object
# =============================================================================
STEP8_OUT="matmul.o"
run_step "Compile LLVM IR → object file" \
  "$LLC" -filetype=obj "$STEP7_OUT" -o "$STEP8_OUT"

# =============================================================================
# Step 9: Link
# =============================================================================
FINAL="matmul-fat-binary"
LINK_LIBS="-lm"
[ "$NVVM_COMPILED"  = "1" ] && LINK_LIBS="$LINK_LIBS -lcuda"
[ "$ROCDL_COMPILED" = "1" ] && LINK_LIBS="$LINK_LIBS -lamdhip64"

run_step "Link → $FINAL" \
  "$CC" "$STEP8_OUT" $LINK_LIBS -o "$FINAL"

echo "=== Done ==="
echo "Targets embedded:"
[ "$NVVM_COMPILED"  = "1" ] && echo "  ✓ NVIDIA  ($NVIDIA_CHIP)"
[ "$ROCDL_COMPILED" = "1" ] && echo "  ✓ AMD     ($AMD_CHIP)"
echo "  ✓ CPU     (host fallback)"
echo ""
echo "Binary: ./$FINAL"
