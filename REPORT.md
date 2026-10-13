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
