# Install lzcomplexity

Use the Python package in scripts and notebooks, the command-line tools for files and batches, or the Rust crate inside an application.

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

Try your first calculation:

```python
import lzcomplexity as lz

print(lz.factorization("banana"))
# (3, [0, 1, 2, 3, 7])
print(lz.h("01010101", log_base=2))
# 0.75
```

Continue with [your first factorization](first-factorization.md), [reading the numbers](reading-the-numbers.md), or the [Python reference](../api/python.md).

## Build from source { #install-the-current-source }

A source build needs **Rust 1.85 or newer**, Cargo, a C compiler, and Python 3.9+. Install Rust with [rustup](https://rustup.rs/) and the C build tools for your platform: Xcode Command Line Tools on macOS, your distribution's development tools on Linux, or Visual Studio C++ Build Tools on Windows.

With your Python environment active:

```sh
git clone https://github.com/pleros-ai/lzcomplexity.git
cd lzcomplexity
python -m pip install .
```

Pip uses maturin to compile the native extension and install the package. Reinstall after changing Rust source, or use the development workflow below.

## Command-line tools

### Download a release binary

Download `lzcomplexity` and `lzdistance` from the [GitHub releases](https://github.com/pleros-ai/lzcomplexity/releases), choosing the files for your operating system and CPU architecture. The standalone tools run without Python.

On macOS/Linux, rename the downloaded files to `lzcomplexity` and `lzdistance`, then make them executable:

```sh
chmod +x lzcomplexity lzdistance
./lzcomplexity --version
./lzdistance --version
```

Place them in a directory on your `PATH` to call them from any working directory. On Windows, use the corresponding `.exe` files.

### Build the binaries { #build-the-current-binaries }

From a clone of the repository:

```sh
cargo build --release -p lzcomplexity-cli --bins --locked
./target/release/lzcomplexity --version
./target/release/lzdistance --version
```

Windows places the executables in `target\release\`.

A first file analysis:

```sh
printf '01010101\n' > sequence.txt
./target/release/lzcomplexity sequence.txt -F text -l 2 -o report.json
```

See the [CLI overview](../cli/index.md) for batch analysis and [input formats](../cli/formats.md) for text, FASTA, tables and images.

## Rust applications

Use the core crate to work directly with `Sequence`, analysis arguments and result structures. The [Rust reference](../api/rust.md) includes a complete example. Building the suffix-array dependency requires a C compiler.

## Work on the Python extension

Install maturin inside your environment and build the extension:

```sh
python -m pip install 'maturin>=1.7,<2'
maturin develop --release
```

Repeat `maturin develop --release` after Rust changes. To build a distributable wheel:

```sh
maturin build --release --out dist
python -m pip install --force-reinstall dist/<the-built-wheel>.whl
```

Use the wheel filename printed by maturin. If installation fails, see [troubleshooting](troubleshooting.md).
