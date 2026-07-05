# ArtifactSDK

## Install

Requirements:
- Python 3.7+

### Windows
```powershell
py -m venv venv
venv\Scripts\Activate.ps1
pip install -e .\ArtifactSDK\.
```

### macOS / Linux
```sh
python3 -m venv venv
source venv/bin/activate
pip install -e ./ArtifactSDK/.
```

## Run SDK
```
artifact --help
```

```
❯ artifact --help
usage: artifact [-h] {generate,build,run,cook,package,lint,version,location} ...

Artifact Engine Build Tool

positional arguments:
  {generate,build,run,cook,package,lint,version,location}
    generate            Generate project files (CMake + native IDE project)
                        without building
    build               Build the engine
    run                 Run the engine
    cook                Cook assets
    package             Package project
    lint                Lint C++/Header files
    version             Show engine version
    location            Print the engine path to the terminal

options:
  -h, --help            show this help message and exit
```

`artifact generate` writes the CMake files, reflection code, and a native IDE
project (a Visual Studio solution on Windows, an Xcode project on macOS) at
the project root. Building from the IDE just activates this venv and shells 
out to `artifact build`.