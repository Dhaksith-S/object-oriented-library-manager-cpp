# Object-Oriented Library Manager

A portable C++17 console application for managing a small lending library. Books,
members, and loan history survive restarts in one validated local CSV snapshot.
Every successful change is saved before the interface reports success.

**Task 03 objective:** model a real-world problem using maintainable object-oriented
design. This repository includes the implementation, UML, fictional sample records,
automated tests, and authentic execution evidence. Licensed under the [MIT license](LICENSE).

## Features

- Book registration, catalog tables, exact ID lookup, and title/author/genre substring search.
- Member registration with generated IDs, membership status, and active borrowing counts.
- Issue and return workflows; availability is derived from active loans.
- Three active loans per member, 14-day due dates, and permanent returned-loan history.
- Strict numeric/text/date validation, meaningful empty states, and recovery after input errors.
- Automatic saving and startup loading; staged snapshot verification and previous-save backups.
- Transactional imports: identical records are skipped; conflicting IDs reject the whole import.
- Original sample catalog: **15 books, 8 members, 10 loans (4 active, 6 returned)**.
- Dependency-free C++ test harness, Python subprocess integration tests, and CTest.

## Architecture and OOP concepts

| Class | Responsibility |
| --- | --- |
| `Book` | Validated ID, title, author, genre and total copies; checked availability calculation. |
| `Member` | Validated identity/contact, membership status and borrowing eligibility. |
| `Loan` | Book/member references and calendar dates; one-way active-to-returned transition. |
| `LibraryManager` | Owns collections, generates member/loan IDs, enforces business rules, searches and imports. |
| `StorageManager` | Parses, validates, serializes, saves and recovers versioned snapshots. |
| `Date` | Validates Gregorian dates and performs portable day arithmetic. |

Classes and objects represent library entities. Private fields and validated
constructors provide **encapsulation**. Public methods expose an **abstraction** of
borrowing and returning. `LibraryManager` uses **composition** to own `std::map`
collections; its getters return const views. Methods and access control prevent
callers from editing inventory or loan state behind the manager's back. File
handling belongs to storage, while prompts and formatting belong to `main.cpp`.
This separation keeps business logic directly testable. No inheritance is needed.

See the [UML class diagram](docs/CLASS_DIAGRAM.md) and
[architecture and storage design](docs/ARCHITECTURE.md).

## Technology and structure

C++17 standard library, CMake 3.16+, a C++17 compiler, and Python 3.8+ for tests.
There are no downloaded C++ dependencies. Python is unnecessary when configuring
with `-DBUILD_TESTING=OFF`.

```text
include/                 Models, Date, manager/repository APIs, validation helpers
src/                     Model implementations, business logic, storage and CLI
tests/                   Models, business rules, storage and CLI integration suites
data/sample_library.csv  Original fictional sample snapshot
docs/CLASS_DIAGRAM.md     GitHub-renderable Mermaid UML
docs/ARCHITECTURE.md      Rules, format, transaction and recovery details
docs/VALIDATION_REPORT.md Actual machine/build/test results and limitations
docs/sample-execution.txt Unedited outputs from two real application processes
tools/capture_execution.py Reproducible execution-evidence capture
CMakeLists.txt           Build targets and CTest registration
LICENSE                  MIT
```

## Build on Windows

Run commands from the repository root. Install either a MinGW GCC toolchain with
Ninja available on `PATH`, or Visual Studio's Desktop development with C++ tools.
Ensure CMake and Python 3 are discoverable. The actual validation used GCC/Ninja.

**GCC / Ninja (PowerShell):**

```powershell
cmake -S . -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
ctest --test-dir build-debug --output-on-failure
.\build-debug\library_manager.exe

cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
ctest --test-dir build-release --output-on-failure
```

**Visual Studio developer terminal:**

```powershell
cmake -S . -B build-vs
cmake --build build-vs --config Debug
ctest --test-dir build-vs -C Debug --output-on-failure
.\build-vs\Debug\library_manager.exe
cmake --build build-vs --config Release
ctest --test-dir build-vs -C Release --output-on-failure
```

Keep the MinGW runtime DLL directory on `PATH` when running GCC-built executables.
Visual Studio instructions are provided for portability; MSVC was not tested here.

## Build on Linux / macOS

Install CMake, Python 3 and GCC or Clang using your platform's usual tools. Run:

```sh
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
ctest --test-dir build-debug --output-on-failure
./build-debug/library_manager

cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
ctest --test-dir build-release --output-on-failure
```

These are standard CMake commands; Linux/macOS execution was not performed in this
Windows validation. GCC/Clang builds enable `-Wall -Wextra -Wpedantic`; MSVC uses `/W4`.

## Run and import the sample

Start from the repository root so the default sample path resolves. Startup uses
`runtime/library.csv`; missing saved data starts an empty library. Select **11**
to import the fictional sample, then **2** for the catalog and **13** for totals.
Nothing is imported automatically into an existing library.

Override paths when needed (quote paths containing spaces):

```powershell
.\build-debug\library_manager.exe --db runtime/demo.csv --sample data/sample_library.csv
.\build-debug\library_manager.exe --help
```

Database and import source paths must be distinct, including aliases and the
database's backup/staging companions. Keep custom databases under ignored
`runtime/`. Re-importing unchanged samples is a no-op. If a sample loan has since
been returned, importing its original active version is a conflict; the entire
import is rejected without undoing the return. Use a new `--db runtime/new-demo.csv`
to repeat the original demonstration.

### Menu walkthrough

| Option | Action and inputs |
| --- | --- |
| 1 | Add Book: unique positive book ID, title, author, genre, total copies. |
| 2 | View All Books: aligned catalog with available/total copies. |
| 3 | Search Books: enter `title`, `author`, `id` or `genre`, then query. |
| 4 | Register Member: name and contact; receives the next unique member ID. |
| 5 | View Members: contact, membership status and active loan count. |
| 6 | Issue / Borrow Book: book ID and active member ID. |
| 7 | Return Book: active loan ID. |
| 8 | View Active Loans: outstanding loans only. |
| 9 | View Loan History: active and returned records with dates. |
| 10 | Check Book Availability: book ID. |
| 11 | Import Sample Library Data: validated merge from the configured source. |
| 12 | Save Library Records: explicit additional snapshot save. |
| 13 | Library Summary: book/member counts, active/returned loans, total copies. |
| 14 | Exit: successful changes have already been saved. |

### Example borrowing and return

With a fresh database: import sample (**11**), register `Taylor Example` with
`taylor@example.invalid` (**4**, generated member ID **9**), and search for
`CLOCKWORK` in `title` (**3**). Issue book **1** to member **9** (**6**, generated
loan ID **11**). Availability (**10**, book **1**) changes from **3/3** to **2/3**.
Return loan **11** (**7**) and availability returns to **3/3**. History (**9**)
retains the returned loan. Save (**12**), exit (**14**) and restart with the same
database to inspect the saved member and loan. Dates come from the machine's UTC
calendar date; the due date is 14 calendar days after issue.

## Persistence and recovery

One CSV contains all books, members and loans, with a version header and record
count footer. Fields support commas, escaped quotes, and quoted newlines. The
whole file is parsed and validated before any state is accepted. Duplicate IDs,
invalid dates, broken references, unavailable active inventory, and excessive
borrowing are rejected. See the [exact format](docs/ARCHITECTURE.md#snapshot-format).

Saves write and verify `library.csv.tmp`, stage a validated previous snapshot,
preserve it as `library.csv.bak`, then replace the primary. On startup, an invalid
or missing primary falls back to a valid backup with a visible warning. A backup
represents the **previous successful save**, so recovery can lose the newest
transaction. If neither file is valid, startup fails and preserves both files.
Saving after recovery preserves the invalid primary as `.corrupt` for inspection.
Existing `.corrupt` or stale `.tmp`/`.stage` files block the relevant save; inspect
and move them aside, then retry. They are never silently trusted as saved data.

The CLI saves a candidate state before replacing live memory. A failed save reports
an error and retains the previous in-memory state. Standard C++17 cannot guarantee
crash-atomic replacement or power-loss durability, and does not offer portable
`fsync`. **Use one process per database.** There is no interprocess lock or network
filesystem guarantee. Avoid running against a concurrently synced database; the
source project may live in a synced folder, but runtime files should be local.

## Validation and error handling

- IDs: **1–1,000,000,000**. Generated IDs advance beyond the largest existing ID;
  exhaustion is reported rather than wrapping or recycling identifiers.
- Copies: **0–1,000,000** per book. Availability can never be negative or exceed total copies.
- Text: **1–512 bytes**, nonblank, with invalid control characters rejected.
  Contact is a required free-form field; it need not be an email address.
- Dates: strict `YYYY-MM-DD`, Gregorian years **1900–9999**. Due/return dates cannot
  precede issue. Late returns are valid. Imported historical dates are preserved.
- At most **100,000 records per entity type** and **32 MiB** per snapshot; numeric
  parsing and date arithmetic reject overflow. Summary copy totals use a 64-bit accumulator.
- Invalid numeric input, duplicate records, unknown IDs, unavailable books,
  inactive members, borrowing limits and duplicate returns produce readable errors.
- EOF during a prompt cancels the unfinished operation and exits gracefully.
  All prior successful operations remain saved. Long fields are abbreviated in
  tables but retained in full in storage. No raw terminal-control codes are accepted.

Search folds ASCII `A–Z` only; ID search is exact. UTF-8 bytes are preserved, but
Unicode case folding and display-width alignment are unsupported. Truncation is
byte-based and may split a multibyte character. This is a small-library teaching
application with no user login, encryption, fine calculation or reminder service.
Active counts scan loan history; very large catalogs can have slower table output.

## Sample provenance and assignment materials

No assignment guide, starter checklist, separate C++ requirements document, or
official library-records seed CSV was present in the initially empty workspace.
The implementation follows the requirements in the supplied Task 03 prompt.
[data/sample_library.csv](data/sample_library.csv) is an **original synthetic
dataset**, not an official seed. All people, titles and contacts are fictional;
`example.invalid` addresses are deliberately unusable. It demonstrates commas,
escaped quotes, long titles, zero inventory, inactive membership and loan history.
The old active sample loans are intentionally overdue; there is no renewal/fine rule.

## Automated tests and execution evidence

```powershell
ctest --test-dir build-debug --output-on-failure -V
python tools/capture_execution.py build-release/library_manager.exe
```

CTest runs `models`, `library`, `storage`, and `cli`. Tests cover constructors,
eligibility, a full 400-year date cycle, business rules, ID exhaustion, CSV edge
cases, duplicate references, transactional imports, record/file limits, write
failures, backups, and real subprocess persistence. Tests use isolated temporary
directories; they never edit committed samples. The harness remains active in
Release builds. The CLI suite uses Python `unittest` and launches real executables.

See [actual validation results](docs/VALIDATION_REPORT.md) and the
[two-process execution transcript](docs/sample-execution.txt). The transcript
shows input recovery, catalog, registration, search, issue, changed availability,
return, history, save, restart, and persisted records. Capture metadata is labeled;
application output is unedited.

Screenshots were not captured because the available computer-use integration
prohibits terminal automation. The transcript is the execution proof for this
submission. To add images manually, run the example workflow and capture the
actual menu, catalog, search, issue, return, and history windows into
`docs/screenshots/`. Do not publish a terminal frame containing personal paths.
The UML is supplied as GitHub-renderable Mermaid; no static image was exported.

## Requirement compliance

- [x] Multi-file C++17 application with Book, Member, Loan, LibraryManager and StorageManager.
- [x] Encapsulation, composition, const views and separate UI/business/storage layers.
- [x] All 14 displayed menu options implemented; search, issue, return and inventory tracking.
- [x] Validated member records, generated IDs, inactive-member checks and borrowing limits.
- [x] Versioned local persistence, automatic reload, staged writes and backup recovery.
- [x] Transactional sample import and original fictional dataset meeting minimum counts.
- [x] Matching UML, architecture notes, comprehensive README and MIT license.
- [x] Actual Debug and Release builds and passing automated suites on Windows GCC.
- [x] Real restart execution transcript; screenshot limitation and manual action documented.
- [x] Public repository and `v1.0.0` release; see publication evidence in the validation report.

Only Windows GCC was exercised. Build instructions for other platforms describe
intended portability, not tested platform coverage.
