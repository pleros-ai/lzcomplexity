# Releases and upgrades

Python packages are available on [PyPI](https://pypi.org/project/lzcomplexity/),
the Rust crate on [crates.io](https://crates.io/crates/lzcomplexity-core), and
standalone binaries on [GitHub Releases](https://github.com/pleros-ai/lzcomplexity/releases).

## Upgrade the Python package

```sh
python -m pip install --upgrade lzcomplexity
```

Check the installed version:

```python
import lzcomplexity as lz
print(lz.__version__)
```

To pin an installation, append `==` and the required version to the package name.
Use a virtual environment to keep each project's dependencies separate.

## Command-line tools

Download the binary for your operating system and processor from
[GitHub Releases](https://github.com/pleros-ai/lzcomplexity/releases), or build
from source using the [installation guide](../guide/install.md).

## Changelog and migration

The [changelog](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/CHANGELOG.md)
lists changes to the library, APIs and command-line tools. For scripts written
against the earlier class-based interface, see the [migration reference](cpp-parity.md).
