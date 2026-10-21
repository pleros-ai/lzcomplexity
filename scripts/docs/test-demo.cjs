const assert = require('node:assert/strict');
const { analyze } = require('../../docs/assets/site.js');
const { cases } = require('../../tests/docs/demo-cases.json');
for (const test of cases) {
  const actual = analyze(test.input);
  assert.equal(actual.count, test.count, test.input);
  assert.deepEqual(actual.boundaries, test.boundaries, test.input);
  assert.ok(Math.abs(actual.h - test.h) < 1e-12, test.input);
}
assert.throws(() => analyze('é'.repeat(65)), /128 UTF-8 bytes/);
console.log(`${cases.length} native-library fixtures match the browser demonstration.`);
