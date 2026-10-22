# From a dynamical system to a symbolic sequence

Study a dynamical system by choosing an observable, converting it to symbols and examining h and E. The observable and partition define which features of the dynamics enter the analysis.

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


print("r       length      h          E")
for r in (3.2, 3.57, 3.8, 4.0):
    sequence = logistic_symbols(r)
    result = lz.lz76(sequence, log_base=2, max_block_size=16)
    print(f"{r:4.2f} {len(sequence):8d} {result['h']:7.4f} {result['emc']['value']:10.4f}")
```

These parameter values sample periodic and irregular regimes. Compare the resulting symbolic sequences while keeping the initial condition, transient and partition fixed. For longer simulations, also check sensitivity to numerical precision.

## Make a parameter scan reproducible

When scanning a control parameter, hold the retained length, sampling scheme, partition, transient policy, logarithm base and maximum range fixed. Repeat across initial conditions when the scientific question requires an ensemble. Store the parameters with each row instead of only plotting the two scores.

Repeat the scan at several sample lengths to assess stability. Set `max_block_size` explicitly to compare the same ranges, then inspect the E profile to see which ranges contribute to the result.

Use the [entropy-density](../concepts/entropy-density.md), [structural complexity E](../concepts/emc.md) and [convergence](../concepts/convergence.md) pages to interpret the response.

## Connecting this to the manual's oscillator study

The example evolves an even-sized ring with alternating coupling and periodic boundary conditions:

```text
dθ_i/dt = ω + γ*cos(θ_i) + (-1)^i * [cos(θ_(i-1)) + cos(θ_(i+1))]
```

It observes oscillator activity through `sin(θ_i)` and uses a mean-based binary partition. A spatial string at one time and a temporal string at one oscillator encode different observations. State explicitly which axis you serialize and which mean you use as the threshold.

The [manual](../downloads/manual-0.9.pdf) describes parameter scans, ensembles of initial conditions and fourth-order Runge–Kutta integration. Its worked example connects the symbolic representation to the system's transition between dynamical regimes.

Simulate the trajectories with your numerical tools, then pass the resulting symbolic sequences to lzcomplexity for analysis.
