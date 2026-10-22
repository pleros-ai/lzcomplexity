# Troubleshooting

This page covers installation, input representation, numerical results and batch execution.

## Installation and imports

### `ModuleNotFoundError: lzcomplexity`

Install into the interpreter that runs your script:

```sh
python -m pip show lzcomplexity
python -m pip install lzcomplexity
python -c 'import sys, lzcomplexity as lz; print(sys.executable, lz.__version__, lz.__file__)'
```

In a notebook, check `sys.executable` in the active kernel. A terminal's `pip` may target a different environment.

### Pip tries to compile or cannot build a wheel

If a compatible wheel is unavailable, pip builds the extension from source. Install Rust 1.85+, Cargo and a native C compiler, then retry. The [installation guide](install.md) lists the build requirements; [PyPI](https://pypi.org/project/lzcomplexity/#files) lists downloadable wheels.

### Source changes do not affect Python results

Rebuild and reinstall the extension after changing Rust source:

```sh
python -m pip install --force-reinstall .
```

For development with maturin, use `maturin develop --release`. Check `lz.__file__` to confirm which installation Python imports.

### Unpacking a result raises an error { #old-python-examples-fail-to-unpack-results }

`factorization()` returns `(count, boundaries)`, `emc()` returns `(E, summands)`, and `lz76()` returns a dictionary. Use `h()` to obtain entropy density directly. See the [Python reference](../api/python.md) for return types.

## Numbers and input data

### My integer list has the wrong length or alphabet

Integer sequences are joined through decimal text: `[0, 1, 10]` becomes `"0110"`. Use `bytes([0, 1, 10])` when each integer from 0 through 255 should represent one symbol.

Unicode strings use UTF-8, so one visible character may occupy several input bytes. Choose an encoding and apply it consistently; see [input types](../api/inputs.md).

### The factor count differs from the number of displayed pieces

A final copied tail appears in the boundary vector but is excluded from the completed-phrase count. Clamp each end position to the input length when displaying pieces. The [factorization page](../concepts/lz76.md) explains the `banana` example.

### h is greater than one

Short-sequence estimates can exceed the limiting entropy bound. Check the byte representation, sample length and `log_base`, then examine how the result changes with longer samples. See [entropy density](../concepts/entropy-density.md).

### Setting `alphabet=4` did not change the units

Python's `alphabet` argument controls the finite-size `epsilon` diagnostic. Set `log_base` to choose entropy units. For base-two results, use Python `log_base=2` or CLI `-l 2`. The [alphabet guide](../concepts/alphabets.md) explains the defaults.

### E is zero for a constant or very short input { #the-shuffle-score-is-zero-for-a-constant-or-tiny-input }

E measures organization through its response across ranges. A constant sequence has a simple pattern, but no variation for this calculation to distinguish. Inputs of at most ten bytes also return zero. Try a longer sequence and consult [structural complexity E](../concepts/emc.md).

### A longer sample or different cap changes E { #a-longer-sample-or-different-cap-changes-the-score }

Length affects the factorization and the automatic range selection. Set `max_block_size` explicitly when comparing samples, use equal lengths where possible, and check the profile as well as the final E value. See [length and convergence](../concepts/convergence.md).

### NID is nonzero on the diagonal or looks unlike an edit distance

NID compares recurring substrings through concatenation counts. A finite sequence can add a new phrase at the join when concatenated with itself, producing a nonzero diagonal. See [information distance](../concepts/nid.md) for the formula and examples.

## File processing and execution

### The CLI processes only one line

Text mode reads the first nonblank line unless multiline analysis is enabled. Use `lzcomplexity input.txt -F text -m` to process each line. `lzdistance` reads file sources as multiple sequences; a directory source contributes the first sequence from each sorted file.

### A CSV header or binary file changes results unexpectedly

CSV columns are concatenated as text, including headers. Raw binary files are expanded into bits using the reader's length convention. Check [input formats](../cli/formats.md) and inspect sequence lengths before processing a dataset.

### Many matrix cells are zero after selecting a range

`lzdistance -i` and `-s` select cells to compute while preserving full matrix dimensions. Cells outside those ranges remain zero. Keep the selected ranges with the output so you can distinguish skipped cells from measured zeros.

### `jobs=1` does not change Python worker usage

The Python `jobs` argument has no effect. Set `RAYON_NUM_THREADS` before the first native call, or start a fresh process with the desired setting. The CLI's `-j` configures its worker pool. See [parallel execution](../project/determinism.md).

### A Python thread pool does not accelerate the analysis

The extension holds the GIL during computation. Rayon workers can run inside a call; use processes for independent Python analyses. Budget the native worker count per process and benchmark a representative batch.

## Report an issue { #report-a-reproducible-issue }

Open an [issue](https://github.com/pleros-ai/lzcomplexity/issues) with a small reproducible input, your command or Python call, the output, and the expected behavior. Include the package version and operating system. For file-reader problems, include the format selector and a small sample file.
