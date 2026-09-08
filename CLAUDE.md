# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A desktop photo editor for quickly cropping and resizing multiple images in a folder — a C++/Qt 6 port of [FitToList-Python](https://github.com/rickapps/FitToList-Python) (sibling checkout at `E:\Projects\FitToList-Python`). Point it at a source folder and a processed folder, then work through source images one at a time: crop, straighten, rotate/flip, save. The port is feature-complete (see README.md for the full feature list); only packaging/installers remain undone.

**When changing behavior that exists in both projects** (straighten's angle math, save/overwrite rules, the source/processed file-tree matching, Reduce All Images), read `E:\Projects\FitToList-Python\CLAUDE.md` first — it documents the original's behavior in more procedural detail (exact state transitions, edge cases, and the reasoning behind non-obvious choices like why straighten re-rotates a pristine snapshot instead of the already-rotated image) than is restated here. The C++ classes below implement the same rules; where wording differs, the Python doc's edge-case reasoning still applies unless a deliberate porting decision is noted in code comments or README.md.

## Build & test

Requires Qt 6 (Widgets + Test components) and CMake 3.16+.

```bash
cmake -B build -S .
cmake --build build
./build/FitToList
ctest --test-dir build --output-on-failure
```

On Linux, package-manager Qt installs (`qt6-base-dev`/`qt6-qtbase-devel`) are found automatically — no extra flags needed. On Windows, there's no system package manager for Qt, so use the `windows` CMake preset (`CMakePresets.json`) instead, which sets `CMAKE_PREFIX_PATH` to a Qt kit and forces the Ninja generator:

```bash
cmake --preset windows
cmake --build --preset windows
ctest --preset windows
```

The preset assumes Qt 6.11.2's MinGW kit at `E:/Qt/6.11.2/mingw_64`, plus `cmake`/`ninja`/`g++`/`mingw32-make` from Qt's bundled `Tools/CMake_64`, `Tools/Ninja`, and `Tools/mingw1310_64` directories (added to this machine's user `PATH`). If Qt lives elsewhere, edit the path in `CMakePresets.json` or add a git-ignored `CMakeUserPresets.json` that inherits from it instead — don't hardcode a machine-specific path back into `CMakeLists.txt` itself, since that's what previously broke the Linux build path (`CMAKE_PREFIX_PATH`/`Qt6_DIR` were unconditionally set to a Windows-only path there before this was fixed).

### Building with Visual Studio (MSIX packaging work)

The `windows-msvc` preset builds with real MSVC instead of MinGW, generating a `.sln`/`.vcxproj` tree under `build-msvc/` that Visual Studio can open directly — needed because a Windows Application Packaging (MSIX) project has to live in a proper VS solution alongside the app's `.vcxproj`, which the MinGW+Ninja preset can't produce. It requires Qt's **MSVC 2022 64-bit** kit (`E:/Qt/6.11.2/msvc2022_64`), installed separately from the MinGW kit via the Qt Maintenance Tool — Qt binaries aren't ABI-compatible across MinGW and MSVC, so the MinGW kit can't be linked from an MSVC build.

This machine's Visual Studio install is version 18 (2026), which the system-wide `cmake` (3.30.5, from Qt's bundled `Tools/CMake_64`) doesn't recognize — configuring `windows-msvc` needs the newer CMake bundled inside Visual Studio itself:

```bash
VSCMAKE="/c/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin"
"$VSCMAKE/cmake.exe" --preset windows-msvc
"$VSCMAKE/cmake.exe" --build build-msvc --config Debug
```

Test binaries built this way link Qt's MSVC DLLs, which aren't on `PATH` by default (unlike the MinGW kit's `bin` dir, which already is) — add `E:/Qt/6.11.2/msvc2022_64/bin` to `PATH` before running `ctest`/the exes directly, or the tests fail immediately with exit code `0xc0000135` (`STATUS_DLL_NOT_FOUND`).

If a future Visual Studio upgrade changes the installed version, update the `generator` field in the `windows-msvc` preset (`CMakePresets.json`) to match — CMake's Visual Studio generator name is tied to the specific VS version and errors with "could not find any instance of Visual Studio" if it doesn't match what's installed.

To run a single test binary directly (all tests link `Qt6::Test` / `QTest`, so standard QTest CLI flags work, e.g. `TestName::testFunction` to run one test case):

```bash
ctest --test-dir build -R ImageDocumentTest --output-on-failure
./build/tests/ImageDocumentTest
```

Widget/dialog/main-window tests (`CanvasWidgetTest`, `ImageTreeWidgetTest`, `MaxSizeDialogTest`, `FolderSelectionDialogTest`, `MainWindowTest`, `ToolbarIconsTest`) run headless via `QT_QPA_PLATFORM=offscreen`, set automatically through `set_tests_properties(... ENVIRONMENT ...)` in `tests/CMakeLists.txt`; set that env var yourself if running the binaries directly outside ctest. They drive real widgets with `QTest`'s synthetic mouse/keyboard events and poll for modal dialogs rather than mocking Qt.

## Architecture

Unlike the Python original's single script, this is organized as one class (mostly) per header/source pair under `src/`, built into two static libraries linked into the `FitToList` executable, with a matching test binary per class under `tests/`:

- **`FitToListCore`** — UI-free logic: `ImageDocument`, `FileNaming`, `Config`. Directly unit-testable with no Qt widgets/display involved.
- **`FitToListUi`** — widgets/dialogs, built on `FitToListCore`: `CanvasWidget`, `DirectoryTreeWidget`, `FolderSelectionDialog`, `MaxSizeDialog`, `ImageTreeWidget`, `ToolbarIcons`.
- **`FitToList`** (executable) — `main.cpp` + `MainWindow`. `MainWindow.cpp` is *not* in a library (only ever built once, into the app), so `MainWindowTest` compiles it directly alongside the test file rather than linking a lib.

Key classes and how responsibility is split between them:

- **`ImageDocument`** owns the image pixels and every pixel-editing operation (crop, rotate, reverse, reset, straighten, save-size planning and writing). No UI/widget dependency. It never mutates the loaded original — edits apply to a separate working copy (`original_`/`current_`), mirroring the Python original's `original_image`/`current_image` split. Emits `imageChanged()` from every mutator ("pixels changed, redraw/re-check dirty") and `imageLoaded()` only from `load()` ("entirely new image — also drop interaction state like a pending selection or open straighten session").
- **`CanvasWidget`** owns only interaction state — the crop-selection drag state machine and the Straighten tool's guide line — and delegates every actual pixel edit to the `ImageDocument` it's pointed at. Listens for `ImageDocument::imageChanged`/`imageLoaded` to clear its selection (or, on `imageLoaded`, also cancel Straighten).
- **`ImageTreeWidget`** is the source/processed image list (main window's left pane). **`DirectoryTreeWidget`** is the whole-filesystem folder picker used by `FolderSelectionDialog`, built on `QFileSystemModel` (replacing the Python original's hand-rolled lazy-loading tree code).
- **`MainWindow`** orchestrates everything: owns the one `ImageDocument` being edited, the source/processed folder state, and the save workflow (`saveCurrent`/`processAndSave`/`confirmDiscardChanges`/`reduceAllImages`) that the Python original spread across `PhotoEditorApp`.
- **`ToolbarIcons`** draws toolbar icons procedurally with `QPainter` (no bundled image assets), matching the Python original's PIL-based approach.
- **`Config`** persists settings via `QSettings` (organization/application name `"FitToList"`, set in `main.cpp`), replacing the original's hand-rolled JSON file.
- **`FileNaming`** implements the non-destructive save-naming rule: saving a source image writes a new `name_00.ext`, `name_01.ext`, ... file; reopening and saving an already-processed file overwrites it in place (with a confirmation prompt).

`user_manual.html` is copied next to the built executable via a CMake `POST_BUILD` step (not bundled as a Qt resource), so **Help > User Guide** can hand its real filesystem path to `QDesktopServices` — a `qrc:` URL isn't reachable by an external browser.
