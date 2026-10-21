#!/usr/bin/env python3
"""Publish the checked default-branch tree through the permitted Pages branch.

No force push and no checkout changes: the new docs commit preserves both
histories and uses exactly the source tree already merged to rust-backend.
"""
import argparse
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--dry-run", action="store_true")
args = parser.parse_args()

def git(*command, input=None):
    return subprocess.check_output(["git", *command], input=input, text=True).strip()

if git("status", "--porcelain"):
    raise SystemExit("Use a clean checkout of the merged rust-backend source.")
subprocess.run(["git", "fetch", "origin", "rust-backend", "docs"], check=True)
source = git("rev-parse", "HEAD")
if source != git("rev-parse", "origin/rust-backend"):
    raise SystemExit("HEAD must match the current origin/rust-backend commit.")
previous = git("rev-parse", "origin/docs")
tree = git("rev-parse", "HEAD^{tree}")
if tree == git("rev-parse", "origin/docs^{tree}"):
    print("The deployment branch already contains this source tree.")
elif args.dry_run:
    print(f"Would publish source {source} on docs, preserving parent {previous}.")
else:
    message = f"docs: publish current documentation from {source[:12]}\n"
    commit = git("commit-tree", tree, "-p", previous, "-p", source, input=message)
    subprocess.run(["git", "push", "origin", f"{commit}:refs/heads/docs"], check=True)
    print(f"Published source {source} as docs commit {commit}. Check the Docs workflow deployment.")
