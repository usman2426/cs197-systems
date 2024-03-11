#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "satsolver.h"


Bignum createBignum() {
    Bignum num = {NULL, 0, 0};
    return num;
}


int print_binary(Bignum *num) {
    unsigned saw_first_one = 0;
    for (int i = num->n_words; i --> 0;) {
        for (int j = 64; j --> 0;) {
            if (num->words[i] >> j || saw_first_one) {
                printf("%lu", (num->words[i] >> j) & 1);
                saw_first_one = 1;
            }
        }
    }
    if (!saw_first_one) printf("0");
    printf("\n");
}

int print_hex(Bignum *num) {
    for (int i = num->n_words; i --> 0;) {
        printf("%lX", num->words[i]);
    }
    printf("\n");
}

int isLessThanPower(unsigned power, Bignum *num) {
    if (num->sign) return 1; // negative always less than a power
    unsigned word_idx = power / 64;
    uint64_t bit = ((uint64_t)1 << (uint64_t)(power % 64));
    if (word_idx >= num->n_words) return 1; // num can't be greater because it has less bits
    // num can't be less because contains at a positive bit at least at index corresponding to power
    if (num->words[word_idx] >= bit) return 0; 
    // less than bit at its most significant word, so check more words for a bit to see if greater
    for (uint64_t i = word_idx + 1; i < num->n_words; i++) {
        if (num->words[i] > 0) return 0;  // finds a positive bit that is greater than power
    }
    // no bits greater than power, so num must be less than power
    return 1;
}

void add(unsigned power, Bignum *num) {
      unsigned word_idx = power / 64;
      uint64_t bit = ((uint64_t)1 << (uint64_t)(power % 64));
      if (num->sign) cascade_down(word_idx, bit, num);
      else cascade_up(word_idx, bit, num);
}

void sub(unsigned power, Bignum *num) {
    unsigned word_idx = power / 64;
    uint64_t bit = ((uint64_t)1 << (uint64_t)(power % 64));
    if (num->sign) cascade_up(word_idx, bit, num);
    else cascade_down(word_idx, bit, num);
}

void cascade_up(unsigned word_idx, uint64_t bit, Bignum *num) {
    if ((word_idx + 1) > num->n_words) {  // possibly going to need a bunch more words
        uint64_t old_n_words = num->n_words;
        num->n_words = 2 * (word_idx + 1);
        num->words = realloc(num->words, num->n_words * sizeof(num->words[0]));
        memset(num->words + old_n_words, 0, (num->n_words - old_n_words) * sizeof(num->words[0]));
    } else if (num->n_words && num->words[num->n_words - 1]) { // will need 1 more word to handle overflow
        num->words = realloc(num->words, (++num->n_words) * sizeof(num->words[0]));
        num->words[num->n_words - 1] = 0;
    }
    // https://stackoverflow.com/questions/46701364/how-to-access-the-carry-flag-while-adding-two-64-bit-numbers-using-asm-in-c
    num->words[word_idx] = bit + num->words[word_idx];
    // GCC does seem to do a fine job of optimizing this into a jb:
    // https://godbolt.org/z/6Tja8xfs6
    unsigned carry = num->words[word_idx] < bit;
    while (carry)
        carry = !(++(num->words[++word_idx]));
}

void cascade_down(unsigned word_idx, uint64_t bit, Bignum *num) {
    if ((word_idx + 1) > num->n_words) {  // possibly going to need a bunch more words
        uint64_t old_n_words = num->n_words;
        num->n_words = 2 * (word_idx + 1);
        num->words = realloc(num->words, num->n_words * sizeof(num->words[0]));
        memset(num->words + old_n_words, 0, (num->n_words - old_n_words) * sizeof(num->words[0]));
        num->words[word_idx] = bit - 1;
        for (unsigned i = 0; i < word_idx; i++) num->words[i] ^= -1;
        cascade_up(0, 1, num); // adds 1
        if (num->sign) num->sign = 0;
        else num->sign = 1;
    } else {
        unsigned old_val = num->words[word_idx];
        num->words[word_idx] -= bit;
        unsigned carry = !(num->words[word_idx] < old_val);  // if > after sub, then overflow happened
        while (carry) {
            word_idx++;
            if (word_idx == num->n_words) {
                for (unsigned i = 0; i < num->n_words; i++) {
                    num->words[i] ^= -1;
                }
                cascade_up(0, 1, num);
                if (num->sign) num->sign = 0;
                else num->sign = 1;
                break;
            }
            old_val = num->words[word_idx];
            num->words[word_idx] -= 1;
            carry = !(num->words[word_idx] < old_val);
        }
    }
}
  