#include "types.h"

static inline void outb(u16 port, u8 val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline u8 inb(u16 port) {
    u8 ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void insw(u16 port, void *addr, u32 word_count) {
    __asm__ volatile ("rep insw" : "+D"(addr), "+c"(word_count) : "d"(port) : "memory");
}