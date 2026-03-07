import os
import io
from contextlib import contextmanager

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