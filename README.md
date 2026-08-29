# FitToList-Cpp

A desktop photo editor for quickly cropping and resizing multiple images in a folder — a C++/Qt port of [FitToList-Python](https://github.com/rickapps/FitToList-Python).

## Status

Early skeleton: the app opens an empty window. Crop, straighten, rotate, the file tree, and batch resize are not yet ported.

## Building

Requires Qt 6 (Widgets) and CMake 3.16+.

```bash
cmake -B build -S .
cmake --build build
./build/FitToList
```

## License

GPLv3 — see [LICENSE](LICENSE).
