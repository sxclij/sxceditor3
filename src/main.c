#include <stdint.h>
#include <unistd.h>

#define mem_size (1 << 16)

enum mem_global {
    mem_global_zero,
    mem_global_ip,
    mem_global_sp,
    mem_global_bp,
};

struct global {
    uint64_t mem[mem_size];
};

static struct global global;

int main() {
    return 0;
}