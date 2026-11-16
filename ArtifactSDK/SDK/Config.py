import os
import json

FROZEN_CONFIG_VARS = "FrozenConfigVars"
ALWAYS_FROZEN_CONFIG_VARS = ["ProjectName"]
DEFAULT_PROJECT_NAME = "Artifact"


def get_config_dir(project_path: str) -> str:
    return f"{project_path.rstrip('/')}/Config"


def list_config_files(project_path: str) -> list[str]:
    config_dir = get_config_dir(project_path)
    if not os.path.isdir(config_dir):
        return []
    return sorted(f"{config_dir}/{name}" for name in os.listdir(config_dir)
                  if name.endswith(".ini") and os.path.isfile(f"{config_dir}/{name}"))


def _parse_entry(line: str) -> tuple[str, str] | None:
    stripped = line.strip()
    if not stripped or stripped[0] in ";#" or (stripped.startswith("[") and stripped.endswith("]")):
        return None
    key, equals, value = stripped.partition("=")
    if not equals:
        return None
    return key.strip(), value.strip()


def read_config_vars(project_path: str) -> dict[str, str]:
    values = {}
    for path in list_config_files(project_path):
        with open(path, encoding="utf-8") as f:
            for line in f.read().splitlines():
                entry = _parse_entry(line)
                if entry:
                    values[entry[0]] = entry[1]
    return values


def get_project_name(project_path: str) -> str:
    name = read_config_vars(project_path).get("ProjectName", DEFAULT_PROJECT_NAME)
    if not name or name in (".", "..") or any(char in name for char in '/\\:*?"<>|'):
        print(f"ProjectName '{name}' is not a valid file name; using '{DEFAULT_PROJECT_NAME}'")
        return DEFAULT_PROJECT_NAME
    return name


def get_frozen_config_vars(project_path: str) -> list[tuple[str, str | None]]:
    values = read_config_vars(project_path)
    raw = values.get(FROZEN_CONFIG_VARS, "[]")
    try:
        names = json.loads(raw)
    except json.JSONDecodeError:
        names = None
    if not isinstance(names, list) or not all(isinstance(name, str) for name in names):
        raise ValueError(f"{FROZEN_CONFIG_VARS} must be a JSON array of config var names, got: {raw}")
    names = ALWAYS_FROZEN_CONFIG_VARS + names
    return [(name, values.get(name)) for name in dict.fromkeys(names) if name and name != FROZEN_CONFIG_VARS]


def _is_header(line: str) -> bool:
    stripped = line.strip()
    return stripped.startswith("[") and stripped.endswith("]")


def _remove_keys(lines: list[str], keys: set[str]) -> list[str]:
    sections = [[]]
    for line in lines:
        entry = _parse_entry(line)
        if entry and entry[0] in keys:
            continue
        if _is_header(line):
            sections.append([])
        sections[-1].append(line)
    kept = []
    for index, section in enumerate(sections):
        if index == 0 or any(line.strip() for line in section[1:]):
            kept.extend(section)
    while kept and not kept[-1].strip():
        kept.pop()
    return kept


def stage_packaged_config(project_path: str, content_dir: str):
    frozen = {name for name, _ in get_frozen_config_vars(project_path)} | {FROZEN_CONFIG_VARS}
    dest_dir = f"{content_dir}/Config"
    for path in list_config_files(project_path):
        with open(path, encoding="utf-8") as f:
            kept = _remove_keys(f.read().splitlines(), frozen)
        if not kept:
            continue
        os.makedirs(dest_dir, exist_ok=True)
        with open(f"{dest_dir}/{os.path.basename(path)}", "w", encoding="utf-8") as f:
            f.write("\n".join(kept) + "\n")


def _cpp_string_literal(text: str) -> str:
    out = []
    for byte in text.encode("utf-8"):
        char = chr(byte)
        if char == "\\":
            out.append("\\\\")
        elif char == '"':
            out.append('\\"')
        elif 0x20 <= byte < 0x7f:
            out.append(char)
        else:
            out.append(f"\\{byte:03o}")
    return '"' + "".join(out) + '"'


def generate_frozen_config_vars(project_path: str, is_packaged: bool) -> str:
    entries = get_frozen_config_vars(project_path) if is_packaged else []
    code = "#if defined(AE_PACKAGED)\n\n"
    code += "void __FrozenConfigVars(std::vector<std::tuple<std::string, std::string, bool>>& vars) {\n"
    for name, value in entries:
        has_value = value is not None
        code += f"    vars.push_back({{{_cpp_string_literal(name)}, {_cpp_string_literal(value or '')}, {'true' if has_value else 'false'}}});\n"
    code += "}\n#endif\n"
    return code
