/*
 * Programming Assignment 02: lsv1.0.0
 * This is the source file of version 1.0.0
 * Read the write-up of the assignment to add the features to this base version
 * Usage:
 *       $ lsv1.0.0
 *       $ lsv1.0.0  /home
 *       $ lsv1.0.0  /home/kali/   /etc/
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>


/* ---------- ANSI color codes ---------- */
#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[0;34m"
#define COLOR_GREEN   "\033[0;32m"
#define COLOR_RED     "\033[0;31m"
#define COLOR_PINK    "\033[0;35m"   /* magenta */
#define COLOR_REVERSE "\033[0;7m"
extern int errno;

/* ---------- Global flags ---------- */
int long_listing = 0;    /* -l */
int horizontal   = 0;    /* -x */
static int compare_names(const void *a, const void *b)
{
    const char *name_a = *(const char **)a;
    const char *name_b = *(const char **)b;
    return strcmp(name_a, name_b);
}
/* ---------- Forward declarations ---------- */
/* ---------- Forward declarations ---------- */
void do_ls(const char *dir);
void print_long(const char *dir, const char *name);
void print_columns(const char *dir, char **names, int count, int maxlen);
void print_horizontal(const char *dir, char **names, int count, int maxlen);
void print_colored(const char *name, const struct stat *st);
void print_permissions(mode_t mode);
int  get_terminal_width(void);

/* Decide the color for a file given its name and st_mode,
 * and print the name wrapped in the correct escape codes. */
void print_colored(const char *name, const struct stat *st)
{
    const char *color = NULL;

    /* 1. Symlink (must be checked first via lstat's mode) */
    if (S_ISLNK(st->st_mode)) {
        color = COLOR_PINK;
    }
    /* 2. Directory */
    else if (S_ISDIR(st->st_mode)) {
        color = COLOR_BLUE;
    }
    /* 3. Executable (any execute bit set) */
    else if (st->st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
        color = COLOR_GREEN;
    }
    /* 4. Archive by extension */
    else if (strstr(name, ".tar") || strstr(name, ".gz") ||
             strstr(name, ".zip") || strstr(name, ".tgz") ||
             strstr(name, ".bz2") || strstr(name, ".xz")) {
        color = COLOR_RED;
    }
    /* 5. Special files: char device, block device, FIFO, socket */
    else if (S_ISCHR(st->st_mode) || S_ISBLK(st->st_mode) ||
             S_ISFIFO(st->st_mode) || S_ISSOCK(st->st_mode)) {
        color = COLOR_REVERSE;
    }

    if (color)
        printf("%s%s%s", color, name, COLOR_RESET);
    else
        printf("%s", name);
}
/* ---------- main ---------- */
int main(int argc, char const *argv[])
{
    int opt;
    while ((opt = getopt(argc, (char * const *)argv, "lx")) != -1) {
        switch (opt) {
            case 'l':
                long_listing = 1;
                break;
            case 'x':
                horizontal = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l] [-x] [dir...]\n", argv[0]);
                return 1;
        }
    }

    if (optind == argc) {
        do_ls(".");
    } else {
        for (int i = optind; i < argc; i++) {
            if (argc - optind > 1)
                printf("%s:\n", argv[i]);
            do_ls(argv[i]);
            if (argc - optind > 1)
                puts("");
        }
    }
    return 0;
}

/* ---------- Terminal width ---------- */
int get_terminal_width(void)
{
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1 || w.ws_col == 0)
        return 80;
    return w.ws_col;
}

/* ---------- Permission string ---------- */
void print_permissions(mode_t mode)
{
    char perms[11];

    if (S_ISDIR(mode))       perms[0] = 'd';
    else if (S_ISLNK(mode))  perms[0] = 'l';
    else if (S_ISCHR(mode))  perms[0] = 'c';
    else if (S_ISBLK(mode))  perms[0] = 'b';
    else if (S_ISFIFO(mode)) perms[0] = 'p';
    else if (S_ISSOCK(mode)) perms[0] = 's';
    else                     perms[0] = '-';

    perms[1] = (mode & S_IRUSR) ? 'r' : '-';
    perms[2] = (mode & S_IWUSR) ? 'w' : '-';
    perms[3] = (mode & S_IXUSR) ? 'x' : '-';

    perms[4] = (mode & S_IRGRP) ? 'r' : '-';
    perms[5] = (mode & S_IWGRP) ? 'w' : '-';
    perms[6] = (mode & S_IXGRP) ? 'x' : '-';

    perms[7] = (mode & S_IROTH) ? 'r' : '-';
    perms[8] = (mode & S_IWOTH) ? 'w' : '-';
    perms[9] = (mode & S_IXOTH) ? 'x' : '-';

    perms[10] = '\0';
    printf("%s ", perms);
}

/* ---------- Long listing (-l) ---------- */
void print_long(const char *dir, const char *name)
{
    struct stat st;
    char fullpath[4096];

    snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, name);

    if (lstat(fullpath, &st) == -1) {
        perror("lstat");
        return;
    }

    /* 1. Permissions */
    print_permissions(st.st_mode);

    /* 2. Link count */
    printf("%2lu ", (unsigned long)st.st_nlink);

    /* 3. Owner */
    struct passwd *pw = getpwuid(st.st_uid);
    printf("%-8s ", pw ? pw->pw_name : "unknown");

    /* 4. Group */
    struct group *gr = getgrgid(st.st_gid);
    printf("%-8s ", gr ? gr->gr_name : "unknown");

    /* 5. Size */
    printf("%8ld ", (long)st.st_size);

    /* 6. Modification time */
    char timebuf[64];
    struct tm *tm_info = localtime(&st.st_mtime);
    strftime(timebuf, sizeof(timebuf), "%b %e %H:%M", tm_info);
    printf("%s ", timebuf);

    /* 7. Filename */
    print_colored(name, &st);
    printf("\n");}

/* ---------- Down-then-across columns (default) ---------- */
void print_columns(const char *dir, char **names, int count, int maxlen)
{
    int term_width = get_terminal_width();
    int col_width  = maxlen + 2;
    int cols       = term_width / col_width;
    if (cols < 1) cols = 1;
    int rows       = (count + cols - 1) / cols;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int idx = c * rows + r;
            if (idx >= count) continue;

            /* We need the stat to know the color */
            char fullpath[4096];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, names[idx]);
            struct stat st;
            lstat(fullpath, &st);

            /* For simplicity, print directly with printf.
               Padding uses the raw name length, which may under- or over-pad
               when ANSI codes are present. Real ls handles this with extra logic. */

            if (c == cols - 1 || idx + rows >= count) {
                print_colored(names[idx], &st);
            } else {
                print_colored(names[idx], &st);
                /* Pad with spaces to col_width (approx.) */
                int pad = col_width - (int)strlen(names[idx]);
                if (pad < 0) pad = 0;
                for (int s = 0; s < pad; s++) putchar(' ');
            }
        }
        printf("\n");
    }
}
/* ---------- Horizontal columns (-x) ---------- */
void print_horizontal(const char *dir, char **names, int count, int maxlen)
{
    int term_width = get_terminal_width();
    int col_width  = maxlen + 2;
    int pos        = 0;

    for (int i = 0; i < count; i++) {
        if (pos > 0 && pos + col_width > term_width) {
            printf("\n");
            pos = 0;
        }

        char fullpath[4096];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, names[i]);
        struct stat st;
        lstat(fullpath, &st);

        print_colored(names[i], &st);
        int pad = col_width - (int)strlen(names[i]);
        if (pad < 0) pad = 0;
        for (int s = 0; s < pad; s++) putchar(' ');
        pos += col_width;
    }
    printf("\n");
}
/* ---------- Directory listing ---------- */
#define MAX_FILES 4096

void do_ls(const char *dir)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL) {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    char *names[MAX_FILES];
    int   count  = 0;
    int   maxlen = 0;

    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        names[count] = strdup(entry->d_name);
        if (!names[count]) {
            perror("strdup");
            break;
        }
        int len = (int)strlen(names[count]);
        if (len > maxlen) maxlen = len;
        count++;

        if (count >= MAX_FILES) break;
    }

    if (errno != 0)
        perror("readdir failed");

    closedir(dp);
       if (errno != 0)
        perror("readdir failed");

    
    /* Sort the collected names alphabetically */
    qsort(names, count, sizeof(char *), compare_names);
    if (long_listing) {
        for (int i = 0; i < count; i++)
            print_long(dir, names[i]);
    } else if (horizontal) {
        print_horizontal(dir, names, count, maxlen);
    } else {
        print_columns(dir, names, count, maxlen);
    }    for (int i = 0; i < count; i++)
        free(names[i]);
}
