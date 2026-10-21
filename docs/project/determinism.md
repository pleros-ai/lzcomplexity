# Determinism and execution

Identical input bytes and matched parameters produce repeatable shuffle scores with the current pinned RNG dependencies. This makes a measurement reproducible; it does not make a single finite surrogate an uncertainty estimate.

## What fixes the shuffled copies

At every block size, the shuffle begins with a fresh copy of the original sequence. An FNV-1a hash of the complete input is combined with the block-size index to seed ChaCha8. The sampler then performs `floor(N/2)` swaps between distinct aligned blocks. One surrogate is used at each scale.

The exact `rand` and `rand_chacha` versions are pinned. Their stream behavior, the seed derivation, the fixed-suffix convention, iteration count and input bytes all matter for a reproducible result. The [implementation explanation](../concepts/implementation.md) and [EMC-like measure](../concepts/emc.md) describe the calculation.

## Same process does not mean same input

These operations can change a finite score even when your scientific interpretation treats them as equivalent:

- cyclically shifting the sequence;
- relabeling its symbols;
- changing byte encoding or case;
- changing a threshold, window boundary or concatenation order;
- taking a longer sample or changing the block-size cap.

Some change the sequence's structure under the measurement, some change block alignment, and all byte changes also change the content-derived seed. A deterministic shuffle does not provide invariance to those transformations.

For a constant sequence, no permutation changes any byte, so every shuffled/original contrast is zero. For at most ten input bytes the released sampler is a no-op; a zero score there does not establish an absence of structure.

## Worker counts

The Rust core uses Rayon for independent work such as surrogate factorizations and paired counts. Surrogate seeds depend on content and scale, not the order in which workers finish.

| Interface | How to select worker count |
|---|---|
| CLI binaries | `-j` / `--jobs` configures their Rayon worker pool |
| Python | `jobs` is reserved and ignored; use Rayon configuration before native work starts |
| Rust | Configure a Rayon pool appropriate to your embedding application |

For a Python script, set the environment variable before the first native call:

```python
import os
os.environ["RAYON_NUM_THREADS"] = "4"

import lzcomplexity as lz
score, increments = lz.emc(b"00010111" * 1024, log_base=2, max_block_size=16)
print(score)
```

Or set it before launching the process:

```sh
RAYON_NUM_THREADS=4 python analysis.py
```

Rayon's global pool is initialized once. Changing the environment after initialization does not resize that existing pool. Use a fresh process when comparing worker settings.

The current Python binding retains the GIL during native calls. Native Rayon workers still run in parallel, but a Python thread pool is not a way to launch independent concurrent binding calls. If using processes, budget processes × native workers to avoid oversubscribing the machine. Measure the complete job, including startup and data movement.

## A useful measurement record

Store this information with an analysis:

```python
import hashlib
import platform
import lzcomplexity as lz

sequence = b"00010111" * 1024
settings = {"log_base": 2, "max_block_size": 16}
record = {
    "package_version": lz.__version__,
    "python": platform.python_version(),
    "input_bytes": len(sequence),
    "input_sha256": hashlib.sha256(sequence).hexdigest(),
    "settings": settings,
    "result": lz.lz76(sequence, **settings),
}
```

For source installations also save `git rev-parse HEAD`, whether the tree had local changes, and your built-wheel hash when available. A source checkout can include unreleased fixes while reporting the same package version as the latest release.

Record preprocessing and data-generation seeds separately from the library's content-derived shuffle seed. Repeating identical calls reproduces the same surrogates; it is not an ensemble of independent shuffles or independent source realizations.
