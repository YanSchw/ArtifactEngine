"""Downloading and unpacking helpers used by the setup installers."""

import json
import sys
import urllib.error
import urllib.request
import zipfile
from pathlib import Path

from SDK.Job import Job
from SetupTool.Dependency import SetupError

USER_AGENT = "ArtifactSDK"
MEGABYTE = 1024 * 1024


def _request(url: str, method: str = "GET") -> urllib.request.Request:
    return urllib.request.Request(url, method=method, headers={"User-Agent": USER_AGENT})


def read_json(url: str):
    """Fetch and parse a JSON document, e.g. LunarG's SDK version index."""
    try:
        with urllib.request.urlopen(_request(url), timeout=30) as response:
            return json.loads(response.read().decode("utf-8"))
    except (urllib.error.URLError, OSError, ValueError) as error:
        raise SetupError(f"Could not query {url}: {error}")


def url_exists(url: str) -> bool:
    try:
        with urllib.request.urlopen(_request(url, method="HEAD"), timeout=20) as response:
            return 200 <= response.status < 300
    except (urllib.error.URLError, OSError):
        return False


def download_file(url: str, destination: Path, title: str):
    """Download ``url`` to ``destination``, showing progress in megabytes."""
    destination.parent.mkdir(parents=True, exist_ok=True)
    try:
        with urllib.request.urlopen(_request(url), timeout=60) as response:
            total = int(response.headers.get("Content-Length") or 0)
            label = f"{title} ({total // MEGABYTE} MB)" if total else title
            with Job(label) as job:
                downloaded = 0
                with open(destination, "wb") as output:
                    while True:
                        chunk = response.read(MEGABYTE)
                        if not chunk:
                            break
                        output.write(chunk)
                        downloaded += len(chunk)
                        job.set_progress(downloaded // MEGABYTE, total // MEGABYTE)
    except (urllib.error.URLError, OSError) as error:
        raise SetupError(f"Download failed ({url}): {error}")


def extract_zip(archive: Path, destination: Path, title: str):
    """Unpack a .zip archive.

    On macOS this shells out to ``ditto``: the LunarG archive contains an
    application bundle, and Python's zipfile drops the executable bits and
    symlinks that make the bundle runnable.
    """
    destination.mkdir(parents=True, exist_ok=True)
    with Job(title) as job:
        try:
            if sys.platform == "darwin":
                returncode = job.run(["ditto", "-x", "-k", str(archive), str(destination)])
                if returncode != 0:
                    job.fail(returncode=returncode)
                return
            with zipfile.ZipFile(archive) as archive_file:
                archive_file.extractall(destination)
        except (OSError, zipfile.BadZipFile) as error:
            raise SetupError(f"Could not unpack {archive}: {error}")
