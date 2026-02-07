from setuptools import setup, find_packages

def parse_requirements(filename):
    with open(filename, 'r') as f:
        lines = f.read().splitlines()
    # filter out comments and empty lines
    return [line.strip() for line in lines if line.strip() and not line.startswith("#")]

setup(
    name="artifact-buildtool",
    version="0.1.0",
    packages=find_packages(),
    install_requires=parse_requirements('requirements.txt'),
    entry_points={
        "console_scripts": [
            "artifact = BuildTool.CLI:main",
        ],
    },
    python_requires='>=3.7',
)
