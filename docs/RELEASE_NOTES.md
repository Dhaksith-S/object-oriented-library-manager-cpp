# Object-Oriented Library Manager v1.0.0

Complete Task 03 C++17 source release for a small lending library.

- Encapsulated Book, Member, Loan, Date, LibraryManager and StorageManager classes.
- All 14 console actions, ASCII search, generated member/loan IDs, borrowing limits,
  issue/return dates and derived inventory availability.
- Versioned CSV snapshots, automatic saves/reload, strict validation, staged writes,
  backup recovery and all-or-nothing sample imports.
- Original fictional sample: 15 books, 8 members and 10 active/returned loan records.
- README, matching Mermaid UML, architecture notes, validation report and authentic
  execution transcript with a reproducible capture script.
- Windows GCC 16.1.0 Debug/Release validation: 4/4 CTest suites per configuration;
  292,367 C++ checks and 13 real CLI integration cases per configuration.

Build from source with CMake; see README for Windows and Linux/macOS commands.
No prebuilt binaries are attached. Only Windows GCC was actually exercised.
Use one process per database. Portable C++17 file replacement cannot guarantee
crash atomicity or power-loss durability. Backup recovery may restore an older
snapshot. Search folds ASCII letters only. Screenshots were unavailable through
the permitted automation; the repository includes real two-process execution output.

MIT licensed. No paid infrastructure or external database is required.
