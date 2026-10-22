# Sampled signals and neural time-series

Convert a sampled recording to a symbolic sequence, then use h and E to study its unpredictability and organization. The symbolization rule determines which variations in the recording are represented.

The manual's neural example uses a binary threshold at each regional time series' mean. The synthetic example below follows that approach for one channel.

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


# Synthetic 8 Hz signal with additive noise.
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
print("E:", result["emc"]["value"])
print("block-size cap:", result["emc"]["max_block_size"])
```

`bytes` gives exactly one input symbol per sample, including zero bytes. `log_base=2` fixes the entropy-density units to bits per input symbol. This example examines ranges up to 16 samples; choose the range to suit your sampling rate and question.

Threshold equality goes to zero. Choosing `>=` instead of `>` can change results when the signal has many repeated values. Record that convention.

## Replace the synthetic series with your data

Before symbolization, apply your acquisition protocol for artifacts, missing samples, filtering and analysis intervals. Match sampling rates across recordings you want to compare.

A threshold based on each epoch's mean emphasizes variations within that epoch. A common threshold fitted to a reference interval preserves a different comparison. Choose one rule and apply it consistently across groups or conditions.

Use equally long windows with the same sampling rate and symbolization. Then compare more than a single number:

- `h`: the normalized LZ entropy-rate estimate of the symbolic series;
- E: sensitivity to correlations across temporal ranges;
- symbol frequencies: thresholding can change the balance of zeros and ones;
- sensitivity to window length and cap, with repeated subjects/epochs as appropriate.

Use repeated epochs or subjects to estimate variability. [Reading the numbers](../guide/reading-the-numbers.md) explains the returned values and diagnostic fields.

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

Those streams contain the same entries in different orders. Time-major ordering puts simultaneous channel states next to each other; channel-major ordering preserves each channel's temporal run. Choose the ordering that represents the relationships you want to study.

## Related research and reproducibility

The [manual](../project/manual.md) discusses task-based fMRI and regional symbolic series, with worked examples and research references.

Save your preprocessing version, sampling rate, window boundaries, threshold rule, channel ordering, input bytes or their hash, package version, logarithm base and maximum range. The same symbolic bytes with the same settings are reproducible; a different symbolization is a different observation of the system.
