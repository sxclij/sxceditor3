#include <unistd.h>

int main() {
    write(STDOUT_FILENO, "Good morning world!\n", 21);
    return 0;
}