import json
import os
import re
from datetime import datetime, timezone

CLASS_PATTERN = re.compile(r'(?:class|struct)\s+(\w+)\s*(?:final\s*)?:\s*public\s+(\w+)\s*\{', re.MULTILINE)
STRUCT_PATTERN = re.compile(r'struct\s+(\w+)\s*\{', re.MULTILINE)
ENUM_PATTERN = re.compile(r'enum\s+(class\s+)?(\w+)(?:\s*:\s*([\w:]+))?\s*\{', re.MULTILINE)

PROPERTY_PATTERN = re.compile(
    r'PROPERTY\s*\(\s*\)\s*'
    r'([\w:<> ,]+?)'
    r'\s+'
    r'(\w+)'
    r'(?:\s*=\s*([^;]+))?'
    r'\s*;',
    re.MULTILINE
)


def _leading_comment(lines: list[str], decl_line_index: int) -> str:
    """Collects the comment block directly above a declaration as its description."""
    doc_lines = []
    i = decl_line_index - 1
    # Skip the reflection macro line so the comment above it is still picked up.
    while i >= 0 and re.match(r'\s*ARTIFACT_(CLASS|STRUCT|ENUM)\(\);?\s*$', lines[i]):
        i -= 1

    if i >= 0 and lines[i].rstrip().endswith('*/'):
        block = []
        while i >= 0:
            block.append(lines[i])
            if lines[i].lstrip().startswith('/*'):
                break
            i -= 1
        for line in reversed(block):
            text = re.sub(r'^\s*/?\*+/?|\*/\s*$', '', line).strip()
            if text:
                doc_lines.append(text)
    else:
        while i >= 0 and re.match(r'\s*//', lines[i]):
            doc_lines.append(re.sub(r'^\s*///?', '', lines[i]).strip())
            i -= 1
        doc_lines.reverse()

    return ' '.join(doc_lines).strip()


def _find_body_end(code: str, start: int) -> int:
    end = code.find('};', start)
    return end if end != -1 else len(code)


def _parse_properties(body: str, body_lines: list[str]) -> list[dict]:
    properties = []
    for match in PROPERTY_PATTERN.finditer(body):
        line_index = body[:match.start()].count('\n')
        prop = {
            'type': match.group(1).strip(),
            'name': match.group(2).strip(),
        }
        default = match.group(3)
        if default:
            prop['default'] = default.strip()
        description = _leading_comment(body_lines, line_index)
        if description:
            prop['description'] = description
        properties.append(prop)
    return properties


def _parse_enum_values(body: str) -> list[dict]:
    values = []
    current = 0
    clean = re.sub(r'/\*.*?\*/', '', body, flags=re.DOTALL)
    for chunk in clean.split(','):
        comment_match = re.search(r'//\s*(.*)', chunk)
        entry = re.sub(r'//[^\n]*', '', chunk).strip()
        if not entry:
            continue
        m = re.match(r'(\w+)(?:\s*=\s*(.+))?$', entry, re.DOTALL)
        if not m:
            continue
        expr = m.group(2)
        if expr:
            try:
                current = eval(expr.strip(), {"__builtins__": None}, {})
            except Exception:
                current = expr.strip()
        value = {'name': m.group(1), 'value': current}
        if comment_match and comment_match.group(1).strip():
            value['description'] = comment_match.group(1).strip()
        values.append(value)
        if isinstance(current, int):
            current += 1
    return values


def _collect_types_from_header(header_path: str, module: str, repo_root: str) -> list[dict]:
    with open(header_path, 'r', encoding='utf-8') as h_file:
        code = h_file.read()
    lines = code.splitlines()
    rel_header = os.path.relpath(header_path, repo_root).replace('\\', '/')
    types = []

    for match in CLASS_PATTERN.finditer(code):
        body = code[match.end():_find_body_end(code, match.end())]
        if not re.search(r'\bARTIFACT_CLASS\(\);', body):
            continue
        decl_line = code[:match.start()].count('\n')
        body_lines = body.splitlines()
        entry = {
            'kind': 'class',
            'name': match.group(1),
            'parent': match.group(2),
            'module': module,
            'header': rel_header,
        }
        description = _leading_comment(lines, decl_line)
        if description:
            entry['description'] = description
        properties = _parse_properties(body, body_lines)
        if properties:
            entry['properties'] = properties
        types.append(entry)

    class_names = {t['name'] for t in types}
    for match in STRUCT_PATTERN.finditer(code):
        if match.group(1) in class_names:
            continue
        body = code[match.end():_find_body_end(code, match.end())]
        if not re.search(r'\bARTIFACT_STRUCT\(\);', body):
            continue
        decl_line = code[:match.start()].count('\n')
        entry = {
            'kind': 'struct',
            'name': match.group(1),
            'module': module,
            'header': rel_header,
        }
        description = _leading_comment(lines, decl_line)
        if description:
            entry['description'] = description
        properties = _parse_properties(body, body.splitlines())
        if properties:
            entry['properties'] = properties
        types.append(entry)

    for match in ENUM_PATTERN.finditer(code):
        decl_line = code[:match.start()].count('\n')
        if decl_line == 0 or 'ARTIFACT_ENUM' not in lines[decl_line - 1]:
            continue
        body = code[match.end():_find_body_end(code, match.end())]
        entry = {
            'kind': 'enum',
            'name': match.group(2),
            'module': module,
            'header': rel_header,
            'isEnumClass': match.group(1) is not None,
            'underlyingType': match.group(3) or 'int',
            'values': _parse_enum_values(body),
        }
        description = _leading_comment(lines, decl_line - 1)
        if description:
            entry['description'] = description
        types.append(entry)

    return types


OBJECT_HEADER = 'Modules/EngineCore/Object/Object.h'

OBJECT_DESCRIPTION = (
    'The reflected base class every ARTIFACT_CLASS() type ultimately derives from. '
    'Objects are constructed by class identity, cast type-safely with Cast<T>(), and '
    'owned through SharedObjectPtr<T>.'
)


def _inject_object_type(modules: list[dict], types: list[dict]) -> None:
    if any(t['name'] == 'Object' for t in types):
        return
    types.append({
        'kind': 'class',
        'name': 'Object',
        'module': 'EngineCore',
        'header': OBJECT_HEADER,
        'description': OBJECT_DESCRIPTION,
    })
    for module in modules:
        if module['name'] == 'EngineCore' and 'Object' not in module['types']:
            module['types'] = sorted(module['types'] + ['Object'])
            break


def _collect_module(modules_root: str, module: str, repo_root: str) -> tuple[dict, list[dict]]:
    module_dir = os.path.join(modules_root, module)
    module_json_path = os.path.join(module_dir, 'Module.json')
    module_json = {}
    if os.path.isfile(module_json_path):
        with open(module_json_path, 'r', encoding='utf-8') as f:
            module_json = json.load(f)

    types = []
    for dirpath, dirnames, filenames in os.walk(module_dir):
        dirnames[:] = [d for d in dirnames if d != 'ThirdParty']
        for filename in sorted(filenames):
            if filename.endswith(('.h', '.hpp')):
                types.extend(_collect_types_from_header(os.path.join(dirpath, filename), module, repo_root))

    module_entry = {
        'name': module,
        'importModules': module_json.get('ImportModules', []),
        'types': sorted(t['name'] for t in types),
    }
    if 'MountContentDir' in module_json:
        module_entry['mountContentDir'] = module_json['MountContentDir']
    if 'SupportedPlatforms' in module_json:
        module_entry['supportedPlatforms'] = module_json['SupportedPlatforms']
    return module_entry, types


def generate_docs_json(engine_path: str, project_path: str, output_dir: str) -> str:
    """Dumps reflection docs for every module of the engine and current project
    into a single api.json consumed by the /Docs Angular frontend."""
    from SDK.Version import get_version_string

    roots = [(engine_path, os.path.join(engine_path, 'Modules'))]
    if os.path.normpath(project_path) != os.path.normpath(engine_path):
        roots.append((project_path, os.path.join(project_path, 'Modules')))

    modules = []
    types = []
    seen_modules = set()
    for repo_root, modules_root in roots:
        if not os.path.isdir(modules_root):
            continue
        for module in sorted(os.listdir(modules_root)):
            if module in seen_modules or not os.path.isdir(os.path.join(modules_root, module)):
                continue
            seen_modules.add(module)
            module_entry, module_types = _collect_module(modules_root, module, repo_root)
            modules.append(module_entry)
            types.extend(module_types)

    _inject_object_type(modules, types)
    types.sort(key=lambda t: t['name'].lower())
    api = {
        'engineVersion': get_version_string(),
        'generatedAt': datetime.now(timezone.utc).isoformat(timespec='seconds'),
        'modules': modules,
        'types': types,
    }

    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, 'api.json')
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(api, f, indent=2)
        f.write('\n')
    return output_path
