"""Version local site assets by content so Pages updates invalidate browser caches."""
import hashlib
from pathlib import Path
from urllib.parse import urlsplit, urlunsplit


def on_config(config):
    for key in ("extra_css", "extra_javascript"):
        versioned = []
        for asset in config[key]:
            parts = urlsplit(asset)
            path = Path(config.docs_dir) / parts.path
            if not parts.scheme and not parts.netloc and path.is_file():
                digest = hashlib.sha256(path.read_bytes()).hexdigest()[:12]
                query = f"{parts.query}&v={digest}" if parts.query else f"v={digest}"
                asset = urlunsplit((parts.scheme, parts.netloc, parts.path, query, parts.fragment))
            versioned.append(asset)
        config[key] = versioned
    return config
