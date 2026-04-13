#!/usr/bin/env sh

#===----------------------------------------------------------------------===#
#
# Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
# See /LICENSE for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

set -e
cd "$(dirname "$0")"

MLIR_OPT="/opt/homebrew/opt/llvm/bin/mlir-opt"
INPUT="matmul-fat-binary.mlir"
OUTPUT="matmul-fat-binary.validated.mlir"

if [ ! -x "$MLIR_OPT" ]; then
  MLIR_OPT="$(command -v mlir-opt 2>/dev/null || true)"
fi

if [ -z "$MLIR_OPT" ] || [ ! -x "$MLIR_OPT" ]; then
  echo "ERROR: mlir-opt not found in PATH or at /opt/homebrew/opt/llvm/bin/mlir-opt" 1>&2
  exit 1
fi

echo "Validating $INPUT with mlir-opt..."
"$MLIR_OPT" "$INPUT" > "$OUTPUT"

echo "Validation complete. Canonical output written to $OUTPUT"
