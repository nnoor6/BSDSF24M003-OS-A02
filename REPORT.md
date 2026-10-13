## Feature 2: Long Listing Format (-l)

### Q1: Difference between stat() and lstat()
`stat()` follows symbolic links and reports information about the target file.
`lstat()` does NOT follow symbolic links — it reports information about the link itself.
In the ls command, lstat() is appropriate when we want to display information about
a symbolic link (e.g., show the link with 'l' type and its own size), rather than
the file it points to. This is what the real ls -l does by default.

### Q2: Extracting file type and permissions from st_mode using bitwise operators
The st_mode field is a bitmask. The upper bits encode the file type, and the lower
12 bits encode permissions and special bits.
- To test the file type, we use macros like S_ISDIR(mode), S_ISLNK(mode), which
  internally do something like ((mode & S_IFMT) == S_IFDIR).
- To test individual permission bits, we use macros like S_IRUSR (owner read),
  S_IWUSR (owner write), S_IXUSR (owner execute), and similarly for group and others.
  Example: (mode & S_IRUSR) is non-zero if the owner has read permission.
- Special bits (setuid, setgid, sticky) are checked with S_ISUID, S_ISGID, S_ISVTX.
By combining these checks we can construct the 10-character permission string
shown by ls -l (e.g., "drwxr-xr-x").

## Feature 3: Column Display (Down Then Across)

### Q1: Logic for "down then across" and why a single loop is insufficient
In "down then across", items are laid out column-by-column: the first
`rows` items fill the first column top-to-bottom, the next `rows` items
fill the second column, and so on. To print them, we cannot simply loop
`for (i = 0; i < count; i++)` because that would print left-to-right
(which is the -x horizontal mode). Instead, we must iterate **row by
row**, and for each row, pick the item that belongs to that row in each
column. The formula is:
    index = column * total_rows + row
This mapping is what produces the "down then across" effect. A single
loop would produce "across" order, which is only correct for -x.

### Q2: Purpose of ioctl(TIOCGWINSZ) and the limitation of a fixed-width fallback
ioctl() with the TIOCGWINSZ request queries the terminal driver for the
current terminal window size (rows and columns). This lets ls adapt its
output to the actual terminal width, so users get the maximum number of
columns that fit. If we instead used a fixed fallback (say 80 columns),
then on a wider terminal we would waste space and produce fewer columns
than possible, and on a narrower terminal the output would overflow or
wrap awkwardly. The fixed fallback is only useful when the output is
redirected (e.g., `ls > file`), because there's no terminal to query.

## Feature 4: Horizontal Column Display (-x)

### Q1: Compare implementation complexity of "down then across" vs "across"

The "across" (horizontal) mode is significantly simpler. It requires only a
single pass through the list of filenames: print each name padded to a fixed
column width, and whenever the current horizontal position plus the next
column width would exceed the terminal width, print a newline and reset the
position. No pre-calculation of rows is needed.

The "down then across" (vertical) mode is more complex because the items are
laid out column-first. Before printing anything, we must:
  1. Know the total number of items (count).
  2. Know the number of columns that fit (based on terminal width and the
     longest filename).
  3. Compute the number of rows = ceil(count / cols).
  4. Then iterate row-by-row and, for each cell, compute the array index using
     the formula: index = column * rows + row.

That pre-calculation and the index math are what make vertical mode harder.

### Q2: Strategy for managing display modes

A single integer flag is used for each display mode:
    int long_listing = 0;   /* -l */
    int horizontal   = 0;   /* -x */

During argument parsing, getopt() sets the appropriate flag to 1. Later, in
do_ls(), after all filenames are collected into the array, we check the flags
in priority order:
    if (long_listing)      -> call print_long() for each entry
    else if (horizontal)   -> call print_horizontal()
    else                   -> call print_columns()   (default vertical)

If both -l and -x are given, -l wins (long listing takes precedence), which
matches how real ls behaves.

## Feature 5: Alphabetical Sort

### Q1: Why read all entries before sorting, and drawbacks for large directories
qsort() requires the entire set of items to be available at once, so that it
can reorder them by comparing and swapping. Since readdir() returns entries
one at a time and in an unspecified order, we must first store every entry
(a copy of each filename, since readdir reuses its internal buffer). Only
after the full list is collected can we sort it and then display the sorted
result.

Drawbacks for directories containing millions of files:
  - Memory usage grows linearly with the number of entries; a huge directory
    can exhaust RAM.
  - Sorting cost is O(n log n); with millions of entries this becomes
    significant.
  - Directory I/O becomes slower, and the program must wait for readdir to
    finish before printing anything, so the user sees no output during the
    scan (real ls has optimizations like deferred sorting by column order).

### Q2: The qsort comparison function

Signature:
    int compare(const void *a, const void *b);

qsort does not know what type of data it is sorting, so it treats every
element as a void pointer. Our array elements are of type char *, so inside
the comparison function we cast each const void * back to const char ** and
dereference once to get the actual char *. We then call strcmp on the two
strings.

The function must take const void * arguments because qsort's interface
guarantees only that it will pass pointers to elements — it does not modify
them, and it needs a single generic signature that works for any data type.
Returning a negative, zero, or positive int tells qsort the relative order
of the two elements.



## Feature 6: Colorized Output Based on File Type

### Q1: How ANSI escape codes work, and the code sequence for green
ANSI escape codes are short character sequences that begin with the ESC
character (ASCII 27, often written as `\033` or `\e`), followed by `[`,
then a numeric code, then a letter (usually `m` for "set graphics mode").
The terminal interprets these sequences as instructions to change text
attributes (color, bold, underline, etc.) instead of printing them as
visible characters.

To print text in green and then reset to the default:
    "\033[0;32m"   start green
    ...text...
    "\033[0m"      reset to default

`0;32` means: 0 = normal intensity, 32 = green foreground. Other common
foreground codes are 30 = black, 31 = red, 33 = yellow, 34 = blue,
35 = magenta, 36 = cyan, 37 = white. Using 1 instead of 0 gives bright
versions (e.g., `\033[1;32m` = bright green).

### Q2: Which st_mode bits indicate an executable file
The executable permission bits in st_mode are:
    S_IXUSR  (0100)  owner  may execute
    S_IXGRP  (0010)  group  may execute
    S_IXOTH  (0001)  others may execute

To check whether any user can execute the file, we do:
    if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) { ... }

Each bit is tested with bitwise AND against the corresponding mask. If the
result is nonzero for any of the three, the file is considered executable
by at least someone, and ls colors it green.
