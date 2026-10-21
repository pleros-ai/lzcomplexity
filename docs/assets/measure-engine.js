/* Browser companion to lzcomplexity's released Rust implementation.
 * The small-input LPF search is deliberately simple. Seed expansion, ChaCha8,
 * 64-bit rand 0.8.6 inclusive sampling, block swaps and score projection follow
 * crates/lzcomplexity-core/{sequence,shuffle,metrics,lz76}.rs without changing
 * the production library. Native parity fixtures live in tests/docs/.
 * BigInt preserves the Rust u64 arithmetic on every supported browser.
 */
(function (scope) {
  "use strict";
  const MASK64 = (1n << 64n) - 1n;
  const LIMIT = 256;
  function encode(text) {
    const bytes = new TextEncoder().encode(text);
    if (bytes.length > LIMIT) throw new Error("Please use at most 256 UTF-8 bytes per sequence.");
    return bytes;
  }
  function factorize(bytes) {
    const n = bytes.length;
    if (!bytes.some(value => value !== bytes[0])) return { count: 1, boundaries: [0, 1, n] };
    const boundaries = [0, 1];
    let position = 1;
    while (position < n) {
      let longest = 0;
      for (let previous = 0; previous < position; previous++) {
        let length = 0;
        while (position + length < n && bytes[previous + length] === bytes[position + length]) length++;
        longest = Math.max(longest, length);
      }
      position += longest + 1;
      boundaries.push(position);
    }
    return { count: boundaries.length - (boundaries[boundaries.length - 1] > n ? 2 : 1), boundaries };
  }
  function analyze(text) {
    const bytes = encode(text), result = factorize(bytes), n = bytes.length;
    return { ...result, bytes, h: n > 1 ? result.count / (n / (Math.log(n) / Math.log(2))) : 0 };
  }
  function rotateLeft(value, bits) { return ((value << bits) | (value >>> (32 - bits))) >>> 0; }
  // rand_core 0.6.4 SeedableRng::seed_from_u64: eight PCG32 words.
  function seedWords(seed) {
    const words = [];
    for (let i = 0; i < 8; i++) {
      seed = (seed * 6364136223846793005n + 11634580027462260723n) & MASK64;
      const x = Number((((seed >> 18n) ^ seed) >> 27n) & 0xffffffffn) >>> 0;
      const rot = Number(seed >> 59n);
      words.push(((x >>> rot) | (x << ((32 - rot) & 31))) >>> 0);
    }
    return words;
  }
  class ChaCha8 {
    constructor(seed) {
      this.key = seedWords(seed);
      this.counter = 0n;
      this.words = [];
      this.index = 0;
    }
    refill() {
      const initial = [0x61707865, 0x3320646e, 0x79622d32, 0x6b206574, ...this.key,
        Number(this.counter & 0xffffffffn), Number(this.counter >> 32n), 0, 0];
      const x = initial.slice();
      function quarter(a, b, c, d) {
        x[a] = (x[a] + x[b]) >>> 0; x[d] = rotateLeft(x[d] ^ x[a], 16);
        x[c] = (x[c] + x[d]) >>> 0; x[b] = rotateLeft(x[b] ^ x[c], 12);
        x[a] = (x[a] + x[b]) >>> 0; x[d] = rotateLeft(x[d] ^ x[a], 8);
        x[c] = (x[c] + x[d]) >>> 0; x[b] = rotateLeft(x[b] ^ x[c], 7);
      }
      for (let round = 0; round < 4; round++) {
        quarter(0, 4, 8, 12); quarter(1, 5, 9, 13); quarter(2, 6, 10, 14); quarter(3, 7, 11, 15);
        quarter(0, 5, 10, 15); quarter(1, 6, 11, 12); quarter(2, 7, 8, 13); quarter(3, 4, 9, 14);
      }
      this.words = x.map((value, i) => (value + initial[i]) >>> 0);
      this.index = 0;
      this.counter = (this.counter + 1n) & MASK64;
    }
    next32() {
      if (this.index === this.words.length) this.refill();
      return this.words[this.index++];
    }
    next64() { const low = BigInt(this.next32()); return low | (BigInt(this.next32()) << 32n); }
    inclusive(high) {
      // rand::UniformInt<usize>::sample_single_inclusive on 64-bit targets.
      const range = BigInt(high + 1);
      const leadingZeros = 64n - BigInt(range.toString(2).length);
      const zone = ((range << leadingZeros) - 1n) & MASK64;
      while (true) {
        const product = this.next64() * range;
        if ((product & MASK64) <= zone) return Number(product >> 64n);
      }
    }
  }
  function hashBytes(bytes) {
    let hash = 14695981039346656037n;
    for (const byte of bytes) hash = ((hash ^ BigInt(byte)) * 1099511628211n) & MASK64;
    return hash;
  }
  function shuffled(bytes, size, seed) {
    const result = bytes.slice(), n = bytes.length;
    if (size === 0 || n <= size + 1 || n <= 10) return result;
    const high = Math.floor((n - size - 1) / size);
    if (high === 0) return result;
    const rng = new ChaCha8(seed);
    for (let iteration = 0; iteration < Math.floor(n / 2); iteration++) {
      const a = size * rng.inclusive(high);
      let b;
      do { b = size * rng.inclusive(high); } while (a === b);
      for (let i = 0; i < size; i++) [result[a + i], result[b + i]] = [result[b + i], result[a + i]];
    }
    return result;
  }
  function autoBlockSize(n) {
    if (n < 10) return 1;
    let m = 10, old = 1;
    for (let iteration = 0; iteration < 100; iteration++) {
      m = Math.max(0, Math.round(Math.log(n / old) / Math.log(2)));
      if (m === old) break;
      old = m;
    }
    return m + (n > 50 ? 10 : 0);
  }
  function isotonic(raw) {
    const sums = [], counts = [];
    for (const value of raw) {
      sums.push(value); counts.push(1);
      while (sums.length > 1) {
        const last = sums.length - 1;
        if (sums[last - 1] / counts[last - 1] <= sums[last] / counts[last]) break;
        sums[last - 1] += sums[last]; counts[last - 1] += counts[last];
        sums.pop(); counts.pop();
      }
    }
    return sums.flatMap((sum, i) => Array(counts[i]).fill(Math.max(0, sum / counts[i])));
  }
  function emc(text, options = {}) {
    const bytes = encode(text), n = bytes.length, count = factorize(bytes).count;
    const maxBlockSize = options.maxBlockSize > 0 ? options.maxBlockSize : autoBlockSize(n);
    if (!Number.isInteger(maxBlockSize) || maxBlockSize > 256) throw new Error("Block-size cap must be an integer from 1 to 256.");
    const logBase = Math.max(2, options.logBase || 2);
    const g = n > 1 ? (Math.log(n) / Math.log(logBase)) / n : 0;
    const originalH = count * g, base = hashBytes(bytes), counts = [];
    for (let size = 1; size <= maxBlockSize; size++) {
      const seed = base ^ ((BigInt(size) * 0x9e3779b97f4a7c15n) & MASK64);
      counts.push(factorize(shuffled(bytes, size, seed)).count);
    }
    const raw = counts.map((c, i) => (i + 1) * (c * g - originalH));
    const fitted = isotonic(raw);
    const summands = fitted.map((value, i) => value - (i > 0 ? fitted[i - 1] : 0));
    return { value: fitted.at(-1) || 0, summands, maxBlockSize, multiInformation: raw[0] || 0, raw, fitted, counts };
  }
  function distance(textA, textB) {
    const a = encode(textA), b = encode(textB);
    const xy = new Uint8Array(a.length + b.length), yx = new Uint8Array(xy.length);
    xy.set(a); xy.set(b, a.length); yx.set(b); yx.set(a, b.length);
    const counts = { x: factorize(a).count, y: factorize(b).count, xy: factorize(xy).count, yx: factorize(yx).count };
    const value = Math.max(counts.xy - counts.x, counts.yx - counts.y) / Math.max(counts.x, counts.y);
    return { value, counts };
  }
  const api = { analyze, emc, distance };
  if (typeof module !== "undefined" && module.exports) module.exports = { ...api, _testing: { ChaCha8, shuffled, hashBytes, factorize } };
  scope.LZMeasures = api;
})(typeof globalThis === "undefined" ? this : globalThis);
