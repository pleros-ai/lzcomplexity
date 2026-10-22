#!/usr/bin/env python3
"""Check generated links, offline search and crawler-visible social previews."""
import hashlib
import json
import pathlib
import struct
import sys
from html.parser import HTMLParser
from urllib.parse import parse_qs, unquote, urljoin, urlparse

ROOT = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "site").resolve()
BASE = "https://pleros-ai.github.io/lzcomplexity/"

class Page(HTMLParser):
    def __init__(self, text):
        super().__init__(convert_charrefs=True)
        self.ids = set()
        self.links = []
        self.metadata = {}
        self.canonicals = []
        self.feed(text)

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if attrs.get("id"):
            self.ids.add(attrs["id"])
        for key in ("href", "src"):
            if attrs.get(key):
                self.links.append(attrs[key])
        for candidate in attrs.get("srcset", "").split(","):
            if candidate.strip():
                self.links.append(candidate.strip().split()[0])
        if tag == "meta":
            name = attrs.get("property", attrs.get("name", ""))
            self.metadata.setdefault(name, []).append(attrs.get("content", ""))
        if tag == "link" and attrs.get("rel") == "canonical":
            self.canonicals.append(attrs.get("href", ""))

pages = {path: Page(path.read_text()) for path in ROOT.rglob("*.html")}
failures = []
checked = 0

def check(source, href):
    global checked
    url = urlparse(urljoin(BASE + source.relative_to(ROOT).as_posix(), href))
    if url.netloc != "pleros-ai.github.io" or not url.path.startswith("/lzcomplexity/"):
        return
    path = ROOT / unquote(url.path[len("/lzcomplexity/"):])
    if path.is_dir():
        path /= "index.html"
    if not path.is_file():
        failures.append(f"{source.relative_to(ROOT)}: missing {href}")
    elif url.fragment and path in pages and unquote(url.fragment) not in pages[path].ids:
        failures.append(f"{source.relative_to(ROOT)}: missing anchor {href}")
    if path.is_file() and path.suffix in (".css", ".js") and "v" in parse_qs(url.query):
        expected = hashlib.sha256(path.read_bytes()).hexdigest()[:12]
        if parse_qs(url.query)["v"] != [expected]:
            failures.append(f"{source.relative_to(ROOT)}: stale asset version {href}")
    checked += 1


def check_social(source, page):
    """Social crawlers need absolute metadata in HTML, without executing JS."""
    label = source.relative_to(ROOT)
    required = (
        "description", "og:type", "og:site_name", "og:locale", "og:title",
        "og:description", "og:url", "og:image", "og:image:secure_url",
        "og:image:type", "og:image:width", "og:image:height", "og:image:alt",
        "twitter:card", "twitter:title", "twitter:description", "twitter:image",
        "twitter:image:alt",
    )
    for name in set(required) | {
        name for name in page.metadata if name.startswith(("og:", "twitter:"))
    }:
        values = page.metadata.get(name, [])
        if len(values) != 1 or not values[0].strip():
            failures.append(f"{label}: {name} must appear once with nonempty content")

    def value(name):
        return page.metadata.get(name, [""])[0]

    expected = {
        "og:type": "website",
        "twitter:card": "summary_large_image",
        "og:image:type": "image/png",
        "og:image:width": "1200",
        "og:image:height": "630",
        "og:description": value("description"),
        "twitter:description": value("description"),
        "twitter:title": value("og:title"),
        "twitter:image": value("og:image"),
        "og:image:secure_url": value("og:image"),
        "twitter:image:alt": value("og:image:alt"),
    }
    for name, expected_value in expected.items():
        if value(name) != expected_value:
            failures.append(f"{label}: incorrect or inconsistent {name}")

    # MkDocs deliberately gives the error page no canonical; regular pages must
    # keep their single canonical rather than acquiring another from our override.
    if label.as_posix() == "404.html":
        if page.canonicals:
            failures.append(f"{label}: the error page should not have a canonical")
        canonical = BASE
    else:
        if len(page.canonicals) != 1:
            failures.append(f"{label}: expected exactly one canonical link")
        canonical = page.canonicals[0] if page.canonicals else ""
    if value("og:url") != canonical:
        failures.append(f"{label}: og:url differs from the canonical URL")

    for name in ("og:url", "og:image", "og:image:secure_url", "twitter:image"):
        href = value(name)
        url = urlparse(href)
        if url.scheme != "https" or url.netloc != "pleros-ai.github.io" or not url.path.startswith("/lzcomplexity/"):
            failures.append(f"{label}: {name} must be an absolute HTTPS site URL")
        else:
            check(source, href)

    image_url = urlparse(value("og:image"))
    if (image_url.scheme != "https" or image_url.netloc != "pleros-ai.github.io"
            or not image_url.path.startswith("/lzcomplexity/")):
        return
    image_path = ROOT / unquote(image_url.path.removeprefix("/lzcomplexity/"))
    if image_path.is_file():
        header = image_path.read_bytes()[:24]
        if (image_path.suffix != ".png" or len(header) < 24
                or header[:8] != b"\x89PNG\r\n\x1a\n"
                or header[8:16] != b"\x00\x00\x00\rIHDR"
                or struct.unpack(">II", header[16:24]) != (1200, 630)):
            failures.append(f"{label}: social image must be a 1200×630 PNG")


for source, page in pages.items():
    for href in page.links:
        check(source, href)
    check_social(source, page)
search = json.loads((ROOT / "search/search_index.json").read_text())
for entry in search["docs"]:
    check(ROOT / "index.html", entry["location"])
for failure in sorted(set(failures)):
    print(failure, file=sys.stderr)
print(f"Checked {len(pages)} HTML pages, {checked} internal references, the search index and social previews.")
raise SystemExit(bool(failures))
