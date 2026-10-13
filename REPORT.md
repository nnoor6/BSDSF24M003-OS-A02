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
