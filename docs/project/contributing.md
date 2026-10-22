# Contributing to the documentation

The site uses MkDocs and Material for MkDocs. Documentation sources, examples,
styles and browser calculations live in this repository.

## Preview locally

```sh
python -m venv .venv-docs
source .venv-docs/bin/activate
python -m pip install -r requirements-docs.txt
mkdocs serve
```

Open the local address printed by MkDocs. On Windows, activate with
`.venv-docs\Scripts\Activate.ps1` in PowerShell.

## Check a change

```sh
mkdocs build --strict
python scripts/docs/check-links.py site
node scripts/docs/test-demo.cjs
```

These commands check navigation, links, assets, search entries, social preview
metadata and the browser engine's agreement with native results. The parity
fixtures cover factorization, h, E, distance, random draws and block swaps.
Run Python examples against an installed build of the library before submitting
changes to them.

To regenerate the numerical fixtures, run
`python scripts/docs/generate-demo-fixtures.py`. Instructions for regenerating
the random-generator fixtures appear in `scripts/docs/generate-rng-fixtures.rs`.

## Where things live

| Content | Location |
|---|---|
| First steps and result interpretation | `docs/guide/` |
| Concepts and calculations | `docs/concepts/` |
| Python and Rust references | `docs/api/` |
| Command-line tools and input formats | `docs/cli/` |
| Worked applications | `docs/recipes/` |
| Manual, citations and project information | `docs/project/` |
| Homepage | `docs/index.md`, `overrides/home.html` |
| Design and browser demonstration | `docs/assets/extra.css`, `docs/assets/site.js`, `docs/assets/measure-engine.js` |
| Social previews | `overrides/main.html`, `docs/assets/social-card.svg`, `docs/assets/social-card.png` |
| Navigation and toolchain | `mkdocs.yml`, `requirements-docs.txt` |

The browser playground accepts 256 UTF-8 bytes per sequence, uses base-two
logarithms and selects the E range automatically. Asset URLs are versioned by
content so published changes refresh cached CSS and JavaScript.

## Publishing

After merging a documentation change, publish from a clean checkout matching
`origin/rust-backend`:

```sh
python scripts/docs/publish-source.py --dry-run
python scripts/docs/publish-source.py
```

The helper mirrors the source tree to the deployment branch. The Docs workflow
then builds, checks and publishes it to [GitHub Pages](https://pleros-ai.github.io/lzcomplexity/).
Check the workflow's deployment result before announcing the update.
