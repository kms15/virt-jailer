#include <unistd.h>

int main() {
    if (getpid() == 1) {
        return 0;
    } else {
        return 1;
    }
}
