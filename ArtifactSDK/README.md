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
usage: artifact [-h] {build,run,cook,package,lint,version} ...

Artifact Engine Build Tool

positional arguments:
  {build,run,cook,package,lint,version}
    build               Build the engine
    run                 Run the engine
    cook                Cook assets
    package             Package project
    lint                Lint C++/Header files
    version             Show engine version

options:
  -h, --help            show this help message and exit
```