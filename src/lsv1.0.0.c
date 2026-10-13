/*
* Programming Assignment 02: lsv1.0.0
* This is the source file of version 1.0.0
* Read the write-up of the assignment to add the features to this base version
* Usage:
*       $ lsv1.0.0 
*       % lsv1.0.0  /home
*       $ lsv1.0.0  /home/kali/   /etc/
*/
#include <sys/ioctl.h>
#include <unistd.h>      
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>      // getpwuid
#include <grp.h>      // getgrgid
#include <time.h>     // ctime
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>


int long_listing = 0;

int get_terminal_width(void)
{
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1 || w.ws_col == 0)
        return 80;   // fallback
    return w.ws_col;
}
extern int errno;


void print_permissions(mode_t mode)
{
    char perms[11];

    /* File type */
    if (S_ISDIR(mode))       perms[0] = 'd';
    else if (S_ISLNK(mode))  perms[0] = 'l';
    else if (S_ISCHR(mode))  perms[0] = 'c';
    else if (S_ISBLK(mode))  perms[0] = 'b';
    else if (S_ISFIFO(mode)) perms[0] = 'p';
    else if (S_ISSOCK(mode)) perms[0] = 's';
    else                     perms[0] = '-';

    /* Owner */
    perms[1] = (mode & S_IRUSR) ? 'r' : '-';
    perms[2] = (mode & S_IWUSR) ? 'w' : '-';
    perms[3] = (mode & S_IXUSR) ? 'x' : '-';

    /* Group */
    perms[4] = (mode & S_IRGRP) ? 'r' : '-';
    perms[5] = (mode & S_IWGRP) ? 'w' : '-';
    perms[6] = (mode & S_IXGRP) ? 'x' : '-';

    /* Others */
    perms[7] = (mode & S_IROTH) ? 'r' : '-';
    perms[8] = (mode & S_IWOTH) ? 'w' : '-';
    perms[9] = (mode & S_IXOTH) ? 'x' : '-';

    perms[10] = '\0';

    printf("%s ", perms);
}


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

    /* 3. Owner name */
    struct passwd *pw = getpwuid(st.st_uid);
    printf("%-8s ", pw ? pw->pw_name : "unknown");

    /* 4. Group name */
    struct group *gr = getgrgid(st.st_gid);
    printf("%-8s ", gr ? gr->gr_name : "unknown");

    /* 5. Size */
    printf("%8ld ", (long)st.st_size);

    /* 6. Modification time (formatted like ls) */
    char timebuf[64];
    struct tm *tm_info = localtime(&st.st_mtime);
    strftime(timebuf, sizeof(timebuf), "%b %e %H:%M", tm_info);
    printf("%s ", timebuf);

    /* 7. Filename */
    printf("%s\n", name);
}
void print_columns(char **names, int count, int maxlen)
{
    int term_width = get_terminal_width();
    int col_width  = maxlen + 2;               // 2 spaces gap
    int cols       = term_width / col_width;
    if (cols < 1) cols = 1;
    int rows       = (count + cols - 1) / cols;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int idx = c * rows + r;            // <-- down then across
            if (idx >= count) continue;

            if (c == cols - 1 || idx + rows >= count)
                printf("%s", names[idx]);      // last in row: no padding
            else
                printf("%-*s", col_width, names[idx]);
        }
        printf("\n");
    }
}

void do_ls(const char *dir);


void print_columns(char **names, int count, int maxlen);
void print_long(const char *dir, const char *name);
int main(int argc, char const *argv[])
{
    int opt;
    while ((opt = getopt(argc, (char * const *)argv, "l")) != -1) {
        switch (opt) {
            case 'l':
                long_listing = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l] [dir...]\n", argv[0]);
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
#define MAX_FILES 4096
#define MAX_NAME  256

void do_ls(const char *dir)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL) {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    /* Collect filenames */
    char *names[MAX_FILES];
    int   count = 0;
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
        int len = strlen(names[count]);
        if (len > maxlen) maxlen = len;
        count++;

        if (count >= MAX_FILES) break;
    }

    if (errno != 0)
        perror("readdir failed");

    closedir(dp);

    /* Now display */
    if (long_listing) {
        for (int i = 0; i < count; i++)
            print_long(dir, names[i]);
    } else {
        print_columns(names, count, maxlen);
    }

    /* Free memory */
    for (int i = 0; i < count; i++)
        free(names[i]);
}
