#!/usr/bin/env python3
"""Version and metadata preparation shared by local and GitHub release builds."""

import argparse
from datetime import date
import json
import os
from pathlib import Path
import re
import subprocess
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
APP_ID = "dev.filimonov.klogg"


def build_version(base, ref_type, ref_name, run_number):
    if not re.fullmatch(
        r"(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)", base
    ):
        raise ValueError(
            "VERSION must contain three numeric components without leading zeros"
        )
    if ref_type == "tag":
        if ref_name != f"v{base}":
            raise ValueError(
                f"Release tag must match VERSION: expected v{base}, got {ref_name}"
            )
        return f"{base}.0"
    if not run_number.isdecimal():
        raise ValueError("CI run number must be numeric")
    return f"{base}.{int(run_number)}"


def prepare(tag, output, repository):
    import yaml  # Only metadata generation needs PyYAML; CI versioning does not.

    if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", repository):
        raise ValueError("Repository must have the form OWNER/REPO")
    base = (ROOT / "VERSION").read_text().strip()
    version = build_version(base, "tag", tag, "0")
    commit = subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True
    ).strip()
    release_date = subprocess.check_output(
        ["git", "show", "-s", "--format=%cs", "HEAD"], cwd=ROOT, text=True
    ).strip()
    date.fromisoformat(release_date)
    release_url = f"https://github.com/{repository}/releases/tag/{tag}"
    output.mkdir(parents=True, exist_ok=True)
    (output / "source.json").write_text(
        json.dumps(
            {
                "repository": repository,
                "tag": tag,
                "commit": commit,
                "version": version,
            },
            indent=2,
        )
        + "\n"
    )

    # Keep the checked-in manifest useful for local builds. The generated manifest
    # stays beside it so its relative source paths resolve to this exact checkout.
    manifest = yaml.safe_load((ROOT / f"{APP_ID}.yaml").read_text())
    module = next(module for module in manifest["modules"] if module["name"] == "klogg")
    module["config-opts"].append(f"-DBUILD_VERSION={version}")
    (ROOT / f"{APP_ID}.release.yaml").write_text(
        yaml.safe_dump(manifest, sort_keys=False)
    )

    metainfo_path = ROOT / "packaging" / "flatpak" / f"{APP_ID}.metainfo.xml"
    tree = ET.parse(metainfo_path)
    component = tree.getroot()
    releases = component.find("releases")
    if releases is not None:
        component.remove(releases)
    releases = ET.SubElement(component, "releases")
    release = ET.SubElement(
        releases, "release", {"version": version, "date": release_date}
    )
    ET.SubElement(release, "url").text = release_url
    ET.indent(tree, space="  ")
    tree.write(metainfo_path, encoding="utf-8", xml_declaration=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    subcommands = parser.add_subparsers(dest="command", required=True)
    subcommands.add_parser("version")
    metadata = subcommands.add_parser("prepare")
    metadata.add_argument("--tag", required=True)
    metadata.add_argument("--repository", default=os.environ.get("GITHUB_REPOSITORY"))
    metadata.add_argument("--output", type=Path, default=Path("release"))
    args = parser.parse_args()
    try:
        if args.command == "version":
            print(
                build_version(
                    (ROOT / "VERSION").read_text().strip(),
                    os.environ.get("GITHUB_REF_TYPE", "branch"),
                    os.environ.get("GITHUB_REF_NAME", ""),
                    os.environ.get("GITHUB_RUN_NUMBER", "0"),
                )
            )
        else:
            if not args.repository:
                parser.error(
                    "prepare requires --repository OWNER/REPO or GITHUB_REPOSITORY"
                )
            prepare(args.tag, args.output, args.repository)
    except ValueError as error:
        parser.exit(1, f"{error}\n")


if __name__ == "__main__":
    main()
