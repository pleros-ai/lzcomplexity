# Releases

*Conventional commits in, tagged wheels out. One merge is the only manual step — here is how not to fight the rest.*

Nobody edits a version number in this repository. You write conventional commits, merge them into
`rust-backend`, and merge the release PR that appears. Everything after that — the tag, the GitHub
release, the changelog, five platform wheels, an sdist, the crates.io publish and eight standalone
binaries — is produced by GitHub Actions from those commit messages.

<div class="lz-stats">
  <div class="lz-stat"><span class="lz-stat__v">2:27</span><span class="lz-stat__k">release run (v1.0.1)</span></div>
  <div class="lz-stat"><span class="lz-stat__v">5</span><span class="lz-stat__k">platform wheels</span></div>
  <div class="lz-stat"><span class="lz-stat__v">8</span><span class="lz-stat__k">standalone binaries</span></div>
  <div class="lz-stat"><span class="lz-stat__v">0</span><span class="lz-stat__k">PyPI tokens stored</span></div>
</div>

Three files hold the whole configuration:

| File | Role |
|---|---|
| `release-please-config.json` | release type, changelog sections, which files carry the version |
| `.release-please-manifest.json` | the current version — a single entry, `{ ".": "1.0.1" }` |
| `.github/workflows/release.yml` | everything that happens after the tag exists |

The version lives in exactly two places in the source tree, both marked with a comment that
release-please keys off:

```toml
# Cargo.toml:10
version = "1.0.1" # x-release-please-version

# pyproject.toml:7
version = "1.0.1" # x-release-please-version
```

That marker is the entire mechanism keeping the Rust crate version and the Python package version
in lockstep. Do not remove it, and do not add a third copy of the version anywhere without adding
the marker and an `extra-files` entry.

`rust-backend` always carries the **last released** version, never the next one. The release PR
writes the new number into all three files, and merging that PR is what creates the tag — so
between releases the tree matches whatever is newest on PyPI. At the time of writing that is
`1.0.1` in all three places.

<hr class="lz-tickrule">

## The bump table — post-1.0

The project shipped 1.0.0 on 2026-07-25. From that point release-please's default versioning
strategy applies with no pre-1.0 modifiers: `bump-minor-pre-major` and
`bump-patch-for-minor-pre-major` were both **deleted** from `release-please-config.json` in commit
`533b285`, and at 1.0.x they would have been ignored anyway.

Two things are decided independently — how much the version moves, and whether a release happens
at all. A commit type with no configured changelog section renders to nothing, and release-please
skips a release whose entire changelog body would be empty.

<div class="lz-scroll" markdown>

| Commit prefix | Bump | Changelog section | Releases on its own? |
|---|---|---|---|
| any type with `!` (`feat!:`, `fix!:`, `perf!:`, …) | **MAJOR** — `1.0.1 → 2.0.0` | its own section, plus `⚠ BREAKING CHANGES` | yes |
| any type with a `BREAKING CHANGE:` footer | **MAJOR** | idem | yes |
| `feat:` | **MINOR** — `1.0.1 → 1.1.0` | Features | yes |
| `fix:` | **PATCH** — `1.0.1 → 1.0.2` | Bug Fixes | yes |
| `perf:` | PATCH | Performance | yes |
| `refactor:` | PATCH | Refactoring | yes |
| `docs:` | PATCH | Documentation | yes |
| `build:` | PATCH | Build System | yes |
| `ci:` | PATCH | CI | yes |
| `chore:` | PATCH, only if it rides along with something else | hidden | **no** |
| `test:`, `revert:`, anything else parseable | PATCH, only if it rides along | none configured | **no** |

</div>

!!! danger

    A `!` or a `BREAKING CHANGE:` footer on any commit merged to `rust-backend` now cuts a **2.0.0**,
    not a minor. Before 1.0 the same commit produced a minor bump.

    `README.md` carries a shortened copy of this table, and the two disagree on one point: the
    README lists `refactor:`, `docs:`, `build:` and `ci:` under "no release on its own". All four
    have visible changelog sections in `release-please-config.json`, so each clears the
    empty-changelog gate by itself. The README is right about `test:` and `chore:`. Where they
    disagree, this page is the current one.

A `Release-As: X.Y.Z` footer overrides everything above and pins the exact version, whatever the
commit types in the range would otherwise have produced.

!!! warning

    Three of the five releases on this branch were pinned with `Release-As:`, so the release history
    is not evidence of what the automatic rules do. `v0.13.0`, `v1.0.0` and `v1.0.1` all carry the
    footer; `0.13.0 → 1.0.0` in particular is a major jump that no `feat:` commit could have
    produced. If you are inferring the bump rules from `git log`, you will infer them wrong. Read
    the table above instead.

<hr class="lz-tickrule">

## The pipeline, end to end

`release.yml` triggers on pushes to `rust-backend` only, plus `workflow_dispatch`. The workflow
declares `permissions: {}` at the top level; three jobs add back exactly what they need —
`release-please` (`contents: write`, `pull-requests: write`), `publish-pypi` (`id-token: write`)
and `build-binaries` (`contents: write`). The other three run with no token permissions at all.

```text
conventional commits merged to rust-backend
      │
      ▼
 [1] release-please                       googleapis/release-please-action@v4
     contents:write, pull-requests:write   target-branch: rust-backend
      │
      ├─ opens / keeps updating a release PR on branch
      │  release-please--branches--rust-backend--components--lzcomplexity
      │  bumping Cargo.toml, pyproject.toml, .release-please-manifest.json
      │  and rewriting CHANGELOG.md
      │
      └─ when that PR merges: tag vX.Y.Z + GitHub release,
         release_created = "true"
      │
      ├───────────────┬──────────────────┬───────────────────┐
      ▼               ▼                  ▼                   ▼
 [2] build-wheels  [2] build-sdist   [3b] publish-crates  [4] build-binaries
     5 platforms       maturin sdist       cargo publish        4 targets
     maturin-action    maturin-action      -p lzcomplexity-     upload-artifact
     sccache: true                          core                + gh release upload
      │               │                                          (best-effort)
      └───────┬───────┘
              ▼
       [3] publish-pypi
           id-token: write
           pypa/gh-action-pypi-publish@release/v1
           OIDC trusted publishing — no API token in the repo
```

Step by step:

1. **Merge conventional commits** into `rust-backend`. Nothing is published yet.
2. **release-please maintains a release PR.** It accumulates every unreleased commit, computes the
   next version, and rewrites `CHANGELOG.md`. Leave it open as long as you like; it updates itself
   on every push.
3. **Merging the release PR** creates the tag `vX.Y.Z` and the GitHub release, and sets
   `release_created`. That is the only manual decision in the whole flow.
4. **Wheels and sdist** are built by `PyO3/maturin-action@v1` across five targets and uploaded as
   workflow artifacts.
5. **PyPI** receives them through `pypa/gh-action-pypi-publish` using OIDC trusted publishing. There
   is no PyPI API token anywhere in the repository or its secrets.
6. **crates.io** receives `lzcomplexity-core` via `cargo publish`. This one *does* need a secret:
   `CARGO_REGISTRY_TOKEN`. Only the core crate is published — `lzcomplexity-py` and
   `lzcomplexity-cli` are not on crates.io. See [the Rust crate page](../api/rust.md).
7. **Standalone binaries** are built for four targets, always uploaded as workflow artifacts, and
   *best-effort* attached to the GitHub release.

!!! note

    The trusted publisher on PyPI is keyed on the repository and the **workflow filename**
    (`release.yml`), not on a branch. Renaming the workflow file breaks publishing until the PyPI
    setting is updated to match. The previous C++ setup used `wheels.yml`.

### Binary attachment is best-effort, artifacts are not

The `gh release upload` step ends with `|| echo "::warning::…"`. If the organisation enables
immutable releases, adding assets after the release exists returns HTTP 422 and the step degrades to
a warning instead of failing the run. The `upload-artifact` steps use `if-no-files-found: error`, so
the workflow artifacts are the guaranteed channel and the release assets are the convenient one. On
`v1.0.1` the release was mutable and all eight assets attached.

### Measured, on the real v1.0.1 run

Workflow run `30157145738`, 2026-07-25. All thirteen jobs succeeded.

| Job | Duration |
|---|---|
| release-please | 7 s |
| Source distribution | 16 s |
| Publish core crate to crates.io | 21 s |
| Wheel • macos-14 • aarch64 | 26 s |
| Wheel • macos-14 • x86_64 | 34 s |
| Wheel • windows-latest • x64 | 62 s |
| Wheel • ubuntu-latest • x86_64 | 73 s |
| Wheel • ubuntu-latest • aarch64 | 96 s |
| Binaries • x86_64-unknown-linux-gnu | 31 s |
| Binaries • aarch64-apple-darwin | 36 s |
| Binaries • x86_64-apple-darwin | 41 s |
| Binaries • x86_64-pc-windows-msvc | 96 s |
| Publish to PyPI | 36 s |

**Total wall clock: 2 min 27 s** for the run, 11:57:59 → 12:00:26 UTC. PyPI records the first
`1.0.1` file at 12:00:14 UTC — 2 min 15 s after the merge push.

<hr class="lz-tickrule">

## What CI runs on every push and PR

`.github/workflows/ci.yml` triggers on `push` to `branches: ["**"]` and on every `pull_request`,
with `cancel-in-progress` concurrency per ref. Three jobs:

| Job | Runner | What it does |
|---|---|---|
| `rust` | `ubuntu-latest` | `cargo fmt --all --check`, `cargo clippy --workspace --all-targets -- -D warnings`, `cargo build --workspace --release`, `cargo test --workspace --release` |
| `binaries` | `ubuntu-latest` | builds `lzcomplexity-cli`, runs both binaries on a two-line sample, asserts `lz76Complexity == 9` and a two-entry `information_distance` |
| `wheel` | `ubuntu-latest`, `macos-latest`, `windows-latest` | `maturin build --release`, installs the wheel with `--no-index`, then runs a Python API smoke test |

The wheel job's Python smoke test asserts both what must exist and what must stay removed:

```python
import lzcomplexity as lz

c, f = lz.factorization("banana")
assert (c, f) == (3, [0, 1, 2, 3, 7])
assert abs(lz.h("01010101") - 0.75) < 1e-12
val, summ = lz.emc("01001010101101010101110101010101010000100101011")
assert isinstance(summ, list) and len(summ) >= 1
full = lz.lz76("ABRACADABRA")
assert full["complexity"] == 5 and "h" in full and "emc" in full and "extras" in full
assert abs(lz.nid("abcd", "abce") - 0.25) < 1e-12
assert not hasattr(lz, "spectral")        # removed surface must stay removed
assert not hasattr(lz, "metrics")
assert not hasattr(lz, "entropy_density") # renamed to h
```

The `--no-index` on the install is deliberate: it keeps pip away from PyPI, where an older build may
live, so the locally built wheel is the only install candidate.

!!! warning

    An accidental MSRV bump ships silently — CI never checks it. `rust-version = "1.75"` is declared
    in `Cargo.toml` and recorded on crates.io, but every CI job uses
    `dtolnay/rust-toolchain@stable` and there is no 1.75 job. CI also runs no `cargo audit`, no
    coverage, no benchmarks, and no differential test against the C++ `main` branch — that
    comparison was done by hand and is written up in [Rust vs C++](cpp-parity.md).

`cargo test --workspace` is 22 tests and 0 doc-tests: 11 in `lzcomplexity-core` (suffix array, LCP,
LPF, sequence handling, the EMC block-entropy formula) and 11 in `lzcomplexity-cli` (the format
readers). There is no unit test for `factorize` or for the PyO3 layer; those are covered only by the
two smoke tests above.

<hr class="lz-tickrule">

## Two gotchas that have already bitten

### Keep commit messages ASCII-only

!!! warning

    A commit whose message contains Unicode mathematical symbols can vanish from the changelog with
    no error anywhere. It happened here. Commit `37ef34c` — `feat!: new block-entropy estimator for
    EMC`, carrying a `BREAKING CHANGE:` footer — landed on `rust-backend` before `v0.13.0`. Its body
    contained `Σ`, `Δ`, `ĥ`, `Ê` and `·`. It appears **nowhere** in the `0.13.0` changelog entry:
    not under Features, not under `⚠ BREAKING CHANGES`. The release-please job was green.

The fix commit, `d16939d`, records the diagnosis in its own body: the previous commit
*"used Unicode math symbols in its message that release-please could not parse."* It restated the
same change in plain ASCII, and that restatement is what the `0.13.0` changelog actually shows.

The failure is not "any non-ASCII byte anywhere". An em dash in a *subject* line survived — commit
`8fe9ac6`, `feat!: flatten the API — top-level nid and h, drop the metrics submodule`, renders
correctly in the `0.12.0` changelog, em dash intact. But there is no way to tell in advance which
characters will survive, and the failure is silent, so the rule is blunt:

```text
Write commit subjects and bodies in ASCII. Spell the mathematics out:

  good:  H_l - H_{l-1};  sum over l = 1..mm of (gain - h_hat)
  bad:   ΔH_l = H_l - H_{l-1};  Ê = Σ_{l=1}^{mm} (ΔH_l - ĥ)
```

Prose about the mathematics belongs in these docs, where it renders. A commit message only has to be
parseable.

### The repository must allow Actions to create pull requests

!!! warning

    If the release PR never appears, the cause is almost always a repository setting rather than the
    workflow. **Settings → Actions → General → Workflow permissions → "Allow GitHub Actions to
    create and approve pull requests"** must be enabled. Without it the `release-please` job still
    runs and still reports success; it cannot open the PR, so nothing is ever released and
    the version never moves. This is a repository setting — it is not in the tree, and no amount of
    editing `release.yml` will fix it.

`release.yml` already grants the job the token permissions it needs (`contents: write`,
`pull-requests: write`). The repository-level toggle is a separate gate on top of those.

<hr class="lz-tickrule">

## Manual recovery — `workflow_dispatch`

`release.yml` accepts a manual run. Use it to rebuild and publish the **current** version without
waiting on release-please — recovery after a partial failure, or a first bootstrap publish.

<div class="lz-run" markdown>

```console
$ gh workflow run release.yml --ref rust-backend
```

</div>

What a manual run does and does not do:

| Job | On `release_created` | On `workflow_dispatch` |
|---|---|---|
| `build-wheels` | runs | runs |
| `build-sdist` | runs | runs |
| `publish-pypi` | runs | runs |
| `build-binaries` | runs | runs |
| `publish-crates` | runs | **does not run** |

`publish-crates` is gated on `release_created` alone, deliberately: a manual re-publish would hit a
duplicate-version error on crates.io. If you need to re-publish the crate, do it from a workstation
with `cargo publish -p lzcomplexity-core`.

!!! warning

    `publish-pypi` got no such gate, and the publish step sets no `skip-existing`. Dispatching the
    workflow at a version whose files PyPI already holds uploads duplicates and fails the job. Use
    `workflow_dispatch` when the version exists but its artifacts do not — a bootstrap publish, or
    recovery from a run that died before `publish-pypi` — not to re-ship a release that already
    completed.

The binaries job has its own fallback for the tag name. In manifest mode release-please exposes the
tag on a path-prefixed output (`.--tag_name`), the bare `tag_name` being empty, so the workflow
reads both; if the result is still empty it asks `gh release list --limit 1`, and failing that names
the assets `dev`.

<hr class="lz-tickrule">

## Dependabot

`.github/dependabot.yml` declares two ecosystems, both weekly at `/`:

| Ecosystem | Commit prefix | Effect once merged into `rust-backend` |
|---|---|---|
| `github-actions` | `ci` | PATCH release |
| `cargo` | `build` | PATCH release |

The config comment gives the motivation plainly: this is what let the C++ `cibuildwheel` pin rot
into a broken CI.

!!! warning

    That configuration is not in effect yet. Dependabot reads `.github/dependabot.yml` from the
    repository's **default branch**, which here is `main` — the C++ branch. The file exists only on
    `rust-backend` and `docs`, and no Dependabot PR has been opened against the repository so far.
    Switching it on means putting the file on `main` *and* adding `target-branch: rust-backend`, so
    the PRs land on the branch the release pipeline watches. Python's `pip` ecosystem is not
    declared at all, so the docs toolchain is outside its scope either way.

!!! note

    Once it is on, every merged Dependabot PR produces a patch release. `ci:` and `build:` both
    fall through to a patch bump *and* both have visible changelog sections, so each one clears the
    empty-changelog gate on its own. Had the prefixes been `chore`, nothing would be released. That
    is a deliberate trade: a noisier version history in exchange for shipped dependency updates. To
    batch them, hold the Dependabot PRs and merge several at once — release-please folds them into
    one release.

<hr class="lz-tickrule">

## How this documentation is published

The site is built and deployed from the **`docs` branch** by `.github/workflows/docs.yml` — a
separate workflow from `release.yml`, on a separate branch, with its own trigger paths — `docs/**`,
`mkdocs.yml`, `requirements-docs.txt`, and, on pushes, the workflow file itself. It also accepts
`workflow_dispatch`.

```text
push to docs branch  ──►  build:  pip install -r requirements-docs.txt
                                  mkdocs build --strict
                                  upload-pages-artifact
                          │
                          ▼
                          deploy: configure-pages (enablement: true)
                                  deploy-pages  ──►  GitHub Pages
```

Pull requests build the site but never deploy — the `deploy` job carries
`if: github.event_name != 'pull_request'`. The build runs `mkdocs build --strict`, which turns a
broken internal link or a bad nav entry into a failed job, so a typo in a cross-reference cannot
ship. `configure-pages` is called with `enablement: true`, which creates the Pages site on the first
run; there is no manual repository-settings step for Pages.

The toolchain is pinned to majors in `requirements-docs.txt`:

```text
mkdocs~=1.6
mkdocs-material~=9.5
pymdown-extensions~=10.9
```

The file's own comment says the pins exist so Dependabot can bump them, but `dependabot.yml`
declares no `pip` ecosystem — these are bumped by hand. What does hold is the second half: the PR
build runs `mkdocs build --strict`, so a toolchain change that breaks the site fails before it
deploys.

!!! note

    Documentation changes do not go through the release pipeline and do not bump the version —
    unless you also merge them into `rust-backend` as a `docs:` commit, which does. `v1.0.1` came
    from exactly one such commit — pinned with `Release-As: 1.0.1`, though a bare `docs:` would have
    bumped the patch on its own.

<hr class="lz-tickrule">

## Release history on this branch

| Version | Trigger | How the number was chosen |
|---|---|---|
| 0.11.0 | `feat!: rewrite the library backend in Rust with a redesigned Python API` | automatic, under the pre-1.0 rules then in force |
| 0.12.0 | `feat!: flatten the API — top-level nid and h, drop the metrics submodule` | automatic, same pre-1.0 rules |
| 0.13.0 | `feat!: emc uses the block-entropy estimator` + `perf: linear-time suffix array` | **`Release-As: 0.13.0`** — the automatic result would also have been 0.13.0 |
| 1.0.0 | `feat: declare a stable 1.0 public API` + `build:` + `ci:` | **`Release-As: 1.0.0`** — a deliberate 0.13 → 1.0 jump; the automatic result would have been 0.14.0 |
| 1.0.1 | `docs: add a README for the core crate` | **`Release-As: 1.0.1`** |

The release commit is always titled `chore(rust-backend): release X.Y.Z (#N)`.

`0.13.0` is the only release *within* the Rust line that moved a number: `emc()` and
`lz76()["emc"]` switched to the block-entropy estimator. Factorization, entropy density and
information distance were untouched by it. See
[Effective measure complexity](../concepts/emc.md) for what the current estimator computes, and why
only the largest block size contributes to the total.

`0.11.0` moved numbers too, but against the C++ `0.10.2` it replaced rather than against another
Rust release: `emc` draws different surrogates under the new RNG, and `h` on non-binary input
changed convention with the new `log_base` default. Both are laid out in
[Rust vs C++](cpp-parity.md).

Eleven versions have been published to PyPI in all: 0.9.12, 0.9.13, 0.9.14, 0.9.15, 0.10.1, 0.10.2,
0.11.0, 0.12.0, 0.13.0, 1.0.0, 1.0.1. Everything from 0.11.0 onward is the Rust implementation.
Installing from each channel is covered in [Install](../guide/install.md).
