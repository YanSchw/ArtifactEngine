"""Packaging for the Web target.

The emscripten link already produced everything the browser needs (Artifact.js, the wasm module and
the preloaded content in Artifact.data); packaging gathers them into one directory and writes the
page that hosts the canvas. The result is static, so any HTTP server can serve Dist/Artifact.
"""

import shutil
from pathlib import Path

from SDK.Util import smart_open

APP_NAME = "Artifact"
OUTPUT_DIR = Path("Dist") / APP_NAME
BINARIES = Path("Binaries")

PAGE = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
<title>{name}</title>
<style>
  html, body {{ margin: 0; height: 100%; overflow: hidden; background: #14161a; }}
  #canvas {{ display: block; width: 100%; height: 100%; border: 0; outline: none; }}
  #status {{
    position: absolute; inset: 0; display: flex; align-items: center; justify-content: center;
    color: #8b93a1; font: 14px/1.4 system-ui, sans-serif; pointer-events: none;
  }}
</style>
</head>
<body>
<canvas id="canvas" tabindex="-1" oncontextmenu="event.preventDefault()"></canvas>
<div id="status">Loading…</div>
<script>
  const status = document.getElementById('status');
  var Module = {{
    canvas: document.getElementById('canvas'),
    print: (text) => console.log(text),
    printErr: (text) => console.error(text),
    setStatus: (text) => {{ status.textContent = text; status.style.display = text ? 'flex' : 'none'; }},
    onRuntimeInitialized: () => Module.setStatus(''),
  }};
  Module.canvas.focus();
</script>
<script async src="{name}.js"></script>
</body>
</html>
"""


def package_for_web(project_path: str):
    output = Path(project_path) / OUTPUT_DIR
    if output.exists():
        shutil.rmtree(output)
    output.mkdir(parents=True)

    binaries = Path(project_path) / BINARIES
    for suffix in (".js", ".wasm", ".data"):
        artifact = binaries / f"{APP_NAME}{suffix}"
        if not artifact.exists():
            raise FileNotFoundError(f"The emscripten build produced no {artifact.name}")
        shutil.copy2(artifact, output / artifact.name)

    with smart_open(str(output / "index.html")) as page:
        page.write(PAGE.format(name=APP_NAME))

    print(f"\n✔ Web build created: {output}")
    print(f"  Serve it with:  python3 -m http.server --directory {output}")
