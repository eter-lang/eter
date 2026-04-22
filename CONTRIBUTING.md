# Contributing to Eter

Thank you for your interest in contributing to Eter. Contributions of code,
documentation, tests, and bug reports are all welcome.

## Ways to Contribute

You can help the project in several ways:

- report bugs or confusing behavior
- improve documentation and examples
- add tests or developer tooling
- implement fixes, refactorings, dialects, and passes

## Before You Send a Change

Before opening a patch or pull request, please:

1. build the project locally
2. run the current regression suite
3. keep the change focused and reviewable
4. add tests when behavior changes
5. format code consistently with the project style

A typical local validation flow is:

```bash
cmake -S . -B build -G Ninja
cmake --build build
cmake --build build --target check-eter
```

## Formatting and linting manually

Before running clang-tidy, configure the build so that `build/compile_commands.json` exists:

```bash
cmake -S . -B build -G Ninja
```

Then run:

```bash
# Apply formatting automatically to the last patch using LLVM's helper
git clang-format HEAD~1
```

## Run clang-tidy on staged changes

To run clang-tidy only on files in the staged diff (useful for pre-commit checks), you can use:

```bash
clang-tidy -p build --warnings-as-errors='*' --header-filter='.*' \
$(git diff --cached --name-only --diff-filter=ACMR | grep -E '\.(c|cc|cpp|cxx|h|hh|hpp|hxx)$')
```

On macOS, where clang-tidy needs the SDK sysroot, use:

```bash
clang-tidy -p build --warnings-as-errors='*' --header-filter='.*' \
$(git diff --cached --name-only --diff-filter=ACMR | grep -E '\.(c|cc|cpp|cxx|h|hh|hpp|hxx)$') \
-- -isysroot $(xcrun --show-sdk-path)
```

These commands are safe to run locally and match what our `pre-commit` hook executes for staged C/C++ files.

If `git clang-format` is not available, install LLVM/Clang and make sure its tools are on your `PATH`.
On Linux, install the Clang tooling package for your distribution so that `clang-format`
and `git-clang-format` are available. Then rerun the command above.
If you use `git clang-format`, make sure you are on the branch with the intended diff and replace `HEAD~1` 
with the right range for your review.

Eventually, you can also run the individual tools directly, such as:

```bash
# Check C/C++ formatting without modifying files
clang-format --dry-run --Werror <files>

# Apply formatting in place
clang-format -i <files>

# Run clang-format on staged changes (dry-run)
```

```bash
clang-format --dry-run --Werror $(git diff --cached --name-only --diff-filter=ACMR | grep -E '\.(c|cc|cpp|cxx|h|hh|hpp|hxx)$')
```

# Run clang-tidy using the CMake compile commands database
clang-tidy -p build <source-files>
```


## Git Hooks

Eter includes native Git hooks under `.githooks/` so contributors can run the
basic formatting, linting, build, and test checks automatically before commit
and push operations.

Enable them with:

```bash
git config core.hooksPath .githooks
chmod +x .githooks/pre-commit .githooks/pre-push
```

You can also run the same checks manually at any time with:

```bash
.githooks/pre-commit
.githooks/pre-push
```

### `pre-commit`

The `pre-commit` hook currently checks:

- staged diff hygiene with `git diff --cached --check`
- `clang-format` for C and C++ headers/sources
- `clang-tidy -p build` for staged C and C++ source files

### `pre-push`

The `pre-push` hook currently checks:

- `cmake --build build`
- `cmake --build build --target check-eter`

Before enabling the hooks, make sure that:

- `clang-format` and `clang-tidy` are available on your `PATH`
- you have already configured the project with CMake so that
  `build/compile_commands.json` exists

## Coding Style

Eter follows LLVM-style conventions where practical:

- `clang-format` is based on LLVM style
- `clang-tidy` is configured for LLVM/MLIR-oriented development
- naming and structure should stay consistent with the surrounding code

Please avoid unrelated formatting or cleanup changes in the same patch unless
those changes are the main purpose of the review.

## Testing Expectations

Behavioral changes should come with a regression test when possible. Eter’s test
suite currently lives under `test/` and uses `lit` plus `FileCheck`.

Typical smoke tests live in:

```text
test/Smoke/*.smoke
```

and use directives such as:

```text
RUN: %eter ... | %FileCheck %s
CHECK: ...
```

## Submitting a Patch

When your change is ready:

1. create a branch for the work
2. commit the change with a clear message
3. open a pull request
4. respond to review feedback promptly and incrementally

Small, isolated patches are much easier to review than large mixed changes.

## Bug Reports and Review Context

When reporting an issue or asking for review, include as much concrete context as
possible, such as:

- operating system and compiler
- LLVM/MLIR version
- exact CMake configure command
- the failing command or test
- a minimal reproducer if available

## Licensing and Headers

New source or configuration files should include the project’s Apache 2.0 with
LLVM-exception header where appropriate.

## Questions

If something is unclear, prefer opening a discussion early rather than guessing.
Early feedback is usually cheaper than a late rewrite.
