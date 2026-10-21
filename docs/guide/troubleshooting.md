# Troubleshooting

Start by identifying the installed package, interpreter and input representation. Most discrepancies arise before the LZ calculation: a different wheel, a source checkout that was not rebuilt, a changed encoding or a different normalization default.

## Installation and imports

### `ModuleNotFoundError: lzcomplexity`

Install into the interpreter that runs the script:

```sh
python -m pip show lzcomplexity
python -m pip install lzcomplexity
python -c 'import sys, lzcomplexity as lz; print(sys.executable, lz.__version__, lz.__file__)'
```

In a notebook, check `sys.executable` in the active kernel. Installing through a different terminal's `pip` can target another environment.

### Pip tries to compile or cannot build a wheel

Your interpreter/platform may lack a compatible prebuilt wheel. Check [PyPI's files](https://pypi.org/project/lzcomplexity/#files), then either select a supported Python/platform combination or install the source build requirements: Rust 1.85+, Cargo and a native C compiler. See [install](install.md).

Do not apply the historical manual's CMake or oneTBB instructions to the current Rust checkout.

### Source changes do not affect Python results

An installed wheel is a compiled copy. Editing `.rs` files does not modify it. Rebuild and reinstall from the intended checkout:

```sh
python -m pip install --force-reinstall .
```

For development inside an environment containing maturin, use `maturin develop --release`. Check `lz.__file__` and the Git revision again.

### Old Python examples fail to unpack results

The current API has five functions. `factorization()` returns a pair, `emc()` returns a pair, and `lz76()` returns a dictionary. `entropy_density()` was replaced by `h()`. Use the [migration table](../project/cpp-parity.md) or [Python reference](../api/python.md).

## Numbers and input data

### My integer list has the wrong length or alphabet

Integer sequences are joined through decimal text: `[0, 1, 10]` becomes `"0110"`. Use `bytes([0, 1, 10])` when each integer from 0 through 255 should be one symbol. Floating-point samples must be symbolized deliberately before analysis.

A Unicode string is encoded as UTF-8, so one visible character can occupy multiple input bytes. Normalize/encode your data consistently; see [input types](../api/inputs.md).

### The reported factor count differs from the number of displayed pieces

The factor count and the boundary vector follow the current implementation's complete-component and trailing-component conventions. A final boundary can be an end sentinel beyond the input length. Slice with clamped bounds and use the returned count as the library's value. The [factorization page](../concepts/lz76.md) explains the `banana` example.

### `h` is greater than one

One is not a universal upper limit for a finite LZ estimate. The log base determines units, the alphabet determines the corresponding asymptotic reference, and short-sequence bias can exceed an asymptotic bound. Check the observed bytes, length and explicit `log_base` before interpreting a value. Do not clip an estimate just to fit a plot. See [entropy density](../concepts/entropy-density.md).

### Setting `alphabet=4` did not change the units

In Python/core, an explicit `alphabet` affects the `epsilon` diagnostic, while the default log base still follows the observed alphabet. Set `log_base` explicitly. The CLI has a legacy default using an explicitly declared alphabet; pass `-l 2` and Python `log_base=2` when comparing them.

### CLI `-l` appears to be ignored

The current source corrects the entropy-density report's handling of explicit logarithm units. A released executable built before PR #14 can still exhibit the old behavior. Check the executable's provenance, not just a source tree elsewhere on the machine. See [versions](../project/releases.md).

### The shuffle score is zero for a constant or tiny input

A constant sequence is unchanged by every permutation. Inputs of at most ten bytes are also unchanged by the released sampler. Their zero contrast follows from this procedure; it does not assert that a constant pattern lacks structure. See [the EMC-like measure](../concepts/emc.md).

### A longer sample or different cap changes the score

Input length changes finite-sample LZ behavior and can change the automatic scale cutoff. An explicit cap changes which contrasts enter the fitted profile. Cyclic shifts and symbol relabeling also change the content-derived random seed. Compare matched settings and check their sensitivity; a larger score is not a universal ranking by theoretical EMC.

### The score differs from the manual's figure

Check the software revision, aggregation definition, sequence generation, length, preprocessing, cap and scaling used in that figure. Historical normalized curves are qualitative comparisons from particular experiments, not exact expected outputs for every release. Start with the [manual coverage guide](../project/manual.md).

### NID is nonzero on the diagonal or looks unlike an edit distance

This is a finite LZ-based operational distance. It is not guaranteed to satisfy the exact metric properties of theoretical normalized information distance. It compares literal patterns through concatenation counts, not character substitutions or an alignment. See [information distance](../concepts/nid.md).

## File processing and execution

### The CLI processes only one line

For raw text, single-sequence mode reads the first nonblank line rather than joining the entire file. Use `lzcomplexity input.txt -F text -m` for line-wise input. `lzdistance` file sources use multiline reading. A directory source instead takes the first sequence from each sorted input file.

### A CSV header or binary file changes results unexpectedly

The CLI's readers have specific legacy behavior: CSV/TSV columns are concatenated as strings, headers are not automatically removed, and raw binary input is expanded into a bitstream with its own tail convention. PNM readers also flatten or split data according to their mode. Read [input formats](../cli/formats.md), and preprocess explicitly when those conventions do not match your data.

### Many matrix cells are zero after selecting a range

`lzdistance -i` and `-s` gate computations without shrinking dimensions. Gated-out cells remain zero in the JSON. Preserve the selected ranges and a mask separately; a zero alone does not identify whether a cell was computed.

### `jobs=1` does not change Python worker usage

The Python keyword is currently reserved and ignored. Set `RAYON_NUM_THREADS` before the first native call or launch a fresh process with the desired setting. The CLI's `-j` configures its own worker pool. See [execution](../project/determinism.md).

### A Python thread pool does not accelerate the analysis

The current extension retains the GIL while it computes. Rayon can use native workers inside one call, but Python threads do not make multiple binding calls execute independently. Consider process-based batches for sufficiently large jobs and budget their native worker counts; measure the full workload first.

## Report a reproducible issue

Use the [issue tracker](https://github.com/pleros-ai/lzcomplexity/issues). Include the package and source revision, platform/interpreter, a minimal input or generation script, explicit arguments, the observed output and the expected behavior. For file problems, include the format and the exact reader command. Avoid posting private datasets; a small synthetic input that reproduces the behavior is more useful.
