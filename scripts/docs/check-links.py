#!/usr/bin/env python3
"""Check generated HTML links/assets/anchors and the offline search index."""
import hashlib
import json
import pathlib
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
        self.feed(text)

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if attrs.get("id"):
            self.ids.add(attrs["id"])
        for key in ("href", "src"):
            if attrs.get(key):
                self.links.append(attrs[key])

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

for source, page in pages.items():
    for href in page.links:
        check(source, href)
search = json.loads((ROOT / "search/search_index.json").read_text())
for entry in search["docs"]:
    check(ROOT / "index.html", entry["location"])
for failure in sorted(set(failures)):
    print(failure, file=sys.stderr)
print(f"Checked {len(pages)} HTML pages, {checked} internal references and the search index.")
raise SystemExit(bool(failures))
