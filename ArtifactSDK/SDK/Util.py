import os
import io
import re
from contextlib import contextmanager
from pathlib import Path
from PIL import Image

def needs_regeneration(file_path: str, file_content: str, encoding: str = "utf-8") -> bool:
    """
    Returns True if:
      - The resource does not exist, OR
      - The file_content is different from the resource.
    """
    # If the file doesn't exist, we definitely need to regenerate
    if not os.path.exists(file_path):
        return True

    # If the file exists, check if its content matches the expected content
    try:
        with open(file_path, "r", encoding=encoding) as f:
            existing_content = f.read()
        return existing_content != file_content
    except FileNotFoundError:
        # If we can't read the existing file, it's a regeneration case
        return True
    
@contextmanager
def smart_open(file_path: str, encoding: str = "utf-8"):
    buffer = io.StringIO()

    # Yield the in-memory file-like object
    yield buffer

    new_content = buffer.getvalue()
    buffer.close()

    if needs_regeneration(file_path, new_content, encoding):
        os.makedirs(os.path.dirname(file_path), exist_ok=True)

        with open(file_path, "w", encoding=encoding) as real_file:
            real_file.write(new_content)


def png_to_ico(png_path, ico_path):
    img = Image.open(png_path)

    # Windows ICO should include multiple sizes
    sizes = [(16,16), (32,32), (48,48), (64,64), (128,128), (256,256)]

    os.makedirs(Path(ico_path).parent, exist_ok=True)
    img.save(ico_path, format='ICO', sizes=sizes)
    with smart_open(str(Path(ico_path).parent) + "/Win64IconResource.rc") as resource_file:
        resource_file.write('IDI_APP_ICON ICON "IconWin64.ico"')

def get_module_name_from_path(module_path: str) -> str:
    module_path = module_path.replace("\\", "/")
    pattern = re.compile(r'.*/Modules/([^/]+)')
    match = pattern.match(module_path)
    if match:
        return match.group(1)
    else:
        raise ValueError(f"Could not extract module name from path: {module_path}")