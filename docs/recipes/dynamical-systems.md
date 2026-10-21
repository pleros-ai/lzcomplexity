# From a dynamical system to a symbolic sequence

LZ76 measures a sequence of observations. For a continuous system, define the observation and the partition that turns it into symbols before computing `h` or an EMC-like score. Results describe that symbolic representation, not every property of the underlying equations.

The manual uses locally coupled oscillators and a mean-based binary partition. This tutorial uses the logistic map as a smaller, reproducible illustration of the same workflow: simulate, discard a transient, symbolize, then measure.

## A complete example

The logistic map advances a state `x` by `x ← r*x*(1-x)`. The code partitions its observations at `x=0.5`. The initial condition, transient, retained length and partition are explicit.

```python
import lzcomplexity as lz


def logistic_symbols(r, length=8192, transient=2000, initial=0.123456789):
    if length < 1 or transient < 0 or not 0 < initial < 1:
        raise ValueError("Use a positive length and an initial state inside (0, 1)")
    x = initial
    for _ in range(transient):
        x = r * x * (1 - x)
    symbols = bytearray()
    for _ in range(length):
        x = r * x * (1 - x)
        symbols.append(1 if x >= 0.5 else 0)
    return bytes(symbols)


print("r       length      h      EMC-like")
for r in (3.2, 3.57, 3.8, 4.0):
    sequence = logistic_symbols(r)
    result = lz.lz76(sequence, log_base=2, max_block_size=16)
    print(f"{r:4.2f} {len(sequence):8d} {result['h']:7.4f} {result['emc']['value']:10.4f}")
```

The parameter choices are demonstration points. They are not a classification algorithm or a claim that a single LZ score proves chaos. Finite-precision trajectories can eventually repeat; changing the initial condition, precision or partition can change the finite sequence.

## Make a parameter scan reproducible

When scanning a control parameter, hold the retained length, sampling scheme, partition, transient policy, log base and shuffle cap fixed. Repeat across initial conditions when the scientific question requires an ensemble. Store the parameters with each row instead of only plotting the two scores.

Increasing the retained length can help assess stability, but the automatic shuffle cap also changes with length. Examine length and cap separately. In periodic sequences, a block size matching the period can preserve whole repeated blocks, producing little or no contrast at that scale. That is a feature of what shuffling changes, not proof that the original sequence lacks structure.

Use the [entropy-density](../concepts/entropy-density.md), [EMC-like measure](../concepts/emc.md) and [convergence](../concepts/convergence.md) pages to interpret the response.

## Connecting this to the manual's oscillator study

The historical example evolves an even-sized ring with alternating coupling and periodic boundary conditions:

```text
dθ_i/dt = ω + γ*cos(θ_i) + (-1)^i * [cos(θ_(i-1)) + cos(θ_(i+1))]
```

It observes oscillator activity through `sin(θ_i)` and uses a mean-based binary partition. A spatial string at one time and a temporal string at one oscillator encode different observations. State explicitly which axis you serialize and which mean you use as the threshold.

The manual describes parameter scans, initial-condition ensembles and fourth-order Runge–Kutta integration. Its prose and figure captions report different simulation sizes in places, so reproducing a published figure requires its original study protocol rather than selecting one number from the manual. The [historical PDF](../downloads/manual-0.9.pdf) preserves those details and references.

The library provides the symbolic analysis. It does not integrate the oscillator equations, choose a generating partition, estimate Lyapunov exponents or automatically locate a critical point.
