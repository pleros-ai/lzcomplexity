# Install

*Four install paths, two standalone binaries, and a verification snippet that takes ten seconds.*

Most people want `pip install lzcomplexity`. It pulls a prebuilt wheel with the Rust core already
compiled in — no toolchain, no compiler, no configuration. The other three paths exist for people
building from source, hacking on the Rust, or calling the core crate from Rust directly.

<div class="lz-stats">
  <div class="lz-stat"><span class="lz-stat__v">5</span><span class="lz-stat__k">platform wheels</span></div>
  <div class="lz-stat"><span class="lz-stat__v">3.9</span><span class="lz-stat__k">minimum Python</span></div>
  <div class="lz-stat"><span class="lz-stat__v">0</span><span class="lz-stat__k">runtime Python deps</span></div>
  <div class="lz-stat"><span class="lz-stat__v">1.75</span><span class="lz-stat__k">minimum Rust</span></div>
  <div class="lz-stat"><span class="lz-stat__v">2</span><span class="lz-stat__k">CLI binaries</span></div>
</div>

## Pick a path

=== "pip (PyPI)"

    ```bash
    pip install lzcomplexity
    ```

    Five platform wheels plus one source distribution are published per release. These are the
    files actually on PyPI for 1.0.1:

    | Wheel tag | Platform | Size |
    |---|---|---|
    | `cp39-abi3-manylinux_2_17_x86_64.manylinux2014_x86_64` | Linux x86-64, glibc ≥ 2.17 | 301.7 kB |
    | `cp39-abi3-manylinux_2_17_aarch64.manylinux2014_aarch64` | Linux aarch64, glibc ≥ 2.17 | 287.9 kB |
    | `cp39-abi3-macosx_10_12_x86_64` | macOS 10.12+, Intel | 276.1 kB |
    | `cp39-abi3-macosx_11_0_arm64` | macOS 11+, Apple Silicon | 264.6 kB |
    | `cp39-abi3-win_amd64` | Windows x86-64 | 203.5 kB |
    | `lzcomplexity-1.0.1.tar.gz` | source distribution | 33.8 kB |

    The wheel tag is `cp39-abi3`, not `cp39`/`cp310`/`cp311`/…. The extension is built against
    CPython's stable ABI (PyO3's `abi3-py39` feature), so **one wheel per platform serves every
    CPython from 3.9 upward**. The package declares `Requires-Python: >=3.9` and carries no
    `Requires-Dist` line at all — installing it adds nothing else to your environment.

    **On a platform not in that table**, pip falls back to the sdist and compiles the Rust core
    locally, which needs a Rust toolchain *and* a C compiler (see the next tab). The gaps are:
    Linux 32-bit, armv7 and musl; Windows ARM64 and 32-bit; macOS older than 10.12.

    !!! tip

        The `cp39-abi3` extension loads unmodified on CPython 3.14.6 — five minor versions past the
        one it is tagged for. That is the interpreter every measured example in these docs was
        produced on.

=== "From source (pip)"

    ```bash
    git clone https://github.com/pleros-ai/lzcomplexity.git
    cd lzcomplexity
    git checkout rust-backend
    pip install .
    ```

    The build backend is maturin (`maturin>=1.7,<2`, declared in `pyproject.toml`). pip fetches it
    into an isolated build environment automatically; you do not need to install maturin yourself
    on this path.

    This is a mixed Python/Rust project: the pure-Python skin comes from `python/lzcomplexity/`
    (`__init__.py`, `__init__.pyi`, `py.typed`) and the compiled `lzcomplexity.abi3.so` — 590 kB,
    stripped by maturin — is dropped inside it. The resulting wheel is about 300 kB.

    !!! warning

        Cloning without `git checkout rust-backend` gets you a different library. The repository's
        default branch, `main`, still holds the previous C++ implementation — CMake, git
        submodules, `installAll.sh`. Every code path documented on this site lives on
        `rust-backend`, which is also the only branch the release pipeline publishes from.

    **You need a C compiler as well as a Rust toolchain.** The suffix-array dependency
    `cdivsufsort` compiles bundled C sources through a `cc` build-dependency. On Windows that means
    the MSVC build tools. The README's line *"No CMake, no submodules, no C++ toolchain"* is true
    as written — no **C++** toolchain — but a C compiler is not optional.

=== "Dev loop (maturin)"

    ```bash
    python3 -m venv .venv && source .venv/bin/activate
    pip install maturin
    maturin develop --release
    ```

    This compiles the extension and installs the package into the active virtualenv, dropping
    `lzcomplexity.abi3.so` into `python/lzcomplexity/` (git-ignored). Re-run it after every Rust
    change.

    `import lzcomplexity` from the repository root does **not** work, and is not meant to:
    `python/` is the `python-source` root, so the importable package is `python/lzcomplexity`.
    Use the virtualenv install, run from inside `python/`, or set `PYTHONPATH=python`.

    !!! warning

        Timings taken without `--release` are meaningless — a debug-profile core is orders of
        magnitude slower than the shipped one, and plain `maturin develop` builds exactly that.
        The release profile the wheels use is `lto = "thin"`, `codegen-units = 1`, `opt-level = 3`,
        set once at the workspace root. See [Performance](../project/performance.md).

=== "Rust crate"

    ```bash
    cargo add lzcomplexity-core
    ```

    or, in `Cargo.toml`:

    ```toml
    [dependencies]
    lzcomplexity-core = "1.0"
    ```

    `lzcomplexity-core` is the only crate published to crates.io — it holds the whole algorithm.
    `lzcomplexity-py` (the PyO3 bindings) and `lzcomplexity-cli` (the two binaries) are workspace
    members that ship inside the wheel and as release assets respectively; neither is on crates.io.

    Same C-compiler requirement as any source build: `cdivsufsort` compiles bundled C. See the
    [Rust crate reference](../api/rust.md).

<hr class="lz-tickrule">

## Minimum supported Rust version

MSRV is **1.75**, edition 2021. It is declared once at the workspace root, inherited by all three
crates, and recorded on crates.io for both published versions of `lzcomplexity-core` (1.0.0 and
1.0.1).

!!! note

    Treat 1.75 as a declaration, not a tested guarantee. Every CI job builds with
    `dtolnay/rust-toolchain@stable` and there is no 1.75 job, so nothing would catch an accidental
    MSRV bump introduced by a dependency or a new language feature. If you are pinned to an old
    toolchain, verify the build yourself.

## The standalone binaries

Two command-line tools, `lzcomplexity` and `lzdistance`, live in the `lzcomplexity-cli` crate.
Neither is on crates.io and neither is part of the Python wheel. The
[CLI overview](../cli/index.md) covers what they do.

### Build them yourself

```bash
cargo build --release -p lzcomplexity-cli
```

That produces `target/release/lzcomplexity` and `target/release/lzdistance`:

<div class="lz-run" markdown>

```console
$ ls -lh target/release/lzcomplexity target/release/lzdistance
-rwxr-xr-x 2 user user 1.3M Jul 25 17:34 target/release/lzcomplexity
-rwxr-xr-x 2 user user 1.4M Jul 25 17:34 target/release/lzdistance
```

</div>

The release profile does not set `strip`, so the binaries ship unstripped. Exact byte counts move
with the toolchain version; the released Linux x86-64 assets for `v1.0.1` are 1,347,952 and
1,418,088 bytes.

### Prebuilt release assets

Each release builds **8 binaries** — two tools across four targets — named
`<bin>-<tag>-<suffix>[.exe]`. For `v1.0.1`:

<div class="lz-scroll" markdown>

| Target triple | Suffix | Assets |
|---|---|---|
| `x86_64-unknown-linux-gnu` | `linux-x86_64` | `lzcomplexity-v1.0.1-linux-x86_64`, `lzdistance-v1.0.1-linux-x86_64` |
| `aarch64-apple-darwin` | `macos-arm64` | `lzcomplexity-v1.0.1-macos-arm64`, `lzdistance-v1.0.1-macos-arm64` |
| `x86_64-apple-darwin` | `macos-x86_64` | `lzcomplexity-v1.0.1-macos-x86_64`, `lzdistance-v1.0.1-macos-x86_64` |
| `x86_64-pc-windows-msvc` | `windows-x86_64` | `lzcomplexity-v1.0.1-windows-x86_64.exe`, `lzdistance-v1.0.1-windows-x86_64.exe` |

</div>

There is no Linux aarch64 binary and no musl build. Use `cargo build` on those platforms.

!!! note

    If a release page shows no binaries, look at the workflow run instead. The pipeline always
    uploads them as workflow artifacts named `binaries-<suffix>`, then attaches them to the release
    on a best-effort basis: with "immutable releases" enabled on the organisation,
    `gh release upload` returns HTTP 422 and the step degrades to a warning rather than failing —
    so for some versions the binaries may exist only as artifacts. On `v1.0.0` and `v1.0.1` the
    attach succeeded and all eight assets are on both release pages.
    [Releases](../project/releases.md) walks through the whole pipeline.

<hr class="lz-tickrule">

## Verify your install

Print the version and check a known factorization. `"banana"` gives 3 complete components with
boundaries `[0, 1, 2, 3, 7]` — the same assertion the project's CI runs against every wheel it
builds.

```python title="verify.py"
import lzcomplexity as lz

print("lzcomplexity", lz.__version__)
print("factorization('banana') ->", lz.factorization("banana"))
assert lz.factorization("banana") == (3, [0, 1, 2, 3, 7])
print("OK")
```

<div class="lz-run" markdown>

```console
$ python3 verify.py
lzcomplexity 1.0.0
factorization('banana') -> (3, [0, 1, 2, 3, 7])
OK
```

</div>

That transcript is from a locally built 1.0.0. A fresh `pip install lzcomplexity` today reports
`1.0.1`, which is a documentation-only patch — it adds a README to the core crate and changes no
code. The version line is the only thing that differs.

!!! note

    The last boundary, `7`, sits past the end of a 6-symbol string, and that is correct.
    `complexity` counts only **complete** LZ76 components — the trailing component runs off the end
    of the sequence and is not counted. The library's count is therefore one less than the textbook
    exhaustive-history count whenever a sequence ends mid-component, which is the common case:
    3 rather than 4 for `"banana"`. [Your first factorization](first-factorization.md) walks the
    boundary list symbol by symbol.

Both binaries answer `--version` and exit 0 (again, a locally built 1.0.0):

<div class="lz-run" markdown>

```console
$ ./target/release/lzcomplexity --version
 [ Info ] v1.0.0
$ ./target/release/lzdistance --version
 [ Info ] Version of lzdistance: v1.0.0
```

</div>

The two tools word that line differently. It is a cosmetic inconsistency in the CLI, not a sign of
a mismatched build — both binaries carry the same workspace version.

## Next

<div class="lz-cards" markdown>
<div class="lz-card" markdown>

### Your first factorization

Factorise a sequence, read the boundary list, and see what a factor actually is.

<p class="lz-card__api"><code>lz.factorization(seq)</code></p>

[Start here](first-factorization.md)

</div>
<div class="lz-card" markdown>

### Reading the numbers

What `complexity`, `h`, `emc` and `nid` mean, and which of them your data supports.

<p class="lz-card__api"><code>lz.lz76(seq)</code></p>

[Read the guide](reading-the-numbers.md)

</div>
<div class="lz-card" markdown>

### Python API

All five functions and every parameter, including the two that are accepted and ignored.

<p class="lz-card__api"><code>import lzcomplexity as lz</code></p>

[Open the reference](../api/python.md)

</div>
<div class="lz-card" markdown>

### Command line

`lzcomplexity` and `lzdistance`: input formats, flags, JSON output.

<p class="lz-card__api"><code>lzcomplexity seq.txt -o out.json</code></p>

[See the tools](../cli/index.md)

</div>
</div>
