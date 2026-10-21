# Install lzcomplexity

Use the Python package for arrays and notebooks, the command-line tools for files and batches, or the Rust crate inside an application. All three current interfaces use the same Rust core.

## Python: the quickest start

You need **Python 3.9 or newer**. Create an environment and install the package:

=== "macOS / Linux"

    ```sh
    python3 -m venv .venv
    source .venv/bin/activate
    python -m pip install --upgrade pip
    python -m pip install lzcomplexity
    ```

=== "Windows PowerShell"

    ```powershell
    py -m venv .venv
    .venv\Scripts\Activate.ps1
    python -m pip install --upgrade pip
    python -m pip install lzcomplexity
    ```

Check the interpreter and package you actually installed:

```python
import sys
import lzcomplexity as lz

print(sys.executable)
print(lz.__version__)
print(lz.factorization("banana"))
# (3, [0, 1, 2, 3, 7])
print(lz.h("01010101", log_base=2))
# 0.75
```

The Python package itself does not require NumPy. NumPy, SciPy and plotting tools are optional parts of your own preprocessing or analysis workflow.

!!! note "Published release or current source?"

    The published release checked for this documentation is **2.0.0**. Its [PyPI files](https://pypi.org/project/lzcomplexity/2.0.0/#files) include ABI3 wheels for Linux x86-64/AArch64, macOS Intel/Apple Silicon and Windows x86-64. Wheel availability depends on your interpreter and platform. A matching wheel installs without a Rust toolchain; otherwise pip may attempt a source build.

    The `rust-backend` branch can contain fixes newer than that release, including PR #14's alphabet-cache and CLI-unit corrections. These docs describe the current source. Installing from source below is the route to those changes until the next package release. `__version__` alone does not identify an unreleased source checkout: record its Git revision too.

Next: [your first factorization](first-factorization.md), [reading the numbers](reading-the-numbers.md), or the [Python reference](../api/python.md).

## Install the current source

A source build needs **Rust 1.85 or newer**, Cargo, a C compiler, and Python 3.9+. The C compiler builds the suffix-array dependency; this is not a CMake project.

1. Install the Rust toolchain using [rustup](https://rustup.rs/).
2. Install the native C build tools for your platform: Xcode Command Line Tools on macOS, a C compiler and development tools on Linux, or Visual Studio C++ Build Tools for an MSVC Windows toolchain.
3. Clone the production branch and install:

```sh
git clone --branch rust-backend https://github.com/pleros-ai/lzcomplexity.git
cd lzcomplexity
python -m pip install .
git rev-parse HEAD
```

Use the virtual environment from the previous section. Pip uses the maturin backend specified in `pyproject.toml` to build the native extension. CMake, nanobind, CaPS and oneTBB instructions in the [historical manual](../project/manual.md) apply to the older C++ implementation.

For a reproducible study, check out a specific tag or commit before installing and record it with your analysis settings. `pip install .` installs a built copy; changing Rust source later does not update an already installed extension.

## Command-line tools

### Download a release binary

The [GitHub releases](https://github.com/pleros-ai/lzcomplexity/releases) contain standalone `lzcomplexity` and `lzdistance` binaries. Select the operating system and CPU architecture for your machine. They do not need Python.

Release 2.0.0 has binary assets for Linux x86-64, macOS arm64/x86-64 and Windows x86-64. Download each tool you need. On macOS/Linux, make the downloaded file executable and either call it by its path or place it in a directory on your `PATH`:

```sh
chmod +x lzcomplexity-v2.0.0-macos-arm64
./lzcomplexity-v2.0.0-macos-arm64 --version
```

The filename above is the Apple Silicon example; use the asset matching your machine. Downloaded release binaries contain the code at that release, not later branch fixes.

### Build the current binaries

From a clone of `rust-backend`, with Rust and a C compiler installed:

```sh
cargo build --release -p lzcomplexity-cli --bins --locked
./target/release/lzcomplexity --version
./target/release/lzdistance --version
```

Windows places `lzcomplexity.exe` and `lzdistance.exe` in `target\release\`. Add that directory to your `PATH` or use the full paths in the CLI examples.

A first file analysis:

```sh
printf '01010101\n' > sequence.txt
./target/release/lzcomplexity sequence.txt -F text -l 2 -o report.json
```

See the [CLI overview](../cli/index.md) and [input formats](../cli/formats.md) before processing a dataset. Explicit format selection is useful when a short file could be misclassified.

## Rust applications

The core crate has no Python or CLI dependency. The [Rust reference](../api/rust.md) covers `Sequence`, argument structures, results and the direct algorithms. A C compiler is still needed for its suffix-array dependency.

## Work on the Python extension

For development, install maturin inside your environment and build in release mode:

```sh
python -m pip install 'maturin>=1.7,<2'
maturin develop --release
```

Repeat `maturin develop --release` after Rust changes. To produce a wheel for another environment on the same supported platform:

```sh
maturin build --release --out dist
python -m pip install --force-reinstall dist/<the-built-wheel>.whl
```

Maturin prints the actual wheel filename. A compiled wheel targets its platform; it is not a portable replacement for the source tree.

If installation fails, start with [troubleshooting](troubleshooting.md). Release automation and the distinction between branch fixes, tags and published packages are explained in [versions and releases](../project/releases.md).
