#!/usr/bin/env bash
set -euo pipefail

unset LDFLAGS
unset CPPFLAGS

# Discover LLVM_PREFIX_PATH if not explicitly set in the environment.
# Priority: environment variable > llvm-config (already in PATH) > brew > hardcoded fallback.
if [[ -z "${LLVM_PREFIX_PATH:-}" ]]; then
    if command -v llvm-config >/dev/null 2>&1; then
        LLVM_PREFIX_PATH=$(llvm-config --prefix)
    elif command -v brew >/dev/null 2>&1 && brew --prefix llvm@22 >/dev/null 2>&1; then
        LLVM_PREFIX_PATH=$(brew --prefix llvm@22)
    else
        LLVM_PREFIX_PATH=/opt/homebrew/opt/llvm@22
    fi
fi

# CMake needs CMAKE_PREFIX_PATH to locate LLVM/MLIR package config files.
CMAKE_PREFIX_PATH=${LLVM_PREFIX_PATH}

# Make LLVM tools (clang, clang-format, clang-tidy, FileCheck, lit, ...)
# available to this script and all subprocesses it spawns.
export PATH="${LLVM_PREFIX_PATH}/bin:$PATH"

# Explicit paths to the LLVM and MLIR CMake config directories.
# Passing these prevents CMake from picking up the wrong version when
# multiple LLVM installations coexist. Both can be overridden by setting
# them in the environment before calling x.
LLVM_DIR=${LLVM_DIR:-${LLVM_PREFIX_PATH}/lib/cmake/llvm}
MLIR_DIR=${MLIR_DIR:-${LLVM_PREFIX_PATH}/lib/cmake/mlir}

LIT_COMMAND=lit
BUILD_DIR=./build
CLANG_FORMAT_COMMIT=HEAD
CLANG_TIDY_COMMIT=HEAD

print_usage() {
    echo "Usage: x <command> [arguments]"
    echo ""
    echo "Commands:"
    echo "  config [BUILD_TYPE]                  Configure the Eter project with CMake."
    echo "  build                                Build the project using ninja."
    echo "  rebuild                              Re-configure and build the project from scratch."
    echo "  check-all                            Run all lit and unit tests."
    echo "  clang-format [CLANG_FORMAT_COMMIT]   Format staged changed files compared to a git commit using clang-format."
    echo "  clang-tidy [CLANG_TIDY_COMMIT]       Run clang-tidy on staged changed files compared to a git commit."
    echo "  tidy-all                             Run clang-tidy on all C/C++ files in the repository."
    echo "  clean                                Clean the build directory."
    echo "  help                                 Show this help message."
    echo ""
    echo "Additional commands:"
    echo "  check-eter                           Run the lit test suite."
    echo "  test                                 Run the unit tests."
    echo "  run-lit-test <folder|file>           Run the lit test suite, but parametrized for a specific folder/file."
    echo "  run-unit-test <binary>               Run a specific unit test binary (e.g. Lexer/LexerTest)."
    echo "  run-single-test <binary> <filter>    Run a single test case via --gtest_filter (e.g. Lexer/LexerTest LexerTest.LexIdentifier)."
    echo "  doc-doxygen                          Generate documentation using Doxygen."
    echo "  check-clang-format [COMMIT]          Run clang-format on staged changed files compared to a git commit."
    echo "  check-commit-clang-format [COMMIT]   Check if a specific commit is clang-format correct (default: HEAD)."
    echo "  check-commit-clang-tidy [COMMIT]     Check if a specific commit is clang-tidy correct (default: HEAD)."
    echo "  create-clangd                        Create a .clangd configuration file for the project."
    echo "  show-config                          Show the current configuration and environment variables."
    echo ""
    echo "Environment:"
    echo "  LLVM_PREFIX_PATH     Path to LLVM installation (auto-discovered if unset)"
    echo "  LLVM_DIR             Path to LLVM CMake config dir (derived from LLVM_PREFIX_PATH if unset)"
    echo "  MLIR_DIR             Path to MLIR CMake config dir (derived from LLVM_PREFIX_PATH if unset)"
    echo "  LIT_COMMAND          Command to run lit tests (default: lit)"
    echo "  BUILD_DIR            Directory for CMake build files (default: ./build)"
    echo ""
    echo "Options:"
    echo "  BUILD_TYPE           Build type for configuration (e.g., Debug, Release). Default is Debug."
    echo "  CLANG_FORMAT_COMMIT  Git commit to compare against for clang-format (default: HEAD)."
    echo "  CLANG_TIDY_COMMIT    Git commit to compare against for clang-tidy (default: HEAD)."
    echo ""
    echo "Location:"
    echo "  x is located at: $(realpath $0)"
    # FUTURE ADDITIONS:
    # echo "  update-test-checks <file>             Update the FileCheck patterns in the given test file."
}


# Show help message if no arguments are provided
if [ $# -eq 0 ]; then
    print_usage
    exit 0
fi



COMMAND=$1
shift # Remove the command from the arguments list

case $COMMAND in
    config)
        BUILD_TYPE=${1:-Debug}
        echo "[x] Configuring Eter project"
        rm -rf ${BUILD_DIR}
        mkdir -p ${BUILD_DIR}
        cmake -G Ninja -S . -B ${BUILD_DIR} \
            -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
            -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH} \
            -DCMAKE_C_COMPILER=${LLVM_PREFIX_PATH}/bin/clang \
            -DCMAKE_CXX_COMPILER=${LLVM_PREFIX_PATH}/bin/clang++ \
            -DLLVM_ENABLE_ASSERTIONS=ON \
            -DLLVM_DIR=${LLVM_DIR} \
            -DMLIR_DIR=${MLIR_DIR}
        ;;

    build)
        echo "[x] Building project"
        cmake --build ${BUILD_DIR} # ninja -C ${BUILD_DIR}
        ;;

    rebuild)
        echo "[x] Rebuilding project from scratch"
        rm -rf ${BUILD_DIR}
        $0 config
        $0 build
        ;;

    check-all)
        echo "[x] Running all tests"
        cmake --build ${BUILD_DIR} --target check-all
        ;;

    check-eter)
        echo "[x] Running the lit test suite"
        cmake --build ${BUILD_DIR} --target check-eter
        ;;

    test)
        echo "[x] Running the unit tests"
        cmake --build ${BUILD_DIR} --target test
        ;;

    doc-doxygen)
        echo "[x] Generating documentation using Doxygen"
        cmake --build ${BUILD_DIR} --target doc-doxygen
        ;;

    run-lit-test)
        if [ -z "${1:-}" ]; then
            echo "[x] Error: Missing path argument for 'test' command."
            print_usage
            exit 1
        fi
        echo "[x] Running lit tests"
        INPUT_FOLDER=$1
        ${LIT_COMMAND} -a -s -v "${INPUT_FOLDER}"
        ;;

    run-unit-test)
        if [ -z "${1:-}" ]; then
            echo "[x] Error: Missing binary name for 'run-unit-test' command."
            print_usage
            exit 1
        fi
        TEST_BINARY=$1
        echo "[x] Running unit test binary: ${TEST_BINARY}"
        "${BUILD_DIR}/unittests/${TEST_BINARY}"
        ;;

    run-single-test)
        if [ -z "${1:-}" ] || [ -z "${2:-}" ]; then
            echo "[x] Error: Missing arguments for 'run-single-test' command."
            print_usage
            exit 1
        fi
        TEST_BINARY=$1
        TEST_FILTER=$2
        echo "[x] Running single test: ${TEST_FILTER} in ${TEST_BINARY}"
        "${BUILD_DIR}/unittests/${TEST_BINARY}" --gtest_filter="${TEST_FILTER}"
        ;;

    check-clang-format)
        CLANG_FORMAT_COMMIT=${1:-$CLANG_FORMAT_COMMIT}
        echo "[x] Checking clang-format against git commit ${CLANG_FORMAT_COMMIT}"
        # git diff --cached --name-only --diff-filter=ACMR | grep -E '\.(c|cc|cpp|cxx|h|hh|hpp|hxx)$' | xargs -r clang-format --dry-run --Werror
        # clang-format --dry-run --Werror $(git diff --cached --name-only --diff-filter=ACMR | grep -E '\.(c|cc|cpp|cxx|h|hh|hpp|hxx)$')
        format_diff=$(git clang-format --diff ${CLANG_FORMAT_COMMIT} 2>&1)
        echo "$format_diff"
        if echo "$format_diff" | grep -q "^[-+]"; then
          exit 1
        fi
        ;;

    clang-format)
        CLANG_FORMAT_COMMIT=${1:-$CLANG_FORMAT_COMMIT}
        echo "[x] Running clang-format against git commit ${CLANG_FORMAT_COMMIT}"
        git clang-format ${CLANG_FORMAT_COMMIT}
        ;;

    clang-tidy)
        CLANG_TIDY_COMMIT=${1:-$CLANG_TIDY_COMMIT}
        echo "[x] Running clang-tidy against git commit ${CLANG_TIDY_COMMIT}"
        # Get the list of changed files
        CHANGED_FILES=$(git diff --cached --name-only --diff-filter=ACMR ${CLANG_TIDY_COMMIT} | grep -E '\.(c|cc|cpp|cxx)$' || true)
        if [ -z "$CHANGED_FILES" ]; then
            echo "[x] No C/C++ files changed compared to ${CLANG_TIDY_COMMIT}. Skipping clang-tidy."
            exit 0
        fi
        echo "[x] Running clang-tidy on the following files:"
        echo "$CHANGED_FILES"
        # Run clang-tidy on the changed files
        clang-tidy $CHANGED_FILES -p ${BUILD_DIR} --header-filter='.*' --warnings-as-errors='*'
        ;;

    tidy-all)
        echo "[x] Running clang-tidy on all C/C++ files in the repository"
        # Find all source files, excluding the build directory
        ALL_FILES=$(find . -type f -not -path "*/${BUILD_DIR#./}/*" -not -path "*/.git/*" -not -path "*/docsrc/*" -not -path "*/docs/*" \( -name "*.cpp" -o -name "*.c" -o -name "*.cc" -o -name "*.cxx" \))
        if [ -z "$ALL_FILES" ]; then
            echo "[x] No C/C++ files found."
            exit 0
        fi
        # Run clang-tidy
        clang-tidy $ALL_FILES -p ${BUILD_DIR} --header-filter='.*' --warnings-as-errors='*'
        ;;

    check-commit-clang-format)
        COMMIT=${1:-HEAD}
        echo "[x] Checking clang-format of commit ${COMMIT}"
        git clang-format --diff "${COMMIT}~1" "${COMMIT}"
        ;;

    check-commit-clang-tidy)
        COMMIT=${1:-HEAD}
        echo "[x] Checking clang-tidy of commit ${COMMIT}"
        # Get the list of files changed in the specific commit
        CHANGED_FILES=$(git diff-tree --no-commit-id --name-only -r "${COMMIT}" --diff-filter=ACMR | grep -E '\.(c|cc|cpp|cxx)$' || true)
        if [ -z "$CHANGED_FILES" ]; then
            echo "[x] No C/C++ files changed in ${COMMIT}. Skipping clang-tidy."
            exit 0
        fi
        echo "[x] Running clang-tidy on the following files (current version):"
        echo "$CHANGED_FILES"
        # Run clang-tidy on the files (uses current working directory versions)
        clang-tidy $CHANGED_FILES -p ${BUILD_DIR} --header-filter='.*'
        ;;

    create-clangd)
        echo "[x] Creating .clangd configuration file"
        cat > .clangd <<EOL
CompileFlags:
  CompilationDatabase: ${BUILD_DIR}/
EOL
        echo "[x] .clangd file created with compilation database path: ${BUILD_DIR}/"
        ;;

    show-config)
        echo "[x] Starting with the following configuration:"
        echo "[x] PATH: ${PATH}"
        echo "[x] CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH:-<not set>}"
        echo "[x] LD_LIBRARY_PATH: ${LD_LIBRARY_PATH:-<not set>}"
        echo "[x] LDFLAGS: ${LDFLAGS:-<not set>}"
        echo "[x] CPPFLAGS: ${CPPFLAGS:-<not set>}"
        echo "[x] LLVM_PREFIX_PATH: ${LLVM_PREFIX_PATH}"
        echo "[x] LLVM_DIR: ${LLVM_DIR}"
        echo "[x] MLIR_DIR: ${MLIR_DIR}"
        echo "[x] LIT_COMMAND: ${LIT_COMMAND}"
        echo "[x] BUILD_DIR: ${BUILD_DIR}"
        echo "[x] CLANG_FORMAT_COMMIT: ${CLANG_FORMAT_COMMIT}"
        echo "[x] CLANG_TIDY_COMMIT: ${CLANG_TIDY_COMMIT}"
        ;;

    clean)
        echo "[x] Cleaning build directory"
        rm -rf ${BUILD_DIR}
        ;;

    help|*)
        print_usage
        ;;

    # update-test-checks)
    #     if [ -z "${1:-}" ]; then
    #         echo "[x] Error: Missing input file for 'update-test-checks' command."
    #         print_usage
    #         exit 1
    #     fi
    #     INPUT_FILE=$1
    #     echo "[x] Updating FileCheck patterns in ${INPUT_FILE}"
    #     python3 mlir/utils/update_test_checks.py \
    #         --opt-binary=${BUILD_DIR}/bin/mlir-opt \
    #         ${INPUT_FILE}
    #     ;;
esac

echo "[x] Done."
