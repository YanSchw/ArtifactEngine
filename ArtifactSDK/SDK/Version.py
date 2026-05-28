VERSION_MAJOR = 0
VERSION_MINOR = 1
VERSION_PATCH_GIT_COMMIT_COUNT = 18

def get_git_commit_count():
    try:
        import subprocess
        result = subprocess.run(['git', 'rev-list', '--count', 'HEAD'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        if result.returncode == 0:
            return int(result.stdout.strip())
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