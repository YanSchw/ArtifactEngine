VERSION_MAJOR = 0
VERSION_MINOR = 1
VERSION_PATCH_GIT_COMMIT_COUNT = 18

import os
COMMIT_COUNT_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), '.GIT_COMMIT_COUNT')

def get_git_commit_count():
    try:
        import subprocess
        result = subprocess.run(['git', 'rev-list', '--count', 'HEAD'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, cwd=os.path.dirname(COMMIT_COUNT_FILE))
        if result.returncode == 0:
            count = int(result.stdout.strip())
            with open(COMMIT_COUNT_FILE, 'w') as f:
                f.write(str(count))
            return count
    except Exception:
        pass
    try:
        with open(COMMIT_COUNT_FILE) as f:
            return int(f.read().strip())
    except Exception as e:
        print(f"Warning: Could not get git commit count: {e}")
    return 0

def get_patch_version():
    commit_count = get_git_commit_count()
    return commit_count - VERSION_PATCH_GIT_COMMIT_COUNT if commit_count >= VERSION_PATCH_GIT_COMMIT_COUNT else None

def get_version_string():
    patch_version = get_patch_version()
    if patch_version is not None:
        return f"{VERSION_MAJOR}.{VERSION_MINOR}.{patch_version}"
    else:
        return f"{VERSION_MAJOR}.{VERSION_MINOR}"