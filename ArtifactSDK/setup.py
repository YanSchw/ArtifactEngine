from setuptools import setup, find_packages

from BuildTool.Version import get_version_string

def parse_requirements(filename):
    with open(filename, 'r') as f:
        lines = f.read().splitlines()
    # filter out comments and empty lines
    return [line.strip() for line in lines if line.strip() and not line.startswith("#")]

setup(
    name="artifact-buildtool",
    version=get_version_string(),
    packages=find_packages(),
    install_requires=parse_requirements('requirements.txt'),
    entry_points={
        "console_scripts": [
            "artifact = BuildTool.CLI:main",
        ],
    },
    python_requires='>=3.7',
)
