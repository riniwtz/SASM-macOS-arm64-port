<a id="readme-top"></a>

<!--
SASM macOS ARM64 Port

SASM itself is Copyright (C) 2013 Dmitriy Manushin and is distributed
under the GNU General Public License v3.0 or later.
-->

<br />

<div align="center">
  <a href="https://github.com/riniwtz/SASM-macOS-arm64-port">
    <img src="assets/sasm.png" alt="SASM Logo" width="96" height="96">
  </a>

  <h3 align="center">SASM macOS ARM64 Port</h3>

  <p align="center">
    An unofficial, reproducible macOS Apple Silicon port of SASM 3.16.0.
    <br />
    Built with Qt 5, qmake, macdeployqt, Mach-O tooling, and macOS code signing.
    <br />
    <br />
    <a href="https://github.com/Dman95/SASM"><strong>View upstream SASM »</strong></a>
    <br />
    <br />
    <a href="https://github.com/riniwtz/SASM-macOS-arm64-port/releases">Releases</a>
    &middot;
    <a href="https://github.com/riniwtz/SASM-macOS-arm64-port/issues">Report Bug</a>
    &middot;
    <a href="https://github.com/riniwtz/SASM-macOS-arm64-port/issues">Request Feature</a>
  </p>
</div>

---

## Table of Contents

<details>
  <summary>Expand</summary>

  <ol>
    <li><a href="#about-the-project">About The Project</a></li>
    <li><a href="#project-structure">Project Structure</a></li>
    <li><a href="#built-with">Built With</a></li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#clone">Clone</a></li>
      </ul>
    </li>
    <li><a href="#build">Build</a></li>
    <li><a href="#deploy">Deploy</a></li>
    <li><a href="#package">Package</a></li>
    <li><a href="#macos-specific-changes">macOS-Specific Changes</a></li>
    <li><a href="#running-nasm-code-on-macos">Running NASM Code on macOS</a></li>
    <li><a href="#optional-gdb-debugging">Optional: GDB Debugging</a></li>
    <li><a href="#releases">Releases</a></li>
    <li><a href="#status">Status</a></li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#upstream">Upstream</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#acknowledgments">Acknowledgments</a></li>
  </ol>

</details>

---

## About The Project

SASM is a cross-platform IDE for assembly development supporting assemblers such as NASM, GAS, FASM, and MASM.

Upstream SASM provides official Windows and Linux support, but does not currently ship an official modern macOS application bundle.

This repository provides an **unofficial macOS ARM64 port and reproducible build workflow for SASM 3.16.0**.

The goal is not to turn SASM into a separate product, but to make the upstream source practical to build, deploy, archive, and distribute on Apple Silicon Macs.

### What this port adds

- Native Apple Silicon (`arm64`) SASM build
- Qt 5 / qmake macOS build workflow
- Proper macOS `.app` bundle generation
- Proper macOS `.icns` application icon
- Fix for the upstream Windows `RC_FILE = icon.rc` configuration affecting macOS bundle metadata
- Qt framework deployment with `macdeployqt`
- Ad-hoc macOS code signing after deployment
- Reproducible build, deploy, and package scripts
- ZIP release packaging suitable for GitHub Releases
- Preserved upstream SASM source and licensing information

### Tested environment

This port has been tested with:

- **SASM:** 3.16.0
- **Architecture:** Apple Silicon ARM64
- **macOS:** 26.5
- **Qt:** Qt 5
- **Build system:** qmake + make
- **Assembler:** NASM
- **Linker:** Apple Clang
- **Deployment:** macdeployqt
- **Signing:** ad-hoc `codesign`

> [!NOTE]
> This project is an unofficial community port. It is not an official macOS release from the upstream SASM maintainer.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

---

## Project Structure

```text
SASM-macOS-arm64-port/
├── .gitignore
├── LICENSE
├── README.md
├── versions.env
│
├── assets/
│   ├── sasm.icns
│   └── sasm.png
│
├── scripts/
│   ├── build.sh
│   ├── deploy.sh
│   └── package.sh
│
├── source/
│   └── src/
│       └── sasm/
│           └── BUILD/
│               ├── SASM.pro
│               ├── ...
│               └── build-macos/       # generated, gitignored
│                   └── sasm.app
│
├── upstream/
│   └── sasm_3.16.0-1.tar.gz
│
└── dist/                               # generated, gitignored
    └── SASM-3.16.0-macOS-arm64.zip
```

The `build-macos/` and `dist/` directories contain generated output and should not be committed to Git.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

---

## Built With

- **SASM 3.16.0**
- **Qt 5**
- **qmake**
- **GNU Make**
- **Apple Clang**
- **NASM**
- **macdeployqt**
- **macOS codesign**
- **Shell scripts**

<p align="right">(<a href="#readme-top">back to top</a>)</p>

---

## Getting Started

### Prerequisites

This port is intended for Apple Silicon Macs.

Confirm your architecture:

```sh
uname -m
```

Expected output:

```text
arm64
```

Install Apple's Command Line Tools:

```sh
xcode-select --install
```

Install Qt 5 and NASM with Homebrew:

```sh
brew install qt@5 nasm
```

Verify the Qt 5 qmake binary:

```sh
"$(brew --prefix qt@5)/bin/qmake" -v
```

Verify NASM:

```sh
nasm -v
```

### Clone

Clone the repository:

```sh
git clone https://github.com/riniwtz/SASM-macOS-arm64-port.git
cd SASM-macOS-arm64-port
```

Make the scripts executable if necessary:

```sh
chmod +x scripts/build.sh scripts/deploy.sh scripts/package.sh
```

<p align="right">(<a href="#readme-top">back to top</a>)</p>

---

## Build

Build SASM:

```sh
./scripts/build.sh
```

The generated application should be located at:

```text
source/src/sasm/BUILD/build-macos/sasm.app
```

The build script:

1. Copies the canonical macOS icon from `assets/sasm.icns`
2. Creates a clean macOS build directory
3. Runs Qt 5 `qmake`
4. Compiles SASM with `make`

The build output is intentionally kept separate from the preserved upstream source files.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

---

## Deploy

Deploy the Qt frameworks and sign the final application bundle:

```sh
./scripts/deploy.sh
```

The deployment process is:

```text
sasm.app
   ↓
macdeployqt
   ↓
Qt frameworks embedded into the application bundle
   ↓
codesign
   ↓
signature verification
```

### Why signing happens after macdeployqt

`macdeployqt` copies and modifies Mach-O binaries and framework paths inside the application bundle.

Signing the application before deployment can invalidate the signature after those files are modified.

This may cause macOS to terminate SASM with an error such as:

```text
Namespace CODESIGNING
Code 2, Invalid Page
```

The final application bundle must therefore be signed **after** `macdeployqt` and after any other modification to executable code or bundled frameworks.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

---

## Package

Create the distributable ZIP archive:

```sh
./scripts/package.sh
```

Output:

```text
dist/SASM-3.16.0-macOS-arm64.zip
```

The packaging script uses macOS `ditto` to preserve application bundle metadata:

```sh
ditto \
    -c \
    -k \
    --sequesterRsrc \
    --keepParent \
    sasm.app \
    SASM-3.16.0-macOS-arm64.zip
```

The ZIP archive is intended to be attached to a GitHub Release rather than committed directly into the repository.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

---

## macOS-Specific Changes

### Application icon

Upstream SASM includes application artwork, but its qmake configuration contains a Windows resource entry:

```qmake
RC_FILE = icon.rc
```

When used during the macOS build, this can produce incorrect application bundle metadata such as:

```text
CFBundleIconFile = icon.rc
```

`icon.rc` is a Windows resource script and cannot be used as a native macOS application icon.

This port scopes the Windows resource configuration to Windows and provides a native macOS `.icns` file:

```qmake
win32 {
    RC_FILE = icon.rc
}

macx {
    ICON = sasm.icns
}
```

The resulting macOS bundle contains:

```text
sasm.app/
└── Contents/
    ├── Info.plist
    ├── MacOS/
    │   └── sasm
    └── Resources/
        └── sasm.icns
```

The resulting `Info.plist` references:

```text
CFBundleIconFile = sasm.icns
```

### Qt deployment

Qt libraries are embedded using:

```sh
"$(brew --prefix qt@5)/bin/macdeployqt" sasm.app
```

This allows the application to use the required Qt frameworks from inside its own `.app` bundle instead of relying on a Qt installation at runtime.

### Code signing

The final application is currently ad-hoc signed:

```sh
codesign \
    --force \
    --deep \
    --sign - \
    --timestamp=none \
    sasm.app
```

The resulting bundle is verified with:

```sh
codesign \
    --verify \
    --deep \
    --strict \
    --verbose=4 \
    sasm.app
```

> [!IMPORTANT]
> Ad-hoc signing is not the same as Apple Developer ID signing or Apple notarization.
>
> Release artifacts downloaded from GitHub may still trigger normal macOS Gatekeeper warnings on another Mac.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

---

## Running NASM Code on macOS

Linux-oriented NASM workflows commonly generate ELF object files.

macOS uses the Mach-O object format instead.

For x86-64 NASM source intended to link through Apple Clang, assemble using:

```sh
nasm -f macho64 program.asm -o program.o
```

Then link:

```sh
clang -arch x86_64 program.o -o program
```

The assembly source must expose an entry point compatible with the selected linking method.

For example, when linking through the normal C runtime with Clang, a minimal program can expose `_main`:

```asm
global _main

section .text

_main:
    mov eax, 42
    ret
```

Build it with:

```sh
nasm -f macho64 program.asm -o program.o
clang -arch x86_64 program.o -o program
```

Then run:

```sh
./program
echo $?
```

Expected result:

```text
42
```

SASM's assembler and linker settings may need to be configured accordingly when writing x86-64 assembly on Apple Silicon.

> [!NOTE]
> The SASM application itself is built as native ARM64.
>
> Programs assembled as x86-64 are a separate architecture concern and may require Rosetta to execute on Apple Silicon.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

---

## Optional: GDB Debugging

SASM's graphical debugging functionality expects GDB.

GDB is **not required** to build, launch, or use SASM as an editor and assembler frontend.

Install GDB with Homebrew:

```sh
brew install gdb
```

Modern macOS requires additional code-signing configuration before GDB can debug other processes.

GDB signing is intentionally treated as a machine-specific setup step.

Private signing certificates and private keys should **not** be stored or distributed with this repository.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

---

## Releases

Release artifacts are generated from the reproducible scripts:

```sh
./scripts/build.sh
./scripts/deploy.sh
./scripts/package.sh
```

This produces:

```text
dist/SASM-3.16.0-macOS-arm64.zip
```

Create a release tag:

```sh
git tag -a v3.16.0-macos.1 \
    -m "SASM 3.16.0 macOS ARM64 release"

git push origin v3.16.0-macos.1
```

Create the GitHub Release using GitHub CLI:

```sh
gh release create v3.16.0-macos.1 \
    dist/SASM-3.16.0-macOS-arm64.zip \
    --title "SASM 3.16.0 macOS ARM64 Port" \
    --notes "Unofficial macOS ARM64 port of SASM 3.16.0."
```

<p align="right">(<a href="#readme-top">back to top</a>)</p>

---

## Status

SASM 3.16.0 successfully builds, deploys, signs, and packages on Apple Silicon macOS.

The current port includes:

- [x] Native Apple Silicon build
- [x] Valid macOS `.app` bundle
- [x] Qt 5 deployment with `macdeployqt`
- [x] Native `.icns` application icon
- [x] Correct macOS bundle icon metadata
- [x] Reproducible build script
- [x] Deployment and code-signing script
- [x] ZIP release packaging

<p align="right">(<a href="#readme-top">back to top</a>)</p>

---

## Contributing

Contributions, bug fixes, documentation improvements, and macOS-specific build improvements are welcome.

1. Fork the repository.

2. Create a feature branch:

   ```sh
   git checkout -b feature/my-change
   ```

3. Commit your changes:

   ```sh
   git commit -m "Describe the change"
   ```

4. Push the branch:

   ```sh
   git push origin feature/my-change
   ```

5. Open a Pull Request.

Please keep changes focused on the macOS port unless the change would be more appropriate for upstream SASM.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

---

## Upstream

This repository is based on **SASM**, originally developed by **Dmitriy Manushin (Dman95)**.

Upstream repository:

https://github.com/Dman95/SASM

This project does **not** claim ownership of SASM itself and is not presented as an official upstream macOS release.

Where possible, changes that are generally useful to SASM itself should be contributed upstream rather than unnecessarily diverging in this port.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

---

## License

SASM is free software distributed under the **GNU General Public License, version 3 or later**.

This repository contains an unofficial macOS ARM64 port of SASM and includes modifications, build scripts, packaging configuration, and related macOS-specific adaptations.

Original SASM:

```text
Copyright (C) 2013 Dmitriy Manushin
```

macOS port and related modifications:

```text
Copyright (C) 2026 Rintaro Iwata
```

SASM-derived source code and modifications remain available under the terms of the **GNU General Public License v3.0 or later**.

See:

- [`LICENSE`](LICENSE) for project-specific attribution and licensing information
- [`COPYING`](COPYING) for the full GNU General Public License Version 3 text
- the preserved upstream SASM copyright and license notices in the source tree

<p align="right">(<a href="#readme-top">back to top</a>)</p>

---

## Acknowledgments

- **Dmitriy Manushin (Dman95)** — creator and upstream maintainer of SASM
- **SASM contributors** — for the original cross-platform assembly IDE
- **Brian R. Hall** — for the historical *Installing SASM on a Mac* guide documenting an earlier macOS build approach

<p align="right">(<a href="#readme-top">back to top</a>)</p>
