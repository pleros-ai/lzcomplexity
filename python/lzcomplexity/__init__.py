"""LZ76-based complexity analysis for symbolic sequences.

Top-level functions
-------------------
- ``lz76`` — full analysis (complexity, entropy, factors, shuffle).
- ``factorization`` — just the LZ76 factor count.
- ``factors`` — complexity and factor boundary list.
- ``entropy_density`` — normalized entropy density.
- ``emc`` — effective measure complexity (random shuffle).

Submodules
----------
- ``metrics`` — information distances (``nid``, ``rid``).
- ``spectral`` — FFT-based spectral analysis (``psd``, ``entropy``, ``semc``).

All sequence-accepting functions accept ``str``, ``bytes``, ``list[int]``,
``list[str]``, or any iterable of ints (e.g. NumPy arrays). For
``list[int]``, each element's decimal representation is concatenated:
``[0, 1, 10]`` becomes the symbolic string ``"0110"``.

Use ``help(lzcomplexity.<name>)`` on any of the names above for full
parameter docs.
"""

from .lzcomplexity import (
    __version__,
    lz76,
    factorization,
    factors,
    entropy_density,
    emc,
    metrics,
    spectral,
)

__all__ = [
    "lz76",
    "factorization",
    "factors",
    "entropy_density",
    "emc",
    "metrics",
    "spectral",
]

# Hide the compiled-extension submodule from `dir(lzcomplexity)` and from
# `import lzcomplexity; lzcomplexity.lzcomplexity` — the public surface is the
# names listed in `__all__` above.
del globals()["lzcomplexity"]
