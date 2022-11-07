#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

uint64_t *WORDS = NULL;
uint64_t N_WORDS = 0;

int add_pow_2(unsigned power) {
    unsigned word_idx = power / 64;
    if ((word_idx + 1) > N_WORDS) {
        N_WORDS = 2 * (word_idx + 1);
        WORDS = realloc(WORDS, N_WORDS * sizeof(WORDS[0]));
    } else if (N_WORDS && WORDS[N_WORDS - 1]) {
        N_WORDS++;
        WORDS = realloc(WORDS, N_WORDS * sizeof(WORDS[0]));
    }

    // https://stackoverflow.com/questions/46701364/how-to-access-the-carry-flag-while-adding-two-64-bit-numbers-using-asm-in-c
    uint64_t bit = ((uint64_t)1 << (uint64_t)(power % 64));
    WORDS[word_idx] = bit + WORDS[word_idx];
    // GCC does seem to do a fine job of optimizing this into a jb:
    // https://godbolt.org/z/6Tja8xfs6
    unsigned carry = (WORDS[word_idx] < bit);
    while (carry)
        carry = !(++WORDS[++word_idx]);
}

int print_binary() {
    unsigned saw_first_one = 0;
    for (int i = N_WORDS; i --> 0;) {
        for (int j = 64; j --> 0;) {
            if (WORDS[i] >> j || saw_first_one) {
                printf("%lu", (WORDS[i] >> j) & 1);
                saw_first_one = 1;
            }
        }
    }
    printf("\n");
}

int main() {
    for (int i = 0; i < 126; i++)
        add_pow_2(i);
    print_binary();

    add_pow_2(0);
    print_binary();

    add_pow_2(127);
    print_binary();

    add_pow_2(126);
    print_binary();
    return 0;
}
