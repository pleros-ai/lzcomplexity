# Versions and releases

The current production implementation lives on **`rust-backend`**. A commit merged there, a GitHub release tag, a PyPI package and a built executable are related artifacts, but they are not interchangeable identifiers.

## Know which code you are running

```python
import sys
import lzcomplexity as lz

print(lz.__version__)
print(lz.__file__)
print(sys.executable)
```

For a source checkout, also record:

```sh
git rev-parse HEAD
git status --short
```

A source fix can land before the next version is published. The version string remains unchanged until the release process bumps it, so a version alone does not identify every development build.

At this documentation audit, **2.0.0** is the latest published [PyPI package](https://pypi.org/project/lzcomplexity/2.0.0/) and [GitHub release](https://github.com/pleros-ai/lzcomplexity/releases/tag/v2.0.0). PR #14's observed-alphabet and CLI-unit corrections landed after that tag. The [installation guide](../guide/install.md) distinguishes published artifacts from current source.

## Changes that matter to analysis

| Version or source milestone | What to check when reproducing results |
|---|---|
| Historical 0.9 manual / C++ line | Old build system, class APIs, Python return shapes and defaults |
| Rust 0.11 onward | Redesigned native backend and Python interface |
| Rust 0.12 onward | Flat Python API with `h`, `nid`, `emc`, `factorization`, `lz76` |
| Rust 0.13 | Shuffle-score aggregation changed from earlier Rust releases |
| Rust 2.0.0 | The current nonnegative isotonic reporting convention changed scores and summands for nonmonotone raw profiles |
| Current source after PR #14 | Observed alphabet metadata refreshes after mutations; CLI `h` honors explicit logarithm units |

The historical changelog uses phrases such as “block-entropy estimator.” Read those as the historical descriptions of the shuffle-based EMC-like score, not an established equality with theoretical excess entropy. The [current definition](../concepts/emc.md) describes its actual calculation.

Use the [full changelog](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/CHANGELOG.md) for commit-level history. When comparing measurements across releases, check the numerical definition and defaults before treating a changed score as a changed source property.

## How a package release happens

The repository uses release-please and conventional commits. The checked-in [release configuration](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/release-please-config.json) and [workflow](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/.github/workflows/release.yml) are the authoritative operational definitions.

1. Commits merge into `rust-backend`.
2. Release-please opens or updates a release PR containing version and changelog changes.
3. Merging that release PR creates a tag and GitHub release.
4. Build jobs create Python wheels, a source distribution and standalone binaries.
5. Publishing jobs send packages to PyPI and the core crate to crates.io when their configured permissions and credentials are available.

A successful source merge is not proof that every publication job finished. Check the release's files and workflow result before announcing availability. The binary-attachment step is best-effort; the workflow also uploads build artifacts.

### Commit conventions

Use ASCII commit messages so the release parser receives predictable input.

| Commit kind | Intended semantic-version effect |
|---|---|
| A breaking-change marker, such as `feat!:` or a `BREAKING CHANGE:` footer | Major on the current stable line |
| `feat:` | Minor |
| `fix:` or `perf:` | Patch |
| Other types | Depend on the release configuration and changelog inclusion |

The configuration includes visible sections for `docs`, `build`, `ci` and `refactor`, and hides `chore`. Do not assume every non-feature commit is excluded from release generation. Explicit `Release-As` footers can override automatic version selection.

## Validation channels

The Rust workflow checks formatting, Clippy, builds, tests, core documentation and the declared minimum Rust compiler. Separate jobs run the standalone tools and build/install Python wheels on Linux, macOS and Windows.

The documentation workflow builds the site with strict link/nav checking and deploys accepted source changes to GitHub Pages. A docs build is a content/build check; it does not replace numerical tests of the library. Likewise, the library test suite does not establish scientific convergence or universal ordering of its shuffle score.

## License and attribution

The current repository is MIT-licensed; see [LICENSE](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/LICENSE). Preserve attribution and record the version or commit you used. [Authors and citation](authors.md) provides the research lineage and a citation starting point.
