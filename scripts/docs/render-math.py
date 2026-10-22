"""Typeset documentation equations at build time, with no browser dependency."""
import html
import re

from latex2mathml.converter import convert
from mkdocs.exceptions import PluginError


MATH = re.compile(r'<(span|div) class="arithmatex">(.*?)</\1>', re.DOTALL)


def on_page_content(content, page, config, files):
    def render(match):
        tag, source = match.groups()
        source = html.unescape(source).strip()
        delimiters = (r"\[", r"\]") if tag == "div" else (r"\(", r"\)")
        if not (source.startswith(delimiters[0]) and source.endswith(delimiters[1])):
            raise PluginError(f"Invalid equation delimiters in {page.file.src_uri}")
        source = source[2:-2].strip()
        try:
            math = convert(source, display="block" if tag == "div" else "inline")
        except Exception as error:
            raise PluginError(f"Cannot typeset equation in {page.file.src_uri}: {source}") from error
        return f'<{tag} class="arithmatex">{math}</{tag}>'

    return MATH.sub(render, content)
