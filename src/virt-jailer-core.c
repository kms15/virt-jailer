#define _GNU_SOURCE
#include <assert.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char* argv[], char* envp[]) {
    // Unshare the user namespace (required to unshare many of the other
    // namespaces)
    if (unshare(CLONE_NEWUSER) == -1) {
        perror("Failed to unshare user namespace");
        exit(1);
    }

    // Unshare the PID namespaces
    if (unshare(CLONE_NEWPID) == -1) {
        perror("Failed to unshare PID namespace");
        exit(1);
    }

    // Unsharing the PID will not switch the current process into the new
    // namespace, so fork to create a new child process running in the new
    // PID namespace
    pid_t child_pid = fork();

    if (child_pid < 0) {
        assert(child_pid == -1);
        perror("Failed to fork into new PID namespace");
        exit(1);
    } else if (child_pid > 0) {
        // This is the parent process in the root PID namespace
        int wait_status = 0;

        // Wait for the child in the new PID namespace to exit
        if (waitpid(child_pid, &wait_status, 0) == -1) {
            perror("Parent failed to wait for child in new PID namespace");
            exit(1);
        }

        // If the child terminated normally, return the return value from the
        // child, otherwise return 1.
        if (WIFEXITED(wait_status)) {
            exit(WEXITSTATUS(wait_status));
        } else {
            exit(1);
        }
    }

    // switch to the new process
    if (execve(argv[2], argv + 2, envp) == -1) {
        perror("Failed to execve to target binary");
        exit(1);
    }

    assert(0); // unreached
}
