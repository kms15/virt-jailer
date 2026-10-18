#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

int main(int argc, char* argv[]) {
    // get the maximum allowed file descriptor value
    struct rlimit rlimit_nofile;

    if (getrlimit(RLIMIT_NOFILE, &rlimit_nofile) != 0) {
        perror("Error getting the maximum file descriptor value");
        exit(1);
    }

    printf("%-5s  %-8s  %-10s  %-10s  %-10s  %s\n", "fd#", "type", "device id",
        "on device", "inode", "flags");

    // loop over each potential file descriptor
    for (int fd = 0; fd < rlimit_nofile.rlim_max; ++fd) {
        int fd_flags = fcntl(fd, F_GETFL);

        // get the flags for this file descriptor (or skip it if it's missing)
        if (fd_flags < 0) {
            if (errno == EBADF) {
                // no file descriptor with this number
                continue;
            }

            fprintf(stderr,
                "Error getting file descriptor flags for fd %d\n",
                fd);
            exit(1);
        }

        // get additional information about the file or device referenced by
        // the file descriptor
        struct stat fd_stat;
        if (fstat(fd, &fd_stat) == -1) {
            fprintf(stderr,
                "Error getting file status for fd %d\n",
                fd);
            exit(1);
        }

        // translate the file type into a human-readable form
        const char* fd_type;

        switch (fd_stat.st_mode & S_IFMT) {
            case S_IFSOCK:
                fd_type="socket";
                break;

            case S_IFLNK:
                fd_type="symlink";
                break;

            case S_IFREG:
                fd_type="file";
                break;

            case S_IFBLK:
                fd_type="blockdev";
                break;

            case S_IFDIR:
                fd_type="dir";
                break;

            case S_IFCHR:
                fd_type="chardev";
                break;

            case S_IFIFO:
                fd_type="fifo";
                break;

            default:
                assert(0); // unreached
                fd_type="unknown";
                break;
        }

        printf("%-5d  %-8s  %-5u%-5u  %-5u%-5u  %-10ju  ",
                fd,
                fd_type,
                (unsigned)major(fd_stat.st_rdev),
                (unsigned)minor(fd_stat.st_rdev),
                (unsigned)major(fd_stat.st_dev),
                (unsigned)minor(fd_stat.st_dev),
                fd_stat.st_ino
                );

        // decode as many of the fd_flags as we can and print any remaining
        // flags.
        int flags = fd_flags;
        if (O_PATH && (flags & O_PATH) == O_PATH) {
            printf("O_PATH");
            flags &= ~O_PATH;
        } else {
            switch (flags & O_ACCMODE) {
                case O_RDONLY:
                    printf("O_RDONLY");
                    flags &= ~O_RDONLY;
                    break;

                case O_WRONLY:
                    printf("O_WRONLY");
                    flags &= ~O_WRONLY;
                    break;

                case O_RDWR:
                    printf("O_RDWR");
                    flags &= ~O_RDWR;
                    break;

                case O_ACCMODE:
                    printf("O_ACCMODE");
                    flags &= ~O_ACCMODE;
                    break;

                default:
                    assert(0); // unreached
                    printf("0");
                    break;
            }

            if (O_APPEND && (flags & O_APPEND) == O_APPEND) {
                printf("|O_APPEND");
                flags &= ~O_APPEND;
            }
            if (O_ASYNC && (flags & O_ASYNC) == O_ASYNC) {
                printf("|O_ASYNC");
                flags &= ~O_ASYNC;
            }
            if (O_DIRECT && (flags & O_DIRECT) == O_DIRECT) {
                printf("|O_DIRECT");
                flags &= ~O_DIRECT;
            }
            if (O_LARGEFILE && (flags & O_LARGEFILE) == O_LARGEFILE) {
                printf("|O_LARGEFILE");
                flags &= ~O_LARGEFILE;
            }
            if (O_NOATIME && (flags & O_NOATIME) == O_NOATIME) {
                printf("|O_NOATIME");
                flags &= ~O_NOATIME;
            }
            if (O_NONBLOCK && (flags & O_NONBLOCK) == O_NONBLOCK) {
                printf("|O_NONBLOCK");
                flags &= ~O_NONBLOCK;
            }

            // composite flag containing D_SYNC on x86_64
            if (O_SYNC && (flags & O_SYNC) == O_SYNC) {
                printf("|O_SYNC");
                flags &= ~O_SYNC;
            }
            if (O_DSYNC && (flags & O_DSYNC) == O_DSYNC) {
                printf("|O_DSYNC");
                flags &= ~O_DSYNC;
            }

            // composite flag containing O_DIRECTORY on x86_64
            if (O_TMPFILE && (flags & O_TMPFILE) == O_TMPFILE) {
                printf("|O_TMPFILE");
                flags &= ~O_TMPFILE;
            }
            if (O_DIRECTORY && (flags & O_DIRECTORY) == O_DIRECTORY) {
                printf("|O_DIRECTORY");
                flags &= ~O_DIRECTORY;
            }
        }
        if (flags) {
            printf("|%#x", flags);
        }
        printf("\n");
    }
}
