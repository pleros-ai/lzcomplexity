# `Sequence`

A `Sequence` owns a `Vec<u8>` and alphabet metadata. Every position is a byte
position. Text is stored as UTF-8; arbitrary byte states can be supplied directly.
Import the type with `use lzcomplexity_core::Sequence`.

The type implements `Clone`, `Debug`, `Default`, `Send`, `Sync`, equality and
lexicographic ordering. Equality and ordering compare the bytes, independently
of alphabet declarations.

## Construct a sequence

### `new` and `default`

```text
Sequence::new() -> Sequence
Sequence::default() -> Sequence
```

Create an empty sequence in automatic alphabet mode. Both constructors are
equivalent. Its observed symbol list is empty and its automatic alphabet size is
two, the normalization floor.

### `with_capacity`

```text
Sequence::with_capacity(capacity: usize) -> Sequence
```

Create an empty sequence with space reserved for at least `capacity` bytes.
Reservation does not add symbols or fix the eventual length. Use it when
building a sequence incrementally.

### `from_str`

```text
Sequence::from_str(text: &str) -> Sequence
```

Copy the UTF-8 bytes of `text` and determine the observed alphabet. This inherent
constructor is infallible; it does not implement Rust's `FromStr` trait.

### `from_bytes`

```text
Sequence::from_bytes(bytes: Vec<u8>) -> Sequence
```

Take ownership of a byte vector and determine the observed alphabet. No text
conversion is performed. Distinct bytes represent distinct states, including
zero and values above 127.

```rust
use lzcomplexity_core::Sequence;

let text = Sequence::from_str("é");
let states = Sequence::from_bytes(vec![0, 1, 255]);
assert_eq!(text.as_bytes(), &[0xc3, 0xa9]);
assert_eq!(text.len(), 2);
assert_eq!(states.alphabet_size(), 3);
```

### `from_bytes_with_alphabet`

```text
Sequence::from_bytes_with_alphabet(bytes: Vec<u8>, alphabet_size: u32) -> Sequence
```

Construct from owned bytes with an alphabet-size **hint**. An exact match with
the observed number of distinct bytes is retained, including zero for an empty
vector and one for a constant vector. A mismatching hint is replaced by the
observed size, floored at two. The hint is not a persistent declaration.

Use `set_alphabet_size` or `with_alphabet_size` when you intend to declare a fixed
normalization alphabet independently of the symbols present.

### `with_alphabet_size`

```text
Sequence::with_alphabet_size(alphabet_size: u32) -> Sequence
```

Create an empty sequence with an explicit size declaration. The declaration
survives subsequent symbol changes until `clear` or `determine_alphabet`.
It does not populate the observed symbol list or restrict accepted byte values.

```rust
use lzcomplexity_core::Sequence;

let hinted = Sequence::from_bytes_with_alphabet(b"AC".to_vec(), 4);
assert_eq!(hinted.alphabet_size(), 2);
let mut declared = Sequence::with_alphabet_size(4);
declared.extend_from_slice(b"AC");
assert_eq!(declared.alphabet_size(), 4);
```

## Read bytes and counts

### `as_bytes` and `as_bytes_mut`

```text
seq.as_bytes() -> &[u8]
seq.as_bytes_mut() -> &mut [u8]
```

Borrow the stored bytes. The mutable slice can replace existing bytes but cannot
change the sequence's length. Borrowing it invalidates cached observed alphabet
information; the next alphabet query rebuilds the cache. An explicit alphabet
size declaration remains in force.

### `to_string_lossy`

```text
seq.to_string_lossy() -> String
```

Return an owned string, replacing invalid UTF-8 with replacement characters.
Use `as_bytes` when exact byte preservation matters.

### `len` and `is_empty`

```text
seq.len() -> usize
seq.is_empty() -> bool
```

Return byte length and whether it is zero. Neither counts Unicode characters.

### `get`, `first` and `last`

```text
seq.get(index: usize) -> Option<u8>
seq.first() -> Option<u8>
seq.last() -> Option<u8>
```

Return a copied byte, or `None` when the requested byte does not exist. `get` uses
zero-based indexing and returns `None` for every index at or beyond `len()`.

### `min_byte`, `max_byte`, `min_range` and `max_range`

```text
seq.min_byte() -> Option<u8>
seq.max_byte() -> Option<u8>
seq.min_range(start: usize, end: usize) -> Option<u8>
seq.max_range(start: usize, end: usize) -> Option<u8>
```

Return the smallest or largest byte in the whole sequence or the half-open range
`[start, end)`. Empty sequences, empty ranges and invalid ranges return `None`.
Range endpoints are not clamped.

### `char_density`

```text
seq.char_density() -> BTreeMap<u8, u32>
```

Count occurrences of every byte present. Keys are sorted in ascending byte order;
values are counts, not probabilities. Divide by `seq.len()` when relative
frequencies are needed.

```rust
use lzcomplexity_core::Sequence;

let seq = Sequence::from_str("ACACA");
assert_eq!(seq.get(1), Some(b'C'));
assert_eq!(seq.get(5), None);
assert_eq!(seq.min_range(1, 4), Some(b'A'));
assert_eq!(seq.max_range(4, 9), None);
assert_eq!(seq.char_density()[&b'A'], 3);
```

## Inspect or declare the alphabet

### `alphabet`

```text
seq.alphabet() -> &[u8]
```

Return the distinct bytes actually present, sorted in **descending** byte order.
The observed list is cached and rebuilt lazily after symbol-changing operations.
An explicit alphabet size does not change this list.

### `alphabet_size`

```text
seq.alphabet_size() -> u32
```

Return the explicit size declaration if there is one; otherwise return the
observed automatic size. Automatic observation normally has a minimum of two.
An exact zero- or one-symbol constructor hint is retained until the observed
cache is invalidated or replaced. The measurement functions still floor their
normalization base at two.

### `set_alphabet_size`

```text
seq.set_alphabet_size(size: u32)
```

Declare the size used by automatic downstream normalization. No bytes are
changed and no check against the observed alphabet is performed. The declaration
survives mutations, including replacement through `as_bytes_mut`.

### `determine_alphabet`

```text
seq.determine_alphabet() -> Vec<u8>
```

Rescan the sequence, replace its observed cache, remove any explicit size
declaration and return the distinct bytes in descending order. If the previous
size exactly matches a zero- or one-symbol alphabet, that value is retained;
otherwise the observed size is floored at two.

```rust
use lzcomplexity_core::Sequence;

let mut seq = Sequence::from_str("ACGT");
assert_eq!(seq.alphabet(), b"TGCA");
seq.set_alphabet_size(8);
seq.as_bytes_mut().fill(b'A');
assert_eq!(seq.alphabet(), b"A");
assert_eq!(seq.alphabet_size(), 8);
assert_eq!(seq.determine_alphabet(), b"A");
assert_eq!(seq.alphabet_size(), 2);
```

## Change sequence contents

### `push` and `pop`

```text
seq.push(byte: u8) -> usize
seq.pop() -> usize
```

Append or remove one byte and return the **resulting length**. `pop` does not
return the removed byte. On an empty sequence it returns zero. Both operations
keep any explicit alphabet declaration and invalidate observed metadata when
contents change.

### `extend_from_slice`

```text
seq.extend_from_slice(bytes: &[u8])
```

Append a borrowed slice of bytes. It returns `()` and preserves any explicit
alphabet declaration.

### `clear`

```text
seq.clear()
```

Remove all bytes, cached observed metadata and any explicit alphabet declaration.
The sequence returns to automatic alphabet mode.

### `map`

```text
seq.map(function: impl Fn(u8) -> u8) -> Sequence
```

Return a new sequence obtained by applying `function` independently to every
byte. The original sequence is unchanged. A persistent alphabet declaration is
copied; otherwise the output alphabet is determined from the transformed bytes.

### `xor_with`

```text
seq.xor_with(other: &Sequence) -> Result<(), &'static str>
```

Replace each byte with `0` where the two inputs agree and `1` where they differ.
This is an equality mask, rather than bitwise XOR of arbitrary byte values.
Unequal lengths return `Err("SequenceNoMatchSize")` without changing `seq`.
An explicit alphabet declaration is retained after a successful transformation.

```rust
use lzcomplexity_core::Sequence;

let mut seq = Sequence::new();
assert_eq!(seq.push(b'A'), 1);
seq.extend_from_slice(b"CG");
let other = Sequence::from_str("ATG");
seq.xor_with(&other).unwrap();
assert_eq!(seq.as_bytes(), &[0, 1, 0]);
assert_eq!(seq.pop(), 2);
let inverted = seq.map(|byte| 1 - byte);
assert_eq!(inverted.as_bytes(), &[1, 0]);
```

## Extract or reorder bytes

### `take`, `drop` and `split_at`

```text
seq.take(length: usize) -> Sequence
seq.drop(length: usize) -> Sequence
seq.split_at(position: usize) -> (Sequence, Sequence)
```

`take` copies the prefix, `drop` copies the suffix after the requested prefix,
and `split_at` returns both. Positions beyond the length are clamped.
The derived sequences receive the original alphabet size as a constructor hint,
so an alphabet mismatch is replaced by each output's observed alphabet. In the
special `drop(n)` case with `n >= len`, the empty output retains the original
size as an explicit declaration.

### `reverse` and `reverse_copy`

```text
seq.reverse() -> &mut Sequence
seq.reverse_copy() -> Sequence
```

Reverse byte order in place, or return a reversed clone. The in-place method
returns the same mutable sequence for chaining. Alphabet information is retained,
since byte permutation does not change the observed symbols.

### `right_shift` and `left_shift`

```text
seq.right_shift(amount: u32) -> &mut Sequence
seq.left_shift(amount: u32) -> &mut Sequence
```

Rotate bytes circularly in place, reducing `amount` modulo the length.
`right_shift(1)` moves `abcd` to `bcda`; `left_shift(1)` moves it to `dabc`.
The method names therefore follow the crate's existing sequence convention.
Empty sequences and full-length rotations remain unchanged.

### `pi`

```text
seq.pi() -> &mut Sequence
```

Remove the final byte in place and return the same mutable sequence.
Calling it on an empty sequence leaves it empty.

### Concatenation with `+`

```text
&first + &second -> Sequence
```

Create a new sequence containing the bytes of `first` followed by `second`.
Neither input changes. When neither input has a persistent alphabet declaration,
the result observes the combined bytes. When at least one has a declaration,
the output declares the maximum of the two input alphabet sizes.

```rust
use lzcomplexity_core::Sequence;

let seq = Sequence::from_str("abcd");
assert_eq!(seq.take(2).as_bytes(), b"ab");
assert_eq!(seq.drop(2).as_bytes(), b"cd");
let (prefix, suffix) = seq.split_at(2);
assert_eq!((&prefix + &suffix), seq);
let mut rotated = seq.clone();
rotated.right_shift(1);
assert_eq!(rotated.as_bytes(), b"bcda");
rotated.left_shift(1).pi();
assert_eq!(rotated.as_bytes(), b"abc");
assert_eq!(seq.reverse_copy().as_bytes(), b"dcba");
```

### `granularity`

```text
seq.granularity(group_size: u32) -> Sequence
```

Replace each complete group of `group_size` bytes with its wrapping byte sum.
An incomplete final group is discarded; zero group size returns an empty
sequence. The original sequence remains unchanged. This transformation is a sum,
so distinct groups can have the same output byte.

```rust
use lzcomplexity_core::Sequence;

let seq = Sequence::from_bytes(vec![1, 2, 250, 10, 99]);
assert_eq!(seq.granularity(2).as_bytes(), &[3, 4]);
assert!(seq.granularity(0).is_empty());
```

Public sequence sampling functions are documented separately in
[building blocks](building-blocks.md#sequence-sampling-functions).
