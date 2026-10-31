const assert = require('node:assert/strict');
const { analyze, emc, distance, _testing } = require('../../docs/assets/measure-engine.js');
const { cases: parseCases } = require('../../tests/docs/demo-cases.json');
const { cases: measureCases, distances } = require('../../tests/docs/measure-cases.json');
const { cases: autoBaseCases } = require('../../tests/docs/auto-base-cases.json');
const rngCase = require('../../tests/docs/rng-cases.json');
const visual = require('../../tests/docs/factorization-visual.json');
const visualParse = analyze(visual.sequence);
assert.equal(visualParse.count, visual.count, 'illustrated factor count');
assert.deepEqual(visualParse.boundaries, visual.boundaries, 'illustrated factor boundaries');
function close(actual, expected, label) {
  assert.ok(Math.abs(actual - expected) < 1e-12, `${label}: ${actual} != ${expected}`);
}
function closeArray(actual, expected, label) {
  assert.equal(actual.length, expected.length, label);
  actual.forEach((value, index) => close(value, expected[index], `${label}[${index}]`));
}
for (const test of parseCases) {
  const actual = analyze(test.input, { logBase: 2 });
  assert.equal(actual.count, test.count, test.input);
  assert.deepEqual(actual.boundaries, test.boundaries, test.input);
  close(actual.h, test.h, test.input);
}
for (const test of measureCases) {
  if (test.count !== undefined) {
    const actual = analyze(test.input, { logBase: 2 });
    assert.equal(actual.count, test.count, test.input);
    assert.deepEqual(actual.boundaries, test.boundaries, test.input);
    close(actual.h, test.h, test.input);
  }
  const actual = emc(test.input, { logBase: 2, ...test.options });
  close(actual.value, test.emc.value, `E ${test.input}`);
  assert.equal(actual.maxBlockSize, test.emc.max_block_size);
  close(actual.multiInformation, test.emc.multi_information, 'one-symbol contrast');
  closeArray(actual.summands, test.emc.summands, 'E increments');
  close(actual.summands.reduce((sum, value) => sum + value, 0), actual.value, 'increments sum');
  actual.summands.forEach(value => assert.ok(value >= 0));
}
for (const test of autoBaseCases) {
  const actual = analyze(test.input);
  assert.equal(actual.alphabetSize, test.alphabetSize, `${test.name}: observed byte alphabet`);
  assert.equal(actual.logBase, test.logBase, `${test.name}: automatic base`);
  assert.equal(actual.count, test.count, `${test.name}: count`);
  assert.deepEqual(actual.boundaries, test.boundaries, `${test.name}: boundaries`);
  close(actual.h, test.h, `${test.name}: automatic h`);
  const score = emc(test.input, { maxBlockSize: test.maxBlockSize });
  close(score.value, test.emc.value, `${test.name}: automatic E`);
  close(score.multiInformation, test.emc.multi_information, `${test.name}: automatic first-range contrast`);
  closeArray(score.summands, test.emc.summands, `${test.name}: automatic E increments`);
  close(analyze(test.input, { logBase: test.logBase }).h, actual.h, `${test.name}: explicit automatic base`);
}
for (const test of distances) {
  const actual = distance(test.a, test.b);
  close(actual.value, test.value, 'distance');
  assert.deepEqual(actual.counts, test.counts);
  close(distance(test.b, test.a).value, actual.value, 'distance symmetry');
}
for (const test of require('../../tests/docs/shuffle-cases.json').cases) {
  const actual = emc(test.input, {logBase: 2, ...test.options, method: 'shuffle'});
  close(actual.value, test.emc.value, 'legacy shuffle value');
  assert.equal(actual.maxBlockSize, test.emc.max_block_size);
  closeArray(actual.summands, test.emc.summands, 'legacy shuffle increments');
  close(actual.multiInformation, test.emc.multi_information, 'legacy shuffle contrast');
}
const rng = new _testing.ChaCha8(BigInt(rngCase.seed));
assert.deepEqual(rngCase.u64.map(() => rng.next64().toString()), rngCase.u64, 'native ChaCha8 u64 words');
const bytes = new TextEncoder().encode(rngCase.input);
rngCase.shuffles.forEach((expected, index) => {
  const actual = _testing.shuffled(bytes, index + 1, BigInt(rngCase.seed));
  assert.deepEqual([...actual], expected, `native block swaps at size ${index + 1}`);
});
for (const test of rngCase.boundaries) {
  assert.deepEqual([..._testing.shuffled(new Uint8Array(test.input), test.size, BigInt(rngCase.seed))], test.output, `boundary block swaps N=${test.input.length} size=${test.size}`);
}
assert.equal(analyze('é'.repeat(128)).bytes.length, 256);
assert.throws(() => analyze('é'.repeat(129)), /256 UTF-8 bytes/);
assert.throws(() => emc('a'.repeat(257)), /256 UTF-8 bytes/);
assert.throws(() => distance('a', 'a'.repeat(257)), /256 UTF-8 bytes/);
assert.throws(() => emc('abc', {maxBlockSize: 2.5}), /Block-size cap/);
assert.throws(() => emc('abc', {maxBlockSize: 257}), /Block-size cap/);
assert.equal(emc('a'.repeat(256)).value, 0, 'constant sequence preserves native E');
assert.equal(emc('0101010101', {method: 'shuffle'}).value, 0, 'N <= 10 preserves native shuffle no-op');
assert.ok(emc('0101010101').value > 0, 'Chao–Shen analyzes short sequences');
assert.throws(() => emc('abc', {method: 'unknown'}), /method must/);
assert.equal(distance('banana', 'banana').value, 1 / 3, 'do not force native self-distance to zero');
console.log(`${parseCases.length} existing parse fixtures, ${measureCases.length} native E fixtures, ${autoBaseCases.length} native automatic-base fixtures, ${distances.length} native distance fixtures, 50 ChaCha8 words and 53 exact block swaps match.`);
