# Programming Assignment 02 — Report

**Student Roll No:** BSDSF24M003
**Course:** Operating Systems
**Project:** Re-engineering the `ls` Utility in C
**Repository:** https://github.com/nnoor6/BSDSF24M003-OS-A02

---

## Overview

This project re-implements the Unix `ls` command in C, adding features
incrementally across six versions from v1.1.0 to v1.6.0. Each version is
tagged in Git and published as a GitHub Release with a compiled binary.

### Implemented Features

| Version | Feature | Flag |
|---------|---------|------|
| v1.1.0  | Long listing format | `-l` |
| v1.2.0  | Column display, down then across | (default) |
| v1.3.0  | Horizontal column display | `-x` |
| v1.4.0  | Alphabetical sort | (all modes) |
| v1.5.0  | Colorized output by file type | (all modes) |
| v1.6.0  | Recursive listing | `-R` |

### System Calls Used

`opendir`, `readdir`, `closedir`, `stat`, `lstat`, `getpwuid`, `getgrgid`,
`localtime`, `strftime`, `ioctl` (`TIOCGWINSZ`), `getopt`.

### Build

    make
    ./bin/ls [-l] [-x] [-R] [dir...]

---

## Feature 1: Project Setup

Repository initialized, starter code copied into `src/lsv1.0.0.c`,
Makefile written, build verified. Project structure:

    BSDSF24M003-OS-A02/
    ├── src/lsv1.0.0.c
    ├── bin/
    ├── obj/
    ├── man/
    ├── Makefile
    ├── README.md
    └── REPORT.md

---

## Feature 2: Long Listing Format (-l)

### Q1: Difference between stat() and lstat()

`stat()` follows symbolic links and reports information about the target file.
`lstat()` does NOT follow symbolic links — it reports information about the
link itself. In the ls command, lstat() is appropriate when we want to display
information about a symbolic link (e.g., show the link with 'l' type and its
own size), rather than the file it points to. This is what real `ls -l` does by
default.

### Q2: Extracting file type and permissions from st_mode

The `st_mode` field is a bitmask. The upper bits encode the file type, and the
lower 12 bits encode permissions and special bits.

- File type is tested with macros like `S_ISDIR(mode)`, `S_ISLNK(mode)`, which
  internally do `((mode & S_IFMT) == S_IFDIR)`.
- Permission bits are tested with macros like `S_IRUSR`, `S_IWUSR`, `S_IXUSR`
  (owner), `S_IRGRP`, `S_IWGRP`, `S_IXGRP` (group), `S_IROTH`, `S_IWOTH`,
  `S_IXOTH` (others). Example: `(mode & S_IRUSR)` is nonzero if the owner has
  read permission.
- Special bits (`S_ISUID`, `S_ISGID`, `S_ISVTX`) can also be checked the same
  way.

By combining these checks we construct the 10-character permission string
shown by `ls -l` (e.g., `drwxr-xr-x`).

---

## Feature 3: Column Display (Down Then Across)

### Q1: Logic for "down then across" and why a single loop is insufficient

In "down then across", items are laid out column-by-column: the first `rows`
items fill the first column top-to-bottom, the next `rows` items fill the
second column, and so on. To print them, we cannot simply loop
`for (i = 0; i < count; i++)` because that would print left-to-right (which is
the `-x` horizontal mode). Instead, we must iterate row by row, and for each
row pick the item that belongs to that row in each column. The formula is:

    index = column * total_rows + row

This mapping produces the "down then across" effect. A single loop would
produce "across" order.

### Q2: Purpose of ioctl(TIOCGWINSZ) and limitation of fixed-width fallback

`ioctl()` with `TIOCGWINSZ` queries the terminal driver for the current
terminal window size (rows and columns). This lets ls adapt its output to the
actual terminal width so the user gets the maximum number of columns that fit.
A fixed fallback (e.g., 80 columns) wastes space on a wide terminal and
overflows on a narrow one. The fixed fallback is only useful when output is
redirected (e.g., `ls > file`), because there is no terminal to query.

---

## Feature 4: Horizontal Column Display (-x)

### Q1: Complexity comparison

The "across" (horizontal) mode is significantly simpler. It requires only a
single pass through the list of filenames: print each name padded to a fixed
column width, and whenever the current horizontal position plus the next
column width would exceed the terminal width, print a newline and reset the
position. No pre-calculation of rows is needed.

The "down then across" mode is more complex because items are laid out
column-first. Before printing anything, we must know the total count, the
number of columns that fit, and then compute the number of rows with
`rows = ceil(count / cols)`. Then we iterate row-by-row and compute the array
index for each cell with `index = column * rows + row`.

### Q2: Strategy for managing display modes

A single integer flag is used for each mode:

    int long_listing = 0;   /* -l */
    int horizontal   = 0;   /* -x */

`getopt()` sets the appropriate flag to 1 during argument parsing. In `do_ls()`,
after all filenames are collected into the array, we check flags in priority
order:

    if (long_listing)      -> print_long() for each entry
    else if (horizontal)   -> print_horizontal()
    else                   -> print_columns()      (default vertical)

If both `-l` and `-x` are given, `-l` wins, matching real ls behavior.

---

## Feature 5: Alphabetical Sort

### Q1: Why read all entries before sorting, drawbacks

`qsort()` requires the entire set of items to be available at once so it can
compare and swap them. Since `readdir()` returns entries one at a time and in
an unspecified order, we must first store every entry (a copy of each filename,
because readdir reuses its internal buffer). Only after the full list is
collected can we sort it and display the sorted result.

Drawbacks for very large directories:
- Memory grows linearly with the number of entries; huge directories can
  exhaust RAM.
- Sorting is O(n log n); with millions of entries this is costly.
- The program must wait for `readdir` to finish before printing anything, so
  the user sees no output during the scan.

### Q2: The qsort comparison function

Signature:

    int compare(const void *a, const void *b);

`qsort` does not know what type of data it is sorting, so it uses `void *`.
Our array elements are `char *`, so inside the comparison function we cast
each `const void *` back to `const char **` and dereference once to get the
actual `char *`. We then call `strcmp` on the two strings.

The function must take `const void *` arguments because qsort's interface
guarantees only that it will pass pointers to elements — it does not modify
them, and it needs a single generic signature that works for any data type.
Returning a negative, zero, or positive int tells qsort the relative order.

---

## Feature 6: Colorized Output Based on File Type

### Q1: How ANSI escape codes work, green sequence

ANSI escape codes are character sequences that begin with the ESC character
(ASCII 27, often written `\033` or `\e`), followed by `[`, then a numeric
code, then a letter (usually `m` for "set graphics mode"). The terminal
interprets these sequences as instructions to change text attributes (color,
bold, underline) instead of printing them as visible characters.

To print text in green and then reset to default:

    "\033[0;32m"   start green
    ...text...
    "\033[0m"      reset to default

`0;32` means: 0 = normal intensity, 32 = green foreground. Other common
foreground codes: 30 black, 31 red, 33 yellow, 34 blue, 35 magenta, 36 cyan,
37 white. `1` instead of `0` gives bright versions.

### Q2: Which st_mode bits indicate an executable

The executable permission bits in `st_mode` are:

    S_IXUSR  (0100)  owner  may execute
    S_IXGRP  (0010)  group  may execute
    S_IXOTH  (0001)  others may execute

To check whether any user can execute the file:

    if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) { ... }

Each bit is tested with bitwise AND against the corresponding mask. If the
result is nonzero for any of the three, the file is considered executable
and ls colors it green.

---

## Feature 7: Recursive Listing (-R)

### Q1: What is a base case, what stops our recursive ls

A base case is the condition under which a recursive function stops calling
itself. Without it, recursion would continue forever (or until the program
runs out of stack memory).

In our recursive ls, the base case is: when the current directory contains
no subdirectories. In that case the loop over collected names finds no entry
whose `lstat()` reports `S_ISDIR`, so no further recursive call is made, and
the function returns normally. We also avoid an infinite loop by skipping
entries whose name starts with `.`, which excludes `.` and `..`. If we didn't
skip these, we would recurse into `.` forever.

### Q2: Why build full path before recursive calls

We must build the full path (e.g., `parent/subdir`) before making the
recursive call because, once we enter the child directory, the process's
current working directory is unchanged. Linux directories are identified by
path, not by a global "current subdirectory".

If we simply called `do_ls("subdir")` from inside `do_ls("parent")`, the
`opendir("subdir")` call would look for "subdir" relative to the process's
current working directory (the top-level directory where the user launched
the program), NOT relative to "parent". In most cases this lookup would
fail and the subdirectory would not be listed.

By constructing the path "parent/subdir", we give `opendir()` a path that
resolves correctly regardless of where the process was launched.

---

## Conclusion

All six features were implemented incrementally with a Git branch per
feature, an annotated tag, and a GitHub Release with the compiled binary
attached. The final `ls` clone supports `-l`, `-x`, `-R`, alphabetical
sorting, and colorized output, closely matching the behavior of the
standard Unix `ls`.
