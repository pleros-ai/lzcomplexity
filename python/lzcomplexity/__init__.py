"""LZ76-based complexity analysis for symbolic sequences.

Top-level functions
-------------------
- ``factorization`` — complexity (factor count) and factor boundary list.
- ``h`` — normalized entropy density (entropy-rate estimator).
- ``emc`` — effective measure complexity E (value and fitted increments).
- ``nid`` — normalized information distance between two sequences.
- ``lz76`` — the full analysis, returned as a dict with everything.

All sequence-accepting functions accept ``str``, ``bytes``, ``list[int]``,
``list[str]``, or integer sequences such as tuples and compatible NumPy arrays.
Materialize generators with ``list(...)`` or ``bytes(...)`` first. For
``list[int]``, each element's decimal representation is concatenated:
``[0, 1, 10]`` becomes the symbolic string ``"0110"``.

E defaults to Chao–Shen. Pass ``method="shuffle"`` to ``emc()`` or
``lz76()`` for the earlier random-shuffle estimator.

E is sensitive to correlations across different ranges of a sequence.
Compare matched lengths, encodings, logarithm bases and range limits.

Use ``help(lzcomplexity.<name>)`` on any of the names above for full
parameter docs.

"""

from .lzcomplexity import (
    __version__,
    lz76,
    factorization,
    h,
    emc,
    nid,
)

__all__ = [
    "lz76",
    "factorization",
    "h",
    "emc",
    "nid",
]

# Hide the compiled-extension submodule from `dir(lzcomplexity)` and from
# `import lzcomplexity; lzcomplexity.lzcomplexity` — the public surface is the
# names listed in `__all__` above.
del globals()["lzcomplexity"]
