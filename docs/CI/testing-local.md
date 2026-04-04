# Testing CI Locally

This guide explains how to test Quar's CI workflows locally, simulating the GitHub Actions environment.

## 1. Prerequisites

- **Docker** installed and running (for container-based workflows)
- **Python 3** and **pip** (to install `act` and other tools)
- **act** ([nektos/act](https://github.com/nektos/act)) — a tool to run GitHub Actions workflows locally
  - Quick install:
    ```sh
    brew install act # macOS
    # or
    pip install act # on Linux (if available)
    # or
    curl -s https://raw.githubusercontent.com/nektos/act/master/install.sh | bash
    ```
- **Disk space**: workflows that download LLVM require ~2GB of free space

## 2. Running a workflow

From the repository root, run:

```sh
act -j build -W .github/workflows/ubuntu-x86.yml
```

- `-j build` runs only the `build` job (all our workflows have a single job)
- `-W <file>` specifies the workflow file to test

To test a container-based workflow (e.g., Fedora):

```sh
act -j build -W .github/workflows/fedora-x86.yml
```

## 3. Limitations & notes

- Some GitHub Actions runners (e.g., `macos-14`, `ubuntu-24.04-arm`) **cannot be simulated** locally: you can only test workflows using `ubuntu-latest` or Docker containers.
- The containers used in workflows must be available on Docker Hub.
- Environment variables and paths are almost identical to GitHub Actions, but there may be minor differences.
- To test a specific matrix combination, you can temporarily edit the workflow file to force only the desired combination.

## 4. Advanced debugging

- Add `-v` for verbose output:
  ```sh
  act -j build -W .github/workflows/arch-x86.yml -v
  ```
- To simulate a different event (e.g., `pull_request`):
  ```sh
  act pull_request -j build -W .github/workflows/ubuntu-x86.yml
  ```

## 5. Useful resources

- [nektos/act GitHub](https://github.com/nektos/act)
- [GitHub Actions documentation](https://docs.github.com/en/actions)

---

**Tip:** Before pushing workflow changes, test them locally with `act` if possible. This reduces trial-and-error and speeds up development.