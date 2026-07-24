//! Shared internals for the `lzcomplexity` and `lzdistance` standalone tools.

pub mod formats;
pub mod term;

/// Default output path: replace the input's final extension with `.<tag>.json`
/// (mirrors `std::filesystem::path(input).replace_extension(".<tag>.json")`).
pub fn default_output(input: &str, tag: &str) -> String {
    let path = std::path::Path::new(input);
    let parent = path.parent();
    let stem = path.file_stem().and_then(|s| s.to_str()).unwrap_or(input);
    let name = format!("{stem}.{tag}.json");
    match parent {
        Some(p) if !p.as_os_str().is_empty() => p.join(name).to_string_lossy().into_owned(),
        _ => name,
    }
}
