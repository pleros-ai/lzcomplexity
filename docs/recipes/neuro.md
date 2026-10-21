# Sampled signals and neural time-series

The library takes **symbols**, not a continuous voltage or an fMRI time-series directly. A neural-data workflow therefore has two parts: a domain-specific preprocessing protocol, followed by an explicitly defined conversion from samples to symbols.

The manual's brain-connectivity example thresholds each regional time-series at its mean, then compares entropy density and the shuffle-based score. The example below demonstrates that conversion on synthetic data. It is an analysis recipe, not a validated measure of consciousness, a connectivity estimator or a clinical decision rule.

## Start with one channel

Save this as `signal_analysis.py`. It uses only Python's standard library and `lzcomplexity`:

```python
import math
import random
import statistics
import lzcomplexity as lz


def symbolize(samples, threshold):
    values = list(samples)
    if not values or not all(math.isfinite(x) for x in values):
        raise ValueError("Provide a nonempty series of finite samples")
    return bytes(1 if x > threshold else 0 for x in values)


# An illustrative sampled signal, not a physiological recording.
rng = random.Random(7)
fs = 250.0
samples = [
    math.sin(2 * math.pi * 8 * i / fs) + 0.4 * rng.gauss(0, 1)
    for i in range(4096)
]
threshold = statistics.mean(samples)
symbols = symbolize(samples, threshold)
result = lz.lz76(symbols, log_base=2, max_block_size=16)

print("samples:", len(symbols))
print("fraction above threshold:", sum(symbols) / len(symbols))
print("h:", result["h"])
print("EMC-like score:", result["emc"]["value"])
print("block-size cap:", result["emc"]["max_block_size"])
```

`bytes` gives exactly one input symbol per sample, including zero bytes. `log_base=2` fixes the entropy-density units to bits per input symbol. The cap of 16 is an explicit example setting, not a generally optimal choice.

Threshold equality goes to zero. Choosing `>=` instead of `>` can change results when the signal has many repeated values. Record that convention.

## Replace the synthetic series with your data

Perform acquisition-specific quality control before symbolization: handle artifacts, missing samples, preprocessing, sampling-rate differences and the choice of analysis interval according to your study protocol. The library does not perform those operations for you.

A threshold based on each epoch's mean emphasizes variations within that epoch. A common threshold fitted to a reference interval preserves a different comparison. Neither choice is automatic in the library. If comparing groups or conditions, use a justified and consistently applied rule; avoid changing preprocessing to obtain the desired complexity ordering.

Use equally long windows with the same sampling rate and symbolization. Then compare more than a single number:

- `h`: the normalized LZ entropy-rate estimate of the symbolic series;
- the EMC-like score: the response to block shuffling under matched caps and units;
- symbol frequencies: thresholding can change the balance of zeros and ones;
- sensitivity to window length and cap, with repeated subjects/epochs as appropriate.

A finite-sample estimate can exceed an asymptotic entropy bound. The shuffle score is not calibrated mutual information, and its legacy error fields are not uncertainty intervals for a neural experiment. See [reading the numbers](../guide/reading-the-numbers.md).

## Multiple channels: choose the question first

For a separate score per channel, call the same function on each channel using matched-length windows. This preserves each channel's temporal ordering.

If you flatten channels into one stream, the ordering becomes part of the method. For example:

```python
# Three binary channels, four observations each.
channels = [b"0101", b"0011", b"1110"]
assert len({len(channel) for channel in channels}) == 1

# All channels at observation 0, then all at observation 1, ...
time_major = bytes(
    channel[t]
    for t in range(len(channels[0]))
    for channel in channels
)
# Channel 0's entire history, then channel 1, ...
channel_major = b"".join(channels)
assert len(time_major) == len(channel_major)
```

Those streams contain the same entries in different orders; LZ analysis can distinguish them. A flattened series is not automatically a connectivity network. Its artificial channel boundaries and ordering need to fit your hypothesis.

## Related research and reproducibility

The manual discusses task-based fMRI and regional symbolic series. Its historical figures are examples from a particular analysis, not acceptance thresholds for this package or new results reproduced here. The [manual and coverage guide](../project/manual.md) locates those studies and the full reference PDF.

Save your preprocessing version, sampling rate, window boundaries, threshold rule, channel ordering, input bytes or their hash, package/source revision, log base and shuffle cap. The same symbolic bytes with the same settings are reproducible; a different symbolization is a different observation of the system.
