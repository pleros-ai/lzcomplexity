# Performance and scaling

The Rust core shares work across Python and the command-line tools. Factorization constructs sequence-indexing arrays; calculating E repeats factorization at several ranges. The table below helps estimate the work needed for each operation.

## Where the time goes

| Operation | Principal work |
|---|---|
| `factorization(seq)` | One original-sequence suffix-array/LCP/LPF pipeline, except short-path inputs |
| `h(seq)` | Original factorization followed by scalar normalization |
| `emc(seq, max_block_size=M)` | Original factorization plus one surrogate factorization per scale |
| `lz76(seq, max_block_size=M)` | Reuses the original count for `h`, E and extras; also factorizes surrogates and two halves |
| `nid(x, y)` | Factor counts for `x`, `y`, `xy` and `yx` |
| Full distance matrix | Pairwise work repeated over rows/columns; pair count grows with both dimensions |

Not every task costs the same: a constant sequence takes an early path, while a diverse input constructs the full arrays. Matrix modes and variants may also change which counts can be reused. Use the [CLI reference](../cli/lzdistance.md) for the selected strategy.

## Construction and parallelism

Below 2,048 bytes, suffixes are comparison-sorted. At and above that threshold, the core uses `cdivsufsort`. LCP is computed with Kasai's algorithm and the LPF stage operates on those arrays.

Independent shuffle scales run through Rayon. Direct distance evaluation also schedules independent counts concurrently. A single “full analysis” call reuses the original factor count but does not eliminate surrogate work.

Python's `jobs` and `partitions` arguments have no effect on execution. For Python process-level experiments, set `RAYON_NUM_THREADS` before importing and using the library. CLI worker selection is described in its runtime help and [reference](../cli/index.md).

## Peak memory matters

The byte buffer, suffix array, LCP array, LPF array and returned boundaries occupy memory proportional to input size. Concurrent scale tasks allocate additional sequence and factorization buffers. Increasing concurrency may increase peak RAM even when elapsed time improves.

The API does not stream an indefinitely long sequence through a fixed memory window. A larger input therefore requires a corresponding memory budget. Internal indices and some counts are 32-bit, with additional signed count storage in shuffle paths; do not infer a safe maximum input length solely from the host's address space.

## A small timing harness

This script warms the calculation, runs it five times on the same binary sequence, and reports the median elapsed time:

```python
import os
import platform
import random
import statistics
import time

import lzcomplexity as lz

rng = random.Random(1729)
sequence = bytes(rng.randrange(2) for _ in range(100_000))
settings = {"log_base": 2, "max_block_size": 16}

# Warm the same code path before recording timings.
lz.emc(sequence, **settings)
times = []
for _ in range(5):
    start = time.perf_counter()
    value, terms = lz.emc(sequence, **settings)
    times.append(time.perf_counter() - start)

print({
    "version": lz.__version__,
    "python": platform.python_version(),
    "platform": platform.platform(),
    "length_bytes": len(sequence),
    "source_seed": 1729,
    "rayon_num_threads": os.environ.get("RAYON_NUM_THREADS", "default"),
    "settings": settings,
    "median_seconds": statistics.median(times),
    "all_seconds": times,
    "score": value,
})
```

Run thread-count comparisons in fresh processes, for example:

```sh
RAYON_NUM_THREADS=1 python benchmark.py
RAYON_NUM_THREADS=4 python benchmark.py
```

Compare worker counts on representative data: CPU availability, memory bandwidth and the selected operation all affect scaling.

## Make comparisons fair

Record the CPU model, core availability, RAM, OS, compiler/build profile, package/source revision, input bytes or generator, logarithm base, cap, worker setting and whether I/O was timed. Compare the same operation on the same inputs and settings. Time the operation and alphabet used in your analysis: factorization and E perform different amounts of work.

Separate computation from reading and preprocessing when useful, but state that boundary. Report multiple measurements rather than one best run. If you change sequence length, also record the actual automatic cap or keep it explicit so the amount of requested work is clear.

For exact stage definitions, see [Inside the implementation](../concepts/implementation.md).
