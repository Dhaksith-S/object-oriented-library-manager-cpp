# Validation report

Validation date: **2026-10-31** (user's Asia/Calcutta calendar date).
Results below come from actual local builds and execution, not assumed coverage.

## Environment

| Item | Actual value |
| --- | --- |
| OS | Microsoft Windows 11 Home Single Language, version 10.0.26300, x64 |
| Compiler | GCC 16.1.0, MinGW-W64 x86_64-ucrt-posix-seh (Brecht Sanders r3) |
| Language | ISO C++17, compiler extensions disabled |
| CMake | 4.3.3 |
| Generator | Ninja |
| Test interpreter | Python 3.14.5 selected by CMake |
| Evidence script | Python 3.11 from the shell's `python` command |
| Git | 2.54.0.windows.1 |
| GitHub CLI | 2.96.0 |

## Builds and tests

Both **Debug** and **Release** configurations built successfully. Project targets
compiled with `-Wall -Wextra -Wpedantic`, with no project-caused compiler warnings.
The final build/test commands are the Windows Ninja commands in the README.

| CTest entry | Debug | Release | Actual checks per configuration |
| --- | --- | --- | --- |
| `models` | Passed | Passed | 292,261 C++ checks |
| `library` | Passed | Passed | 52 C++ checks |
| `storage` | Passed | Passed | 54 C++ checks |
| `cli` | Passed | Passed | 13 Python unittest integration cases |
| **Total** | **4/4 suites passed; 0 failed** | **4/4 suites passed; 0 failed** | **292,367 C++ checks + 13 CLI cases** |

The large model count primarily comes from two checks on each day across a full
400-year Gregorian calendar cycle. It is not a claim of 292,367 distinct test
scenarios. Checks are implemented by an always-active dependency-free harness,
not `assert`, so they execute in Release as well as Debug. Python's unittest suite
launches the real executable with isolated temporary files and subprocess timeouts.

### Coverage exercised

- Book/member constructors, field/ID/copy bounds, blank/long/control-character text,
  eligibility and checked availability.
- Date syntax, leap days, century exceptions, year bounds, negative day addition,
  overflowing arithmetic, loan due/return chronology and duplicate returns.
- Registration and ID generation, duplicate IDs, search by all four fields,
  missing records, inactive membership, three-loan limits and exhausted inventory.
- Issue/return consistency, preservation of other loans, ID-space exhaustion,
  active-loan referential integrity, and merged inventory overflow.
- Save/reload round trips, active loan recovery, missing files, corrupt files,
  duplicate IDs of all entity types, invalid dates/flags/references, footer count
  mismatch, invalid quotes, quoted commas/newlines/CRLF, record and file-size limits.
- Failed writes using real filesystem obstacles (directory destinations, parent
  files, stale staging files, invalid backup destinations), preserving old data.
- Recovery with invalid primary, missing primary, invalid primary and backup,
  preservation of a corrupt primary, and successful save after recovery.
- Repeated sample import, conflicting re-import following a return, and rejection
  of a whole conflicting merge without partial mutations.
- All 14 CLI options, graceful exit and EOF, EOF mid-operation, invalid input
  recovery, failed-save memory rollback, startup errors, help/argument errors,
  sample path/hard-link protection, and backup recovery through real processes.

## Functional execution evidence

[sample-execution.txt](sample-execution.txt) contains real output from two separate
Release executable processes, generated using:

```powershell
python tools/capture_execution.py build-release/library_manager.exe
```

The first run recovers from alphabetic menu input, imports the sample, lists books,
registers fictional member 9, searches by title, issues book 1 as loan 11, confirms
availability changes to 2/3, returns the loan, displays history, saves, and exits.
The second run reloads the same temporary database, lists the registered member,
shows restored availability of 3/3, displays loan history and reports 7 returned
loans. Both processes exited with status 0 and empty stderr. Program output is
unedited; capture labels and supplied stdin are explicitly identified as metadata.
The capture tool removes its disposable database afterward.

## Sample import and materials

The initial workspace was empty. No step-by-step task guide, starter checklist,
separate C++ project requirements document, or official seed CSV was available.
None is claimed to have been reviewed. Requirements came from the supplied Task 03
prompt. The original fictional `data/sample_library.csv` supplies 15 books,
8 members and 10 loans (4 active and 6 returned). Tests use these actual committed
records and verify repeated import, references and availability.

## Issues found and corrected

1. The first compile identified a missing validation-header include in the manager
   test. The include was added and both configurations subsequently compiled cleanly.
2. Review identified that lexical path comparison alone could allow a sample
   database alias on a case-insensitive filesystem or through a hard link. Startup
   now also checks filesystem equivalence; a real hard-link CLI regression test
   confirms the sample is preserved.
3. Documentation portability review identified Python's newer `Path.hardlink_to`
   method. The integration test uses `os.link`, and CMake explicitly requires
   Python 3.8+ to match the documented test interpreter minimum.

## Documentation and diagram

README relative file links were checked against actual files. The README's Windows
Ninja Debug/Release commands were executed. The documented executable usage and
capture command were run. The Mermaid diagram was checked against model fields,
methods, composition and ID associations. No `mmdc` renderer was on PATH, so the
mandatory GitHub-renderable Mermaid source is provided without a static image.
Linux/macOS, Clang and MSVC instructions were not executed on this Windows machine.

## Screenshot status

No screenshots were captured. The available Windows computer-use integration
prohibits automating terminal applications; an authentic reproducible transcript
is supplied under the assignment's allowed fallback. No fake terminal images were
generated. Optional remaining manual action: launch the executable, perform the
README workflow, and capture the actual menu/catalog/search/issue/return/history
windows into `docs/screenshots/`, excluding personal paths or unrelated windows.

## Known limits

- Single process per database; no interprocess lock, encrypted storage or account login.
- Standard C++17 staged replacement is not guaranteed crash-atomic or power-loss
  durable. Backup recovery can restore an older successful snapshot. Stale staging
  files and preserved corrupt files require inspection/moving aside before reuse.
- No snapshot checksum; edits producing another valid snapshot cannot be detected.
- ASCII-only search folding and byte-based table abbreviation; UTF-8 alignment and
  multibyte truncation are not handled. Text is preserved in full in the database.
- 100,000 records per type, 32 MiB snapshots, bounded IDs/copies/text/date ranges.
- Historical active sample loans are overdue, intentionally; no fines or renewal rules.
- Active-count queries scan loan history. This is designed for small-library use.
- Tests check handled filesystem failures and recovery states, not real power-loss
  fault injection or every possible filesystem failure timing.

## GitHub publication status

- Public repository: [Dhaksith-S/object-oriented-library-manager-cpp](https://github.com/Dhaksith-S/object-oriented-library-manager-cpp).
- GitHub metadata confirmed **PUBLIC**, with default branch **main**. The first
  uploaded commit was `09e5680524e1e98a5d64a74d96c9331fcb74e075`.
- The remote recursive file tree matched all **32 tracked files**, including README,
  source, tests, UML, fictional sample records and execution evidence. No builds,
  runtime databases, credentials or personal workstation paths were committed.
- Source release: [v1.0.0](https://github.com/Dhaksith-S/object-oriented-library-manager-cpp/releases/tag/v1.0.0),
  with [release notes](RELEASE_NOTES.md). The tag identifies the validated source;
  GitHub supplies the source archive downloads.
- The project uses an isolated repository rooted in the workspace. No other
  repository was modified, no force push was used, and no optional CI workflow
  permission was required. Build outputs and runtime files are ignored.
