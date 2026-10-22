"""Version local site assets by content so Pages updates invalidate browser caches."""
import hashlib
import html
import posixpath
import re
from pathlib import Path
from urllib.parse import urlsplit, urlunsplit


def on_config(config):
    card = Path(config.docs_dir) / "assets/social-card.png"
    config.extra["social_card_version"] = hashlib.sha256(card.read_bytes()).hexdigest()[:12]
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


def on_post_page(output, page, config):
    """Refresh shared diagrams and logos when their contents change."""
    def version(url):
        parts = urlsplit(html.unescape(url))
        if parts.scheme or parts.netloc:
            return url
        relative = posixpath.normpath(posixpath.join(posixpath.dirname(page.file.dest_uri), parts.path))
        path = Path(config.docs_dir) / relative
        if not path.is_file() or path.suffix not in (".svg", ".png", ".webp"):
            return url
        digest = hashlib.sha256(path.read_bytes()).hexdigest()[:12]
        query = f"{parts.query}&v={digest}" if parts.query else f"v={digest}"
        return html.escape(urlunsplit((parts.scheme, parts.netloc, parts.path, query, parts.fragment)), quote=True)

    def image_tag(match):
        def attribute(attr):
            name, value = attr.groups()
            if name == "srcset":
                candidates = []
                for candidate in value.split(","):
                    fields = candidate.strip().split()
                    if fields:
                        candidates.append(" ".join([version(fields[0]), *fields[1:]]))
                value = ", ".join(candidates)
            else:
                value = version(value)
            return f'{name}="{value}"'
        return re.sub(r'\b(src|srcset)="([^"]+)"', attribute, match.group())

    return re.sub(r'<(?:img|source)\b[^>]*>', image_tag, output)
