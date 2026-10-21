# Shuffle-measure diagnostic results

These checks inspect the existing **EMC-like shuffle measure**. They do not
assert equality to effective measure complexity, excess entropy, or mutual
information. All production values below use the existing shuffle generator,
LZ76 factor counts, scale weighting and nonnegative isotonic projection.
No source labels or expected scientific answers enter the production code.

## Reproduce

```sh
cargo build --release -p lzcomplexity-core --example shuffle_diagnostics
python scripts/validate_shuffle.py --self-test
python scripts/validate_shuffle.py --lengths 2048 --seeds 1 --check
python scripts/validate_shuffle.py --check --json > shuffle-report.json
```

The diagnostic example reads raw bytes, bypassing file-format detection. The
Python harness needs only the standard library. It does not build or modify
the repository automatically; `--driver` can select an already-built example.

The numerical tables were captured against the original shuffle implementation
at rollback commit `75d3fef`, whose tracked file tree matched release baseline
`edbd34d`. The harness now also records the driver binary SHA-256, source
revision and whether the source tree is dirty; it never silently presents a
modified checkout as clean HEAD. These fields are read-only diagnostics.

Recorded settings: Python 3.9.6; lengths 2,048, 16,384 and 65,536; input seeds
0, 1 and 2; `log_base=2`; one worker; automatic block-size cutoff. Fixed-cap
checks use 8, 16 and 24 at length 65,536 with input seed 0. Periodic fixtures
use the seed as their starting phase; other stochastic fixtures use Python's
`random.Random(seed)`. Shuffles retain the production content-derived seeds,
ChaCha8 stream, aligned block swaps, unchanged tail and `N/2` swap iterations.
These input seeds are not an independent ensemble of shuffle seeds, and the
reported ranges are not confidence intervals or calibrated error bars.

## Observed response

The table contains mean **current production scores** across the three input
seeds/phases. It is descriptive; no source ordering is a universal assertion.
All numbers use base-2 logarithms, not a claim of calibrated mutual information.

| source | N=2,048; M=18 | N=16,384; M=21 | N=65,536; M=22 |
|---|---:|---:|---:|
| constant | 0.000000 | 0.000000 | 0.000000 |
| iid p=0.1 | 0.243490 | 0.030560 | 0.028537 |
| iid p=0.5 | 0.033191 | 0.061310 | 0.009155 |
| period 2 | 1.014540 | 1.734619 | 0.785278 |
| period 8 | 5.220703 | 5.419189 | 4.146810 |
| Markov flip=0.1 | 1.335015 | 1.315989 | 1.267782 |
| even gamma=0.1 | 0.413915 | 0.574219 | 0.599797 |
| even gamma=0.5 | 0.803799 | 1.144307 | 1.311149 |
| even gamma=0.9 | 0.266509 | 0.364185 | 0.442220 |

On these fixtures, the periodic and correlated controls generally respond more
strongly than independent inputs. Fair and biased independent sequences can
still have positive finite-sample scores, and the size need not decrease
monotonically with length. Constant input scores exactly zero: shuffling
cannot alter it, so every shuffled/original LZ contrast is zero. This follows
from the defined contrast; it is not a statement that constant sequences have
no visible pattern.

Automatic cutoffs change with input length. The period-2 score falls sharply
between the latter two lengths as the cutoff moves from odd to even. An even
block size preserves this pattern inside identical blocks, producing a zero
contrast at that scale. The projection combines it with other scales. This
shows why a length comparison also needs a matched-cutoff comparison; it is
not evidence of a change in the source's intrinsic structure.

## Cutoff and phase sensitivity

Same length 65,536, same seed/phase 0, with only the requested cap changed:

| source | cap 8 | cap 16 | cap 24 |
|---|---:|---:|---:|
| iid p=0.5 | 0.000000 | 0.000000 | 0.035156 |
| period 2 | 0.673340 | 0.748901 | 0.797363 |
| period 8 | 2.779175 | 3.252594 | 3.495392 |
| even gamma=0.9 | 0.183594 | 0.367188 | 0.480469 |

For the period-8 fixture at N=65,536 with the automatic cap 22, all eight cyclic
phases gave scores between **4.112427 and 4.184326**. Changing phase also
changes the input hash and therefore the surrogate draw; this measures their
combined finite-sample effect rather than alignment alone. It supplies no
calibrated uncertainty interval.

## Two distinct formulas

The [2019 paper, Eq. (4)](https://journals.plos.org/plosone/article?id=10.1371/journal.pone.0214863)
uses an unweighted sum of shuffled/original entropy-rate contrasts. The current
released implementation instead multiplies each contrast by its block size,
projects that weighted ladder, and returns its final fitted level. These are
different finite-sample quantities. Their difference is a scientific-definition
choice to discuss explicitly, not an implementation bug automatically corrected
by this harness.

For a compact comparison, N=16,384, cap 21, input seed/phase 0, using exactly
the same underlying shuffled factor counts:

| source | current weighted/PAV score | unweighted contrast sum |
|---|---:|---:|
| constant | 0.000000 | 0.000000 |
| iid p=0.1 | 0.039022 | 0.023926 |
| iid p=0.5 | 0.161499 | 0.000854 |
| period 8 | 5.508911 | 8.624390 |
| even gamma=0.9 | 0.320947 | 0.371704 |

The harness reports the unweighted sum as a diagnostic alongside the current
score. It never substitutes that sum for the production result, clips it, or
uses it as an accuracy oracle; unlike the projected score, it can be negative.
Changing production to this or any other formula needs an explicit scientific
decision. Agreement with exact EMC is not a test requirement for either column.

## Implementation checks

The repaired alphabet cache now gives the same normalization for identical
bytes constructed at once or built incrementally. For 4,000 repeated `ACGT`
bytes, both routes give **2.046149112677217** with default settings. Previously
the incremental route retained a stale alphabet size of two and returned
**4.092298225354434**; the fresh construction already returned the correct
four-symbol normalization. Explicit user size declarations remain supported.
The CLI also now honors an explicit `-l` in its entropy-density report while
retaining its legacy default. Neither fix changes the shuffle formula.

All checks passed for the recorded run:

- 81 automatic-cutoff records: nine controls, three lengths, three input
  seeds/phases; plus fixed-cap and phase-sensitivity records.
- Finite values; nonnegative fitted score and increments; increments sum to
  the reported score; the raw one-symbol contrast matches `multi_information`.
- Every fitted ladder matches an independent min-max characterization of
  isotonic regression. The reference does not use the production
  pool-adjacent-violators algorithm.
- Shared surrogate-count prefixes remain identical when only the cap changes.
- Four controls repeat bit-for-bit and match between one and three workers.
- Twelve checks of the complete public function match reuse of the original
  factor count; base-4 output has the same surrogate counts and expected scaling.
- Five self-tests cover the independent projection reference, formula
  distinction and control generators.
- A separate compatibility run compared preserved pre-fix and corrected
  binaries on 49 freshly constructed sequences: nine controls at two lengths
  and two caps, alphabets of 4/20/256 symbols, and seven tiny-length cases.
  Scores, diagnostics, summands, raw ladders and surrogate counts were
  bit-identical. The alphabet-cache correction changes stale/mutated metadata,
  not the scientific shuffle formula or these fresh-input results.

These checks establish implementation properties on the tested inputs.
They do not establish universal qualitative ordering, convergence, exact EMC,
or statistical error bars. Definitions, caps, input encoding and versions must
be matched when comparing measurements. See [the method description](shuffle-measure.md).
