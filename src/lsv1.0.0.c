/*
* Programming Assignment 02: lsv1.0.0
* This is the source file of version 1.0.0
* Read the write-up of the assignment to add the features to this base version
* Usage:
*       $ lsv1.0.0 
*       % lsv1.0.0  /home
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

void do_ls(const char *dir);

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
void do_ls(const char *dir)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL) {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }
    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        if (long_listing)
            print_long(dir, entry->d_name);
        else
            printf("%s\n", entry->d_name);
    }

    if (errno != 0)
        perror("readdir failed");

    closedir(dp);
}
