# Reproducibility and parallel execution

Identical input bytes and parameters produce the same E value, independently
of worker scheduling.

## Inputs and settings

Keep sequence length, encoding, logarithm base and `max_block_size` consistent
when comparing E. Changing the byte labels, cyclic alignment or preprocessing
can change the resulting value. The
[E guide](../concepts/emc.md) explains the range settings and interpretation.

For studies involving generated data, save the data-generation seed separately
from the analysis settings. Use independent realizations or windows when the
experiment calls for an ensemble.

## Worker counts

The Rust core uses Rayon to parallelize independent factorizations. Choose
worker counts through the interface you use:

| Interface | Worker configuration |
|---|---|
| Command line | `-j` / `--jobs` sets the Rayon worker count |
| Python | Set `RAYON_NUM_THREADS` before the first native call; the `jobs` keyword has no effect |
| Rust | Run the calculation in a configured Rayon pool |

For a Python script:

```python
import os
os.environ["RAYON_NUM_THREADS"] = "4"

import lzcomplexity as lz
score, increments = lz.emc(b"00010111" * 1024, log_base=2, max_block_size=16)
print(score)
```

Or select the worker count when launching it:

```sh
RAYON_NUM_THREADS=4 python analysis.py
```

Rayon's global pool is initialized once. Start a fresh process to compare
worker settings.

Python calls retain the GIL while the native calculation runs. Rayon workers
can execute in parallel within that call. To run independent Python analyses
concurrently, use separate processes and divide the available cores between
the processes and their Rayon workers.

## Save an analysis record

This example stores the input fingerprint, settings and complete result:

```python
import hashlib
import lzcomplexity as lz

sequence = b"00010111" * 1024
settings = {"log_base": 2, "max_block_size": 16}
record = {
    "package_version": lz.__version__,
    "input_bytes": len(sequence),
    "input_sha256": hashlib.sha256(sequence).hexdigest(),
    "settings": settings,
    "result": lz.lz76(sequence, **settings),
}
```

Save the symbolization rule and any preprocessing alongside this record.
