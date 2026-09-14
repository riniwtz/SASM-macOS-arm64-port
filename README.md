SASM macOS Build

SASM: 3.16.0
Architecture tested: Apple Silicon ARM64
Qt: Qt 5
Build system: qmake + make
Deployment: macdeployqt
Assembler: NASM
Linker: Apple Clang

Build:

./scripts/setup.sh
./scripts/build.sh
./scripts/deploy.sh
./scripts/package.sh

Documentation:
Important macOS notes:

1. Upstream SASM.pro contains:

   RC_FILE = icon.rc

   This causes qmake to use icon.rc as CFBundleIconFile on macOS.

2. The fix is:

   win32 {
       RC_FILE = icon.rc
   }

   macx {
       ICON = sasm.icns
   }

3. SASM does not currently include a macOS .icns asset.
   sasm.icns is generated from the upstream SASM icon.

4. Run macdeployqt before final code signing.

5. Do not modify binaries/frameworks after signing.

6. NASM output must use Mach-O rather than Linux ELF.
