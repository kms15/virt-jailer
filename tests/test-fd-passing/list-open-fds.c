#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/magic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/vfs.h>


int main(int argc, char* argv[]) {
    // get the maximum allowed file descriptor value
    struct rlimit rlimit_nofile;

    if (getrlimit(RLIMIT_NOFILE, &rlimit_nofile) != 0) {
        perror("Error getting the maximum file descriptor value");
        exit(1);
    }

    printf("%-5s  %-16s  %-10s  %-10s  %-10s  %s\n",
            "fd#", "type", "on-device", "filesystem", "inode", "flags");

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
        struct statx fd_statx;
        if (statx(fd, "", AT_EMPTY_PATH, STATX_TYPE | STATX_INO | STATX_MNT_ID, &fd_statx) == -1) {
            fprintf(stderr,
                "Error getting file stats for fd %d\n",
                fd);
            exit(1);
        }

        // get additional information about the mount device
        struct statfs fd_statfs;
        if (fstatfs(fd, &fd_statfs) == -1) {
            fprintf(stderr,
                "Error getting mount filesystem stats for fd %d\n",
                fd);
            exit(1);
        }


        // translate the file type into a human-readable form
        const char* fd_type;
        char fd_type_scratch[32];
        switch (fd_statx.stx_mode & S_IFMT) {
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
                snprintf(fd_type_scratch,
                    sizeof(fd_type_scratch)/sizeof(fd_type_scratch[0]),
                    "%s[%d,%d]",
                    "blockdev",
                    fd_statx.stx_rdev_major,
                    fd_statx.stx_rdev_minor
                    );
                fd_type = fd_type_scratch;
                break;

            case S_IFDIR:
                fd_type="dir";
                break;

            case S_IFCHR:
                snprintf(fd_type_scratch,
                    sizeof(fd_type_scratch)/sizeof(fd_type_scratch[0]),
                    "%s[%d,%d]",
                    "chardev",
                    fd_statx.stx_rdev_major,
                    fd_statx.stx_rdev_minor
                    );
                fd_type = fd_type_scratch;
                break;

            case S_IFIFO:
                fd_type="fifo";
                break;

            default:
                assert(0); // unreached
                fd_type="unknown";
                break;
        }

        // translate the device id into the standard [major,minor] format
        char device_id[32];
        snprintf(device_id,
            sizeof(device_id)/sizeof(device_id[0]),
            "[%d,%d]",
            fd_statx.stx_dev_major,
            fd_statx.stx_dev_minor
            );

        // translate the mount filesystem into a human readable string for the
        // common cases.
        const char* mountfs;
        char mountfs_scratch[32];
        switch (fd_statfs.f_type) {
            case BTRFS_SUPER_MAGIC:
                mountfs = "btrfs";
                break;

            case DEVPTS_SUPER_MAGIC:
                mountfs = "devpts";
                break;

            case EXT2_SUPER_MAGIC:
                mountfs = "ext2+";
                break;

            case FUSE_SUPER_MAGIC:
                mountfs = "ext2+";
                break;

            case PIPEFS_MAGIC:
                mountfs = "pipefs";
                break;

            case XFS_SUPER_MAGIC:
                mountfs = "xfs";
                break;

            default:
                // Rather than try to map all of the supported file system
                // types in the linux kernel to human-readable nmaes, we just
                // default to showing the magic number for the less common
                // ones.
                snprintf(mountfs_scratch,
                    sizeof(mountfs_scratch)/sizeof(mountfs_scratch[0]),
                    "%#x", (unsigned)fd_statfs.f_type);
                mountfs = mountfs_scratch;
                break;
        }

        // decode as many of the fd_flags as we can and print any remaining
        // flags.
        // For safety, we just use snprintf to build up the string, swapping
        // between two buffers for the string we are building. This is O(n^2),
        // but n is small.
        const int FDFLAGS_BUF_SIZE=256;
        char flags_buf1[FDFLAGS_BUF_SIZE];
        char flags_buf2[FDFLAGS_BUF_SIZE];
        char* flags_str=flags_buf1;
        char* next=flags_buf2;
        char* swap;

        int flags = fd_flags;
        if (O_PATH && (flags & O_PATH) == O_PATH) {
            snprintf(flags_str, FDFLAGS_BUF_SIZE, "%s", "O_PATH");
            flags &= ~O_PATH;
        } else {
            switch (flags & O_ACCMODE) {
                case O_RDONLY:
                    snprintf(flags_str, FDFLAGS_BUF_SIZE, "%s", "O_RDONLY");
                    flags &= ~O_RDONLY;
                    break;

                case O_WRONLY:
                    snprintf(flags_str, FDFLAGS_BUF_SIZE, "%s", "O_WRONLY");
                    flags &= ~O_WRONLY;
                    break;

                case O_RDWR:
                    snprintf(flags_str, FDFLAGS_BUF_SIZE, "%s", "O_RDWR");
                    flags &= ~O_RDWR;
                    break;

                case O_ACCMODE:
                    snprintf(flags_str, FDFLAGS_BUF_SIZE, "%s", "O_ACCMODE");
                    flags &= ~O_ACCMODE;
                    break;

                default:
                    assert(0); // unreached
                    snprintf(flags_str, FDFLAGS_BUF_SIZE, "%s", "0");
                    break;
            }

            if (O_APPEND && (flags & O_APPEND) == O_APPEND) {
                snprintf(next, FDFLAGS_BUF_SIZE, "%s%s", flags_str, "|O_APPEND");
                swap = flags_str; flags_str = next; next = swap;
                flags &= ~O_APPEND;
            }
            if (O_ASYNC && (flags & O_ASYNC) == O_ASYNC) {
                snprintf(next, FDFLAGS_BUF_SIZE, "%s%s", flags_str, "|O_ASYNC");
                swap = flags_str; flags_str = next; next = swap;
                flags &= ~O_ASYNC;
            }
            if (O_DIRECT && (flags & O_DIRECT) == O_DIRECT) {
                snprintf(next, FDFLAGS_BUF_SIZE, "%s%s", flags_str, "|O_DIRECT");
                swap = flags_str; flags_str = next; next = swap;
                flags &= ~O_DIRECT;
            }
            if (O_LARGEFILE && (flags & O_LARGEFILE) == O_LARGEFILE) {
                snprintf(next, FDFLAGS_BUF_SIZE, "%s%s", flags_str, "|O_LARGEFILE");
                swap = flags_str; flags_str = next; next = swap;
                flags &= ~O_LARGEFILE;
            }
            if (O_NOATIME && (flags & O_NOATIME) == O_NOATIME) {
                snprintf(next, FDFLAGS_BUF_SIZE, "%s%s", flags_str, "|O_NOATIME");
                swap = flags_str; flags_str = next; next = swap;
                flags &= ~O_NOATIME;
            }
            if (O_NONBLOCK && (flags & O_NONBLOCK) == O_NONBLOCK) {
                snprintf(next, FDFLAGS_BUF_SIZE, "%s%s", flags_str, "|O_NONBLOCK");
                swap = flags_str; flags_str = next; next = swap;
                flags &= ~O_NONBLOCK;
            }

            // composite flag containing D_SYNC on x86_64
            if (O_SYNC && (flags & O_SYNC) == O_SYNC) {
                snprintf(next, FDFLAGS_BUF_SIZE, "%s%s", flags_str, "|O_SYNC");
                swap = flags_str; flags_str = next; next = swap;
                flags &= ~O_SYNC;
            }
            if (O_DSYNC && (flags & O_DSYNC) == O_DSYNC) {
                snprintf(next, FDFLAGS_BUF_SIZE, "%s%s", flags_str, "|O_DSYNC");
                swap = flags_str; flags_str = next; next = swap;
                flags &= ~O_DSYNC;
            }

            // composite flag containing O_DIRECTORY on x86_64
            if (O_TMPFILE && (flags & O_TMPFILE) == O_TMPFILE) {
                snprintf(next, FDFLAGS_BUF_SIZE, "%s%s", flags_str, "|O_TMPFILE");
                swap = flags_str; flags_str = next; next = swap;
                flags &= ~O_TMPFILE;
            }
            if (O_DIRECTORY && (flags & O_DIRECTORY) == O_DIRECTORY) {
                snprintf(next, FDFLAGS_BUF_SIZE, "%s%s", flags_str, "|O_DIRECTORY");
                swap = flags_str; flags_str = next; next = swap;
                flags &= ~O_DIRECTORY;
            }
        }
        if (flags) {
            snprintf(next, FDFLAGS_BUF_SIZE, "%s|%#x", flags_str, flags);
            swap = flags_str; flags_str = next; next = swap;
        }
        // add "..." if potentially truncated
        if (strlen(flags_str) == FDFLAGS_BUF_SIZE - 1) {
            flags_str[FDFLAGS_BUF_SIZE - 3] = '.';
            flags_str[FDFLAGS_BUF_SIZE - 2] = '.';
            flags_str[FDFLAGS_BUF_SIZE - 1] = '.';
        }

        printf("%-5d  %-16s  %-10s  %-10s  %-10llu  %-12s\n",
                fd,
                fd_type,
                device_id,
                mountfs,
                fd_statx.stx_ino,
                flags_str
                );
    }
}
