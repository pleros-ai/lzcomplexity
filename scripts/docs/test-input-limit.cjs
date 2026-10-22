const assert = require('node:assert/strict');
const { utf8Prefix, fitInsertion } = require('../../docs/assets/site.js');
const bytes = value => new TextEncoder().encode(value).length;

assert.equal(utf8Prefix('a'.repeat(300)), 'a'.repeat(256));
assert.equal(utf8Prefix('é'.repeat(150)), 'é'.repeat(128));
assert.equal(utf8Prefix('🧬'.repeat(100)), '🧬'.repeat(64));
assert.equal(utf8Prefix('a'.repeat(253) + '🧬'), 'a'.repeat(253));
assert.equal(utf8Prefix('a'.repeat(252) + '🧬'), 'a'.repeat(252) + '🧬');
assert.equal(utf8Prefix('é🧬a', 5), 'é');
assert.equal(utf8Prefix('a', 0), '');
assert.equal(fitInsertion('a'.repeat(256), 128, 128, 'x'), '');
assert.equal(fitInsertion('a'.repeat(256), 128, 130, '🧬'), '');
assert.equal(fitInsertion('a'.repeat(256), 128, 132, '🧬x'), '🧬');
assert.equal(fitInsertion('a'.repeat(254) + 'ZZ', 100, 102, '12345'), '12');
assert.equal(fitInsertion('é'.repeat(128), 20, 22, '🧬x'), '🧬');
assert.equal(fitInsertion('é'.repeat(128), 20, 20, '\n'), '');
for (const [value, start, end, pasted] of [
  ['prefixSUFFIX', 6, 6, 'x'.repeat(400)],
  ['é'.repeat(128), 10, 14, '🧬'.repeat(100)],
  ['a'.repeat(256), 64, 128, 'é'.repeat(100)],
]) {
  const insertion = fitInsertion(value, start, end, pasted);
  const result = value.slice(0, start) + insertion + value.slice(end);
  assert.ok(bytes(result) <= 256);
  assert.ok(result.endsWith(value.slice(end)), 'Pasting preserves text after the selection.');
  assert.ok(!/[\uD800-\uDBFF]$/.test(insertion), 'Do not split a UTF-16 surrogate pair.');
}
console.log('Input limits handle 256-byte ASCII/Unicode, insertion, selection replacement and oversized paste.');
