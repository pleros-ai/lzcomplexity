// Regenerate tests/docs/rng-cases.json in a temporary Cargo package containing
// this file as src/main.rs. Dependencies:
//   lzcomplexity-core = { path = "/absolute/path/to/crates/lzcomplexity-core" }
//   rand = "=0.8.6"
//   rand_chacha = "=0.3.1"
//   serde_json = "1"
// Run on a 64-bit target: cargo run --quiet > /path/to/tests/docs/rng-cases.json
// The browser intentionally follows the current 64-bit Python/CLI builds.
use lzcomplexity_core::sequence::{shuffle_copy_seeded, Sequence};
use rand::{RngCore, SeedableRng};
use rand_chacha::ChaCha8Rng;
fn main() {
    let seed = 12345678901234567890u64;
    let mut rng = ChaCha8Rng::seed_from_u64(seed);
    let values: Vec<String> = (0..50).map(|_| rng.next_u64().to_string()).collect();
    let seq = Sequence::from_str("0123456789abcdefghijklmnopq");
    let shuffles: Vec<Vec<u8>> = (1..=8)
        .map(|size| {
            shuffle_copy_seeded(&seq, size, 13, seed)
                .as_bytes()
                .to_vec()
        })
        .collect();
    let mut boundaries = Vec::new();
    for n in [10, 11, 50, 51, 256] {
        let input: Vec<u8> = (0..n).map(|i| i as u8).collect();
        let seq = Sequence::from_bytes(input.clone());
        for size in [1, 2, 3, 7, 16, 127, 128, 255, 256] {
            let output = shuffle_copy_seeded(&seq, size, (n / 2) as u32, seed)
                .as_bytes()
                .to_vec();
            boundaries.push(serde_json::json!({"input":input,"size":size,"output":output}));
        }
    }
    println!(
        "{}",
        serde_json::json!({"source":"64-bit Rust lzcomplexity-core shuffle_copy_seeded; rand 0.8.6 / rand_chacha 0.3.1","seed":seed.to_string(),"u64":values,"input":"0123456789abcdefghijklmnopq","shuffles":shuffles,"boundaries":boundaries})
    );
}
