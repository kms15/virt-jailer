#include <unistd.h>

int main(int argc, char* argv[], char* envp[]) {
    // switch to the new process
    return execve(argv[2], argv + 2, envp);
}
