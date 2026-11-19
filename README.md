# DupFind — duplicate file finder (C++ / Qt 6)

A Windows desktop duplicate-file finder written in C++17 with Qt 6 Widgets. It walks a
folder tree, groups files by exact size, hashes only the files whose size is not unique
with SHA-256, and reports groups of byte-identical files.

This is the same design as my C# WPF version, rebuilt in C++/Qt. The scan engine was
written first and the UI second, so the expensive logic could be tested without a
window.

## How the scan works

Two passes, which is what keeps it usable on a large tree:

1. **Bucket by size.** Every file is recorded under its exact byte size. Hash collisions
   are impossible across different sizes, so this pass needs no I/O beyond the directory
   walk.
2. **Hash the candidates only.** Sizes with one file are dropped. The rest are hashed
   with SHA-256 in 1 MiB chunks and grouped by `(size, hash)`.

Consequences worth knowing:

- Files are read only when their size is shared, so a tree of unique files is hashed
  zero times.
- Memory is proportional to the duplicate candidates, not to the number of files.
- Empty files are skipped: they are all identical, and reporting them as duplicates is
  noise.
- Symlinks are not followed, so a link cannot pull the walk into a cycle.

## Structure

| Path | What it is |
| --- | --- |
| `src/scan/ScanEngine.*` | The two-pass scanner. Depends only on QtCore, no widgets. |
| `src/ui/DuplicatesModel.*` | `QAbstractTableModel` over the result, with raw sort keys. |
| `src/worker/ScanWorker.*` | `QObject` moved to a `QThread`; emits progress/finished/cancelled. |
| `src/ui/MainWindow.*` | Widgets, signals/slots, `QSettings` for the last folder and window geometry. |
| `tests/` | Three QtTest binaries: engine, model, worker. |

`dupfind_core` is a separate CMake target that links only `Qt6::Core`. A change to the
scanner does not rebuild the widget code, which is why the layer exists.

## Build and test

Requires CMake 3.21+, a C++17 compiler, and Qt 6.5+ with the Widgets and Test modules.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/DupFindQt
```

## Design decisions and tradeoffs

- **Read-only.** The app reports duplicates and leaves deletion to you. Deleting files
  from a scanner needs confirmation, an undo path and careful error handling; shipping a
  half-safe delete would be worse than not shipping one.
- **A worker thread, not `QtConcurrent`.** The worker is a `QObject` with `moveToThread`,
  so cancellation, progress and completion are ordinary queued signals and the lifetime
  is explicit. Cancellation is a `std::atomic<bool>` because that is safe to set from the
  GUI thread at any moment.
- **Cancelled means discarded.** A cancelled scan emits `cancelled()` and the partial
  result is dropped, so a half-finished scan is never shown as a complete one.
- **Raw sort keys.** `Qt::UserRole` holds the size in bytes and the group index, so the
  `QSortFilterProxyModel` sorts numerically instead of alphabetically by the formatted
  text.
- **No per-row widgets.** `QTableView` renders only the visible rows; the earlier WPF
  version needed a virtualizing panel to get the same behaviour.

## Known limits

- Only duplicate files are found, not similar images or near-identical documents.
- Hash comparison is not a byte-by-byte verify, so the (astronomically unlikely) SHA-256
  collision would be reported as a duplicate.
- No installer and no code signing. There is no stored settings migration either; if a
  key changes, the old value is simply ignored.
- Tested on Linux (GCC) and built and tested on Windows in CI. Not yet exercised on a
  large real-world tree; the 100 000-file case that broke the WPF version has not been
  re-run here.

## CI

`.github/workflows/build.yml` builds with MSVC on `windows-latest`, runs the three test
binaries through CTest, then packages the exe with `windeployqt` as a downloadable
artifact.
