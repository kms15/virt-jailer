#define _GNU_SOURCE
#include <errno.h>
#include <linux/nsfs.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/pidfd.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysmacros.h>
#include <string.h>
#include <unistd.h>


int main(int argc, char* argv[], char* envp[]) {

    // Get the file descriptor for the current process

    pid_t my_pid = getpid();
    int my_pid_fd = syscall(SYS_pidfd_open, my_pid, 0);

    if (my_pid_fd == -1) {
        perror("Failed to open file descriptor for this processes PID");
        exit(1);
    }

    // loop over each of the namespaces for this process

    const struct {
        int ioctl_fetch_op;
        const char* name;
    } namespaces[] = {
        { PIDFD_GET_USER_NAMESPACE, "user" },
        { PIDFD_GET_CGROUP_NAMESPACE, "cgroup" },
        { PIDFD_GET_IPC_NAMESPACE, "IPC" },
        { PIDFD_GET_MNT_NAMESPACE, "mount" },
        { PIDFD_GET_NET_NAMESPACE, "net" },
        { PIDFD_GET_PID_NAMESPACE, "PID" },
        { PIDFD_GET_TIME_NAMESPACE, "time" },
        { PIDFD_GET_UTS_NAMESPACE, "UTS" },
    };
    const int num_namespaces = sizeof(namespaces) / sizeof(namespaces[0]);

    // print the header

    printf("%-10s  %-9s  %-10s  %-11s  %-13s\n", "namespace", "device id", "inode", "user ns dev", "user ns inode");

    for (int i=0; i < num_namespaces; i++) {

        // get the file descriptor for the namespace

        int namespace_fd = ioctl(my_pid_fd, namespaces[i].ioctl_fetch_op, 0);
        if (namespace_fd == -1) {
            fprintf(stderr,
                "Error getting file descriptor for namespace %s: %s\n",
                namespaces[i].name, strerror(errno));
            exit(1);
        }

        // get the file status for the namespace

        struct stat namespace_st;
        if (fstat(namespace_fd, &namespace_st) == -1) {
            fprintf(stderr,
                "Error getting file status for namespace %s: %s\n",
                namespaces[i].name, strerror(errno));
            exit(1);
        }

        // print the results so far for the namespace
        printf( "%-10s  %-9ju  %-10ju",
                namespaces[i].name,
                namespace_st.st_dev,
                (uintmax_t) namespace_st.st_ino);

        // If this is not a user namespace, get the owning user namespace
        if (namespaces[i].ioctl_fetch_op != PIDFD_GET_USER_NAMESPACE) {
            // get the file descriptor for the owning user namespace

            int owner_namespace_fd = ioctl(namespace_fd, NS_GET_USERNS);
            if (owner_namespace_fd == -1) {
                if (errno == EPERM) {
                    printf("  %-11s  %-13s\n", "DENIED", "DENIED");
                    continue;
                }

                printf("\n");
                fprintf(stderr,
                    "Error getting file descriptor for the user namespace that "
                        "owns namespace %s: %s\n",
                    namespaces[i].name, strerror(errno));
                exit(1);
            }

            // get the file status for the owning user namespace

            struct stat owner_namespace_st;
            if (fstat(owner_namespace_fd, &owner_namespace_st) == -1) {
                printf("\n");
                fprintf(stderr,
                    "Error getting file status for the user namespace that "
                        "owns namespace %s: %s\n",
                    namespaces[i].name, strerror(errno));
                exit(1);
            }

            printf("  %-11ju  %-13ju\n",
                   owner_namespace_st.st_dev,
                   (uintmax_t) owner_namespace_st.st_ino
                  );
        } else {
            printf( "  %-11jx  %-13ju\n",
                    namespace_st.st_dev,
                    (uintmax_t) namespace_st.st_ino);
        }
    }
}
