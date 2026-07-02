# DV Volume-Flow VTK Project Setup Guide

This README explains how to install the required dependencies, build the project, and run the VTK programs.

---

## 1. Required Software

Before running this project, install:

1. Visual Studio 2022 Community or Visual Studio 2022 Build Tools
2. Desktop development with C++ workload
3. Git
4. CMake
5. vcpkg
6. VTK with OpenGL support

---

## 2. Install Visual Studio C++ Compiler

Download Visual Studio 2022 Community from:

https://visualstudio.microsoft.com/

During installation, make sure to select:

```text
Desktop development with C++
```

This is required because the project uses the MSVC C++ compiler.

---

## 3. Install Git

Download and install Git from:

https://git-scm.com/downloads

After installation, check:

```powershell
git --version
```

---

## 4. Install CMake

Install CMake using PowerShell:

```powershell
winget install --id Kitware.CMake -e
```

After installation, close and reopen PowerShell or VS Code, then check:

```powershell
cmake --version
```

If `cmake` is not recognized, run:

```powershell
$env:Path += ";C:\Program Files\CMake\bin"
cmake --version
```

---

## 5. Install vcpkg

It is recommended to install vcpkg in the D drive because VTK is large.

```powershell
cd D:\

mkdir dev
cd D:\dev

git clone https://github.com/microsoft/vcpkg
cd vcpkg

.\bootstrap-vcpkg.bat
```

---

## 6. Install VTK with OpenGL Support

Run:

```powershell
cd D:\dev\vcpkg

.\vcpkg install "vtk[opengl]:x64-windows" --recurse --clean-after-build
```

This process may take a long time.

After installation, run:

```powershell
.\vcpkg integrate install
```

Check whether VTK is installed:

```powershell
.\vcpkg list vtk
```

Expected result should include:

```text
vtk:x64-windows
vtk[opengl]:x64-windows
vtk-compile-tools:x64-windows
```

Check whether the VTK header exists:

```powershell
Test-Path "D:\dev\vcpkg\installed\x64-windows\include\vtk-9.3\vtkRenderer.h"
```

It should return:

```text
True
```

---

## 7. Open the Project Folder

Go to the project root folder:

```powershell
cd "D:\UTM\Year 3\Sem 2\DATA VISUALISATION\Project 2\DV_Volume-FLow"
```

If the project is stored in a different location, change the path accordingly.

---

## 8. Dataset Setup

Make sure the flow datasets are inside:

```text
Flow Dataset/testData1.vtk
Flow Dataset/testData2.vtk
Flow Dataset/carotid.vtk
```

For the head volume dataset, extract:

```text
Volumetric Dataset/vtkHeadDataset.zip
```

Run this command from the project root folder:

```powershell
Expand-Archive -LiteralPath ".\Volumetric Dataset\vtkHeadDataset.zip" -DestinationPath "." -Force
```

After extraction, check:

```powershell
Test-Path ".\data\headsq\quarter.1"
```

It should return:

```text
True
```

---

## 9. Configure the Project with CMake

From the project root folder, run:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=D:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake
```

If vcpkg is installed in another location, update the toolchain file path.

Example:

```powershell
-DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake
```

---

## 10. Build the Project

Run:

```powershell
cmake --build build --config Release
```

If successful, the executable files will be created in:

```text
build/Release
```

Expected output files:

```text
FlowVis.exe
Isosurface.exe
RayMarching.exe
```

---

## 11. Run the Programs

Run these commands from the project root folder.

### Flow Visualisation

```powershell
.\build\Release\FlowVis.exe
```

### Iso-surface Visualisation

```powershell
.\build\Release\Isosurface.exe
```

### Ray Marching / Volume Rendering

```powershell
.\build\Release\RayMarching.exe
```

---

## 12. Common Problems and Fixes

### Problem: `cmake is not recognized`

Fix:

```powershell
$env:Path += ";C:\Program Files\CMake\bin"
cmake --version
```

Or close and reopen PowerShell / VS Code after installing CMake.

---

### Problem: `vtkRenderer.h not found`

This means VTK is not installed or CMake is not using the vcpkg toolchain file.

Fix by configuring CMake with the vcpkg toolchain:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=D:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake
```

---

### Problem: `RenderingVolumeOpenGL2 not found`

This means VTK was installed without OpenGL support.

Fix:

```powershell
cd D:\dev\vcpkg

.\vcpkg install "vtk[opengl]:x64-windows" --recurse --clean-after-build
```

---

### Problem: `Unable to open file: ../data/testData1.vtk`

This means the dataset path is wrong or the dataset is missing.

Make sure the file exists here:

```text
Flow Dataset/testData1.vtk
```

---

### Problem: `Can't find file: ../data/headsq/quarter.1`

This means the head dataset was not extracted or the path is wrong.

Run:

```powershell
Expand-Archive -LiteralPath ".\Volumetric Dataset\vtkHeadDataset.zip" -DestinationPath "." -Force
```

Then check:

```powershell
Test-Path ".\data\headsq\quarter.1"
```

---

## 13. Clean Rebuild

If the build has problems, delete the build folder and rebuild:

```powershell
if (Test-Path .\build) {
    Remove-Item -Recurse -Force .\build
}

cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=D:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake

cmake --build build --config Release
```

---

## 14. Notes for Teammates

- Do not use the VS Code normal `Run Code` button.
- Always build using CMake.
- Make sure VTK is installed with OpenGL support.
- Make sure datasets are placed in the correct folders.
- If vcpkg is installed in a different drive, update the CMake toolchain path.
