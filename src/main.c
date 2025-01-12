#include <stdint.h>
#include <stdio.h>
#include <sys/resource.h>
#include <termios.h>
#include <unistd.h>

#define RLIMIT_SIZE (1024 * 1024 * 16)
#define BUF_SIZE (1024 * 1024 * 2)

enum result_type {
    result_type_ok,
    result_type_err,
};

struct result {
    enum result_type type;
};
struct string {
    char* data;
    int64_t size;
};

static struct termios term_original;

struct result rlimit_init() {
    const rlim_t kstacksize = RLIMIT_SIZE;
    struct rlimit rl;
    int result;
    result = getrlimit(RLIMIT_STACK, &rl);
    if (result != 0) {
        perror("getrlimit");
        return (struct result){.type = result_type_err};
    }
    if (rl.rlim_cur >= kstacksize) {
        return (struct result){.type = result_type_ok};
    }
    rl.rlim_cur = kstacksize;
    result = setrlimit(RLIMIT_STACK, &rl);
    if (result != 0) {
        perror("setrlimit");
        return (struct result){.type = result_type_err};
    }
    return (struct result){.type = result_type_ok};
}

struct result term_update(struct string* dst) {
    dst->size = read(STDIN_FILENO, dst->data, BUF_SIZE);
    return (struct result){.type = result_type_ok};
}
struct result term_deinit() {
    if (tcsetattr(STDIN_FILENO, TCSANOW, &term_original) == -1) {
        perror("tcsetattr");
        return (struct result){.type = result_type_err};
    }
    return (struct result){.type = result_type_ok};
}
struct result term_init() {
    if (tcgetattr(STDIN_FILENO, &term_original) == -1) {
        perror("tcgetattr");
        return (struct result){.type = result_type_err};
    }
    struct termios term_new = term_original;
    term_new.c_lflag &= ~(ICANON | ECHO);
    term_new.c_cc[VMIN] = 0;
    term_new.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &term_new) == -1) {
        perror("tcsetattr");
        return (struct result){.type = result_type_err};
    }
    return (struct result){.type = result_type_ok};
}

struct result input_update(const struct string src) {
    for (int64_t i = 0; i < src.size; i++) {
        if (src.data[i] == 'q') {
            return (struct result){.type = result_type_err};
        }
    }
    return (struct result){.type = result_type_ok};
}

void loop() {
    char buf_data[BUF_SIZE];
    struct string buf = (struct string){.data = buf_data, .size = 0};
    while (1) {
        if (term_update(&buf).type == result_type_err) {
            perror("term_update");
            return;
        };
        if (input_update(buf).type == result_type_err) {
            perror("input_update");
            return;
        }
    }
}
struct result deinit() {
    if (term_deinit().type == result_type_err) {
        perror("term_deinit");
        return (struct result){.type = result_type_err};
    }
    return (struct result){.type = result_type_ok};
}
struct result init() {
    if (rlimit_init().type == result_type_err) {
        perror("rlimit_init");
        return (struct result){.type = result_type_err};
    }
    if (term_init().type == result_type_err) {
        perror("term_init");
        return (struct result){.type = result_type_err};
    }
    return (struct result){.type = result_type_ok};
}

int main() {
    if (init().type == result_type_err) {
        perror("init");
        return -1;
    }
    loop();
    if (deinit().type == result_type_err) {
        perror("deinit");
        return -1;
    }
    return 0;
}