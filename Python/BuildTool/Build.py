import os
import shutil
import subprocess

def build_cmake(clean: bool = False):
    if clean:
        if os.path.exists("Build"):
            shutil.rmtree("Build")

    if os.path.exists("Binaries"):
        shutil.rmtree("Binaries")

    subprocess.run(["cmake", "-S", ".", "-G", "Xcode", "-B", "Build"], check=True)
    subprocess.run(["cmake", "--build", "Build", "--config", "Debug"], check=True)