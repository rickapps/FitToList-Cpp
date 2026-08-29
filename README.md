# FitToList-Cpp

A desktop photo editor for quickly cropping and resizing multiple images in a folder — a C++/Qt port of [FitToList-Python](https://github.com/rickapps/FitToList-Python), built for the same "batch prep" workflow: point it at a folder of source images and a destination folder, then work through the source images one at a time — drag out a crop, straighten a crooked shot, rotate/flip, and save.

## Status

**Feature-complete.** Every feature in the Python original has been ported:

- Folder-based batch workflow (source + processed folders, with a tree showing every source image next to its processed outputs)
- Interactive crop selection — drag to draw, drag edges/corners to resize, drag inside to move, with live dimensions in the status bar
- Crop to Selection, and double-click inside a selection for Process & Save in one step
- Straighten — drag either end of a guide line (up to 30° either way) to level a tilted photo, fine-tunable without the image shrinking on repeated adjustments
- Rotate left/right, horizontal flip (Reverse Image), and Reset
- Non-destructive edits — the original file is never modified; saving a source image writes a new `name_00.ext`, `name_01.ext`, ... file, while reopening and saving an already-processed file overwrites it in place (with a confirmation prompt)
- Auto-advance to the next unprocessed image after Process & Save
- Optional Max Save Size, applied only when saving (images are shrunk to fit, never enlarged)
- Reduce All Images — batch-shrinks every source image straight to the processed folder in one step, for prepping a folder of camera photos
- Unsaved-changes protection when switching images or folders
- Persisted settings (source/processed folders, max save size) between sessions
- Toolbar buttons for the most common actions, plus a full File/Actions/Help menu and an in-app User Guide

Not yet done: packaging/installers (running from a build directory only, for now).

## Building

**Requirements:** Qt 6 (Widgets and Test components) and CMake 3.16+. On Fedora: `sudo dnf install cmake qt6-qtbase-devel`; on Debian/Ubuntu: `sudo apt install cmake qt6-base-dev`.

```bash
git clone https://github.com/rickapps/FitToList-Cpp.git
cd FitToList-Cpp
cmake -B build -S .
cmake --build build
./build/FitToList
```

On first launch, use **File > Select Folders...** to choose a source folder (where your original images live) and a processed folder (where edited copies will be written). For the full walkthrough of every feature, see [`user_manual.html`](user_manual.html) or open it from inside the app via **Help > User Guide**.

### Running the tests

Unlike the Python original (a GUI script with no automated tests), this port has a full test suite — the pure logic (`ImageDocument`, `FileNaming`, `Config`) is exercised directly, and the widgets/dialogs/main window are driven with `QTest`'s synthetic mouse/keyboard events and modal-dialog polling, run headless via the `offscreen` Qt platform plugin:

```bash
ctest --test-dir build --output-on-failure
```

## For developers

Unlike the Python original's single script, this is a normal small CMake/Qt project: one class (mostly) per header/source pair under `src/`, built into two static libraries (`FitToListCore` for UI-free logic, `FitToListUi` for widgets/dialogs) linked into the `FitToList` executable, with matching test binaries under `tests/`.

- **`ImageDocument`** owns the image pixels and every pixel-editing operation (crop, rotate, reverse, reset, straighten, save-size planning and writing) — no UI/widget dependency, so it's directly unit-testable. It never mutates the loaded original; edits apply to a separate working copy, mirroring the Python original's `original_image`/`current_image` split.
- **`CanvasWidget`** owns only interaction state — the crop-selection drag state machine and the Straighten tool's guide line — and delegates every actual pixel edit to the `ImageDocument` it's pointed at. It listens for `ImageDocument::imageChanged`/`imageLoaded` to clear its selection (or, on a fresh image, also cancel Straighten), the same way every mutator in the Python original called `clear_selection()`.
- **`ImageTreeWidget`** is the source/processed image list (the main window's left pane); **`DirectoryTreeWidget`** is the whole-filesystem folder picker used by `FolderSelectionDialog` — built on `QFileSystemModel`, which replaces almost all of the Python original's hand-rolled lazy-loading tree code.
- **`MainWindow`** orchestrates everything: it owns the one `ImageDocument` being edited, the source/processed folder state, and implements the save workflow (`saveCurrent`/`processAndSave`/`confirmDiscardChanges`/`reduceAllImages`) that the Python original spread across `PhotoEditorApp`.
- **`ToolbarIcons`** draws the toolbar icons procedurally with `QPainter` (no bundled image assets), matching the Python original's PIL-based approach.
- **`Config`** persists settings via `QSettings`, replacing the original's hand-rolled JSON file.

## License

GPLv3 — see [LICENSE](LICENSE).
