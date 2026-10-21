# Contributing to the documentation

The documentation lives beside the library on `rust-backend`. The site is generated with
MkDocs and Material for MkDocs, with a custom Pleros homepage and shared light/dark styles.
The `docs` branch is the deployment mirror, required by the repository’s existing
GitHub Pages environment policy.

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

The strict build checks navigation and Markdown links. The second command also checks
links and assets in the rendered HTML, fragment anchors and the search index. The Node
check compares the small homepage demo with committed outputs from the native library.
It is not a validation of the scientific measures. Local CSS and JavaScript URLs
are versioned by content automatically, so a new deployment refreshes cached assets.

For changes to Python examples, install the native package from the same source checkout
and execute the example. State the source revision, input encoding, logarithm base and
block-size cap when publishing numerical results. Keep historical results labelled with
their original implementation; do not turn a reference figure into a new accuracy claim.

## Where things live

| Content | Location |
|---|---|
| First steps and result interpretation | `docs/guide/` |
| Mathematical and implementation explanations | `docs/concepts/` |
| Python and Rust reference | `docs/api/` |
| Command-line tools and input formats | `docs/cli/` |
| Worked applications | `docs/recipes/` |
| Manual, citations and project information | `docs/project/` |
| Homepage | `docs/index.md`, `overrides/home.html` |
| Design and browser demonstration | `docs/assets/extra.css`, `docs/assets/site.js` |
| Navigation and toolchain | `mkdocs.yml`, `requirements-docs.txt` |

## Publishing

Pull requests and default-branch pushes build and check the site without deploying it.
The existing GitHub Pages environment permits the `docs` branch. After a reviewed
change is merged, use a clean checkout matching `origin/rust-backend`:

```sh
python scripts/docs/publish-source.py --dry-run
python scripts/docs/publish-source.py
```

The helper fetches both branches, verifies the source revision, then creates a
commit on `docs` with the **identical source tree**, preserving both histories.
It does not switch the checkout or force-push. That push triggers the Docs workflow,
which rebuilds and checks the site before publishing to
[GitHub Pages](https://pleros-ai.github.io/lzcomplexity/). Check its deployment result.
This keeps documentation edits on the current library branch while respecting the
existing deployment policy.

The original manual is available from [the manual guide](manual.md). Its older API and
installation instructions remain historical reference material.
