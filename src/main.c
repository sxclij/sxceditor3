#include <stdint.h>
#include <stdio.h>
#include <sys/resource.h>
#include <termios.h>
#include <unistd.h>

#define RLIMIT_SIZE (1024 * 1024 * 128)
#define BUFFER_SIZE (1024 * 1024 * 1)

enum bool {
    FALSE,
    TRUE,
};
enum result {
    RESULT_OK,
    RESULT_ERR,
};

struct constvec {
    const char* data;
    int32_t size;
};
struct vec {
    char* data;
    int32_t size;
};
struct node {
    struct vec data;
    struct node* prev;
    struct node* next;
};

enum result limit_init() {
    const rlim_t kstacksize = RLIMIT_SIZE;
    struct rlimit rl = {.rlim_cur = kstacksize, .rlim_max = kstacksize};
    setrlimit(RLIMIT_STACK, &rl);
    return RESULT_OK;
}
enum result term_update(struct vec* dst) {
    dst->size = read(STDIN_FILENO, dst->data, BUFFER_SIZE);
    return RESULT_OK;
}
enum result term_deinit(struct termios* orig_term) {
    tcsetattr(STDIN_FILENO, TCSANOW, orig_term);
    return RESULT_OK;
}
enum result term_init(struct termios* orig_term) {
    tcgetattr(STDIN_FILENO, orig_term);
    struct termios new_termios = *orig_term;
    new_termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    new_termios.c_oflag &= ~(OPOST);
    new_termios.c_cflag |= (CS8);
    new_termios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    new_termios.c_cc[VMIN] = 0;
    new_termios.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
    return RESULT_OK;
}
enum result input_update(struct vec* src, enum bool* isescape) {
    for (int32_t i = 0; i < src->size; i++) {
        if (src->data[i] == 'q') {
            *isescape = TRUE;
            return RESULT_OK;
        }
    }
    return RESULT_OK;
}

enum result main2(struct termios* orig_term) {
    char term_data[BUFFER_SIZE];
    struct vec term_vec = {.data = term_data, .size = 0};
    enum bool isescape = FALSE;
    while (isescape == FALSE) {
        if (term_update(&term_vec) == RESULT_ERR) {
            printf("err: _update\n");
            return RESULT_ERR;
        }
        if (input_update(&term_vec, &isescape) == RESULT_ERR) {
            printf("err: _update\n");
            return RESULT_ERR;
        }
    }
    return RESULT_OK;
}

int main() {
    struct termios orig_term;
    if (limit_init() == RESULT_ERR) {
        printf("err: limit_init\n\n");
        return 0;
    }
    if (term_init(&orig_term) == RESULT_ERR) {
        printf("err: term_init\n\n");
        return 0;
    }
    if (main2(&orig_term) == RESULT_ERR) {
        printf("err: main2\n\n");
    }
    if (term_deinit(&orig_term) == RESULT_ERR) {
        printf("err: term_deinit\n\n");
    }
    return 0;
}
