# Architecture and persistence

## Boundaries and invariants

`Book`, `Member` and `Loan` validate their own fields at construction. Domain objects
do not know about CSV or the console. Models expose getters; only `Loan::returnOn`
changes loan state, and it rejects repeat or pre-issue returns. `Book::available`
checks that active loans do not exceed inventory before subtraction. Member active
loan counts come from the manager; they are never duplicated in stored member state.

`LibraryManager` owns three ID-keyed maps. It checks existence, active membership,
availability, a three-loan member limit and unique IDs before issuing a loan.
Generated IDs are largest existing ID plus one, with an explicit upper bound.
Every loan, including a returned loan, must reference an existing book and member.
`validate()` checks active inventory, active-member eligibility and per-member limits
in linear passes over loan history. Copies are never decremented as a second source
of truth: total copies minus active loans is the only availability calculation.

Public operations cannot edit entity fields in place through map getters. Sample
import constructs a candidate library and validates all merged records. Conflicts
reject the candidate; identical IDs/content are skipped. Returning a sample's loan
then importing its original active version is an explicit conflict, not a reset.

The console reads entire lines and uses bounded `from_chars` integer conversion.
Every mutation occurs on a candidate manager. `StorageManager::save` must succeed
before that candidate replaces live memory. EOF cancels an unfinished prompt. Read
operations do not write files. Exit is safe because changes save immediately;
option 12 intentionally permits an explicit additional save.

## Calendar design

`Date` accepts Gregorian years 1900 through 9999 and validates month length and leap
years (including century exceptions). Dates convert to an ordinal day count with
integer arithmetic. Day addition uses a 64-bit intermediate and a binary search for
the resulting year, with no DST, local-time or platform-extension dependency. Model
tests exercise every day in a full 400-year Gregorian cycle. `today()` obtains the
UTC date through standard C++17 `time`/`gmtime`. This single-threaded application
does not share `gmtime`'s static buffer across threads.

Due dates cannot precede issue dates. Return dates cannot precede issue dates;
returns after due dates are accepted. Imported dates may be past or future and are
not rewritten. Issuing through the UI uses today's UTC date plus 14 calendar days.

## Snapshot format

UTF-8-compatible byte text, comma separated, version **1**:

```csv
LIBRARY_SNAPSHOT,1
BOOK,1,"Example title","Example author","Fiction",2
MEMBER,1,"Example Member","member@example.invalid",1
LOAN,1,1,1,2026-01-01,2026-01-15,
END,1,1,1
```

| Record | Columns in order |
| --- | --- |
| Header | `LIBRARY_SNAPSHOT`, version `1` |
| BOOK | type, ID, title, author, genre, total copies |
| MEMBER | type, ID, name, contact, active flag (`0` or `1`) |
| LOAN | type, ID, book ID, member ID, issue date, due date, return date (empty means active) |
| Footer | `END`, book count, member count, loan count |

Entity rows can appear in any order between the header and footer. Output is
deterministic: books, then members, then loans, each sorted by ID. The footer detects
missing records and common truncation; it is not a cryptographic checksum. Arbitrary
edits that produce another fully valid snapshot cannot be distinguished from intended
data. Unknown versions, unknown record types, extra/missing fields, blank rows,
duplicate IDs, invalid state and count mismatches reject the entire snapshot.

CSV parsing accepts LF or CRLF between rows, a final row without a newline, commas
inside quoted fields, doubled quote escapes, and newlines inside quoted fields.
Quotes may only open a field at its beginning. Only a delimiter or row terminator
may follow a closing quote. Bare CR outside a quoted field is rejected. There is no
automatic delimiter/header guessing or UTF-8 BOM removal. Text may include tab/CR/LF
but no other ASCII controls; the CLI replaces these whitespace controls in tables.
Numeric fields contain digits only (no signs, spaces or exponents).

At most 100,000 books, 100,000 members, 100,000 loans and 32 MiB total are accepted.
Fields are limited to 512 bytes; IDs and copy counts have explicit numeric ceilings.
Per-record parsing errors include the logical CSV record number. Whole-library
integrity checks run after row construction and before accepting the snapshot.

## Save sequence and recovery

For a configured path such as `runtime/library.csv`:

1. Validate the library and serialize the complete snapshot in memory.
2. Reject directory destinations and existing `.tmp`/`.stage` companions.
3. Create missing parent directories, write `.tmp`, flush/close it, then reparse it.
4. If the primary is valid, copy it to `.stage`, reparse, and promote it to `.bak`.
   During backup replacement, the valid primary and new staged snapshot still exist.
5. If the primary is invalid, preserve it as `.corrupt`. Preserve an existing valid
   recovery backup; refuse to overwrite a previously preserved `.corrupt` file.
6. Remove the old primary and rename `.tmp` to the primary path. A valid previous
   backup remains available. Handled failures remove only staging files made by
   this save and propagate an error to the caller.

Startup first tries the primary, then the backup. If neither exists, it starts empty.
A missing or invalid primary with a valid backup produces an explicit recovery
warning. If both fail validation, startup stops and preserves the files. Startup
never promotes partial staging files automatically. Stale `.tmp`/`.stage` files
may be remnants of interruption and require inspection and moving aside before a
new save. An empty primary is corrupt, not a signal to discard saved state.

There is a replacement window where only the backup exists. A failed final rename
may therefore require restart recovery. On the very first save, there is no prior
snapshot to back up; an interruption can lose that first unacknowledged transaction.
Reopening/reparsing a staged file checks application-level consistency, not durable
hardware writes. Standard C++17 provides no guaranteed crash-atomic replacement,
portable `fsync`, or power-loss durability. Backup recovery restores the previous
successful snapshot, so it can lose the most recent saved change. No concurrency
locking is provided: **only one process may use a database**. Network/cloud-synced
runtime files are not a supported concurrency or durability mechanism.

## Testing strategy

Model tests exercise boundary validation and calendar arithmetic. Manager tests
exercise real issue/return operations, invariants, import conflicts and ID
exhaustion. Storage tests construct malformed fixtures, round-trip quoted content,
recover backups and use actual filesystem obstacles to simulate failed writes.
They check record/file ceilings and partial-import rejection. CLI tests launch
separate executable processes to check prompts, input errors, persistence, sample
alias protection and failed-save rollback. They use no mocks of the production
storage implementation. All fixtures live in temporary directories and are removed
when tests finish. The CLI and capture script use timeouts to detect hangs.

## Scope and provenance

The supplied prompt was the available specification. The initial workspace had no
step-by-step guide, starter checklist, additional requirements document, or official
seed CSV. The committed sample snapshot is newly authored fictional data, documented
in the README. There is no CLI for editing/deleting entities or changing membership
status; inactive members are representable in validated imports. There is no
authentication, encryption, renewal, fine or overdue-notification subsystem.
