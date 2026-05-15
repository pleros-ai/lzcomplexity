//! Spectral analysis: PSD, spectral entropy, effective spectral complexity.
//!
//! Port of `modules/core/src/spectral.cpp`. Uses `rustfft` in place of pocketfft.

use rand::SeedableRng;
use rand_chacha::ChaCha8Rng;
use rayon::prelude::*;
use rustfft::num_complex::Complex64;
use rustfft::FftPlanner;

use crate::shuffle::max_block_size;

#[derive(Clone, Debug)]
pub struct Signal {
    pub samples: Vec<f64>,
    pub sample_rate: usize,
    pub use_complex: bool,
    pub use_window: bool,
    pub cut: bool,
    pub window: String,
}

/// Generalised cosine window: `w[i] = sum_k a[k] * cos(k * t[i])` with
/// `t = linspace(-pi, pi, M)`.
fn general_cosine(m: usize, coeffs: &[f64]) -> Vec<f64> {
    if m <= 1 {
        return vec![1.0; m];
    }
    let mut w = vec![0.0; m];
    let step = (2.0 * std::f64::consts::PI) / (m as f64 - 1.0);
    for i in 0..m {
        let t = -std::f64::consts::PI + step * i as f64;
        for (k, a) in coeffs.iter().enumerate() {
            w[i] += a * (k as f64 * t).cos();
        }
    }
    w
}

fn window_named(name: &str, m: usize) -> Vec<f64> {
    let alpha = match name {
        "hamming" | "hamm" | "ham" => 0.54,
        _ => 0.5, // hann/han
    };
    general_cosine(m, &[alpha, 1.0 - alpha])
}

/// Compute the PSD of `signal` (no segmentation here — see [`process_signal`]).
fn psd_single(signal: &[f64], use_complex: bool, scale: f64) -> Vec<f64> {
    let n = signal.len();
    if n == 0 {
        return Vec::new();
    }
    let mut planner = FftPlanner::<f64>::new();
    let fft = planner.plan_fft_forward(n);
    let mut buf: Vec<Complex64> = if use_complex {
        signal.iter().map(|&x| Complex64::new(x, 0.0)).collect()
    } else {
        // r2c equivalent — same output length here since we keep the full buffer.
        signal.iter().map(|&x| Complex64::new(x, 0.0)).collect()
    };
    fft.process(&mut buf);
    buf.iter().map(|z| (z.norm_sqr()) * scale).collect()
}

/// Apply a window in-place.
fn apply_window(signal: &mut [f64], win_name: &str) {
    let win = window_named(win_name, signal.len());
    for (s, w) in signal.iter_mut().zip(win.iter()) {
        *s *= *w;
    }
}

/// `process_signal` — return the PSD (averaged across segments when `signal.cut`).
pub fn process_signal(signal: &Signal, step: usize) -> Vec<f64> {
    let scale = 1.0 / (signal.sample_rate as f64).powi(2);
    let do_segment = |seg: &[f64]| -> Vec<f64> {
        let mut buf = seg.to_vec();
        if signal.use_window {
            apply_window(&mut buf, &signal.window);
        }
        psd_single(&buf, signal.use_complex, scale)
    };

    if !signal.cut || signal.sample_rate == 0 {
        return do_segment(&signal.samples);
    }

    let mut segments: Vec<Vec<f64>> = Vec::new();
    let sr = signal.sample_rate;
    let n = signal.samples.len();
    if n <= sr {
        return do_segment(&signal.samples);
    }
    let mut i: usize = 0;
    while i + 1 < (n - sr) / sr {
        let start = i * sr;
        let end = (i + 1) * sr;
        if end > n {
            break;
        }
        segments.push(do_segment(&signal.samples[start..end]));
        i += step.max(1);
    }
    if segments.is_empty() {
        return do_segment(&signal.samples);
    }
    column_means(&segments)
}

fn column_means(rows: &[Vec<f64>]) -> Vec<f64> {
    if rows.is_empty() {
        return Vec::new();
    }
    let cols = rows[0].len();
    let mut out = vec![0.0; cols];
    for r in rows {
        for j in 0..cols.min(r.len()) {
            out[j] += r[j];
        }
    }
    let n = rows.len() as f64;
    for v in &mut out {
        *v /= n;
    }
    out
}

/// Compute the spectral entropy of an already-computed PSD vector.
pub fn spectral_entropy_of_psd(psd: &[f64]) -> f64 {
    if psd.is_empty() {
        return 0.0;
    }
    let half = psd.len() / 2;
    if half == 0 {
        return 0.0;
    }
    let acc: f64 = psd[..half].iter().copied().sum();
    if acc <= 0.0 {
        return 0.0;
    }
    let mut res = 0.0;
    for &x in &psd[..half] {
        let p = x / acc;
        if p > 0.0 {
            res += p * p.log2();
        }
    }
    -res
}

pub fn spectral_entropy(signal: &Signal, step: usize) -> f64 {
    spectral_entropy_of_psd(&process_signal(signal, step))
}

/// Effective spectral complexity. Uses a deterministic seed per block size.
pub fn effective_spectral_complexity(
    signal: &Signal,
    block_size: i32,
    step: usize,
    change_shuffle: bool,
) -> f64 {
    let entropy = spectral_entropy(signal, step);

    // Determine mm
    let signal_len = signal.samples.len();
    let mm: i64 = if block_size <= 0 {
        let mut x = max_block_size(signal_len) as i64;
        if signal_len > 50 {
            x += 10;
        }
        x
    } else {
        block_size as i64
    };
    let mm = mm as usize;

    let h_rand: Vec<f64> = (1..=mm)
        .into_par_iter()
        .map(|idx| {
            let mut rng = ChaCha8Rng::seed_from_u64((idx as u64).wrapping_mul(0x9E3779B97F4A7C15));
            let rand_samples = block_shuffle_doubles(&signal.samples, idx as u32, (signal_len / 2) as u32, &mut rng);
            if change_shuffle {
                // shuffle the PSD instead
                let mut psd = process_signal(signal, step);
                let _ = rand_samples;
                let times = (signal_len / 2) as u32;
                for _ in 0..times {
                    shuffle_doubles_in_place(&mut psd, idx as u32, &mut rng);
                }
                spectral_entropy_of_psd(&psd)
            } else {
                let mut shuffled = signal.clone();
                shuffled.samples = rand_samples;
                spectral_entropy(&shuffled, step)
            }
        })
        .collect();

    let mut sum = 0.0;
    for h in &h_rand {
        sum += h - entropy;
    }
    sum
}

fn block_shuffle_doubles(v: &[f64], block_size: u32, times: u32, rng: &mut ChaCha8Rng) -> Vec<f64> {
    let mut out = v.to_vec();
    for _ in 0..times {
        shuffle_doubles_in_place(&mut out, block_size, rng);
    }
    out
}

fn shuffle_doubles_in_place(v: &mut [f64], block_size: u32, rng: &mut ChaCha8Rng) {
    use rand::Rng;
    let n = v.len();
    let bs = block_size as usize;
    if bs == 0 || n <= bs + 1 {
        return;
    }
    let max_idx = (n - bs - 1) / bs;
    if max_idx == 0 {
        return;
    }
    let mut op1 = bs * rng.gen_range(0..=max_idx);
    while op1 + bs > n.saturating_sub(1) {
        op1 = bs * rng.gen_range(0..=max_idx);
    }
    if n <= 10 {
        return;
    }
    loop {
        let op2 = bs * rng.gen_range(0..=max_idx);
        let no_overlap = op2 + bs <= op1 || op2 >= op1 + bs;
        let in_bounds = op2 + bs <= n.saturating_sub(1);
        if no_overlap && in_bounds {
            for i in 0..bs {
                v.swap(op1 + i, op2 + i);
            }
            return;
        }
    }
}
