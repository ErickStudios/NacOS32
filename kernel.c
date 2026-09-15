#define WHITE_TXT 0x07
#include "types.h"
#include "port.h"

// Informacion del framebuffer
extern u16* 	lfb_address;
extern u32 		lfb_resh;
extern u32 		lfb_resw;

extern u8 TryParseRt(char* code, u32* size, u8* jit);

void ata_read_sector(u32 lba, u8 *target_buffer) {
    while (inb(0x1F7) & 0x80);

    outb(0x1F2, 1);
    outb(0x1F3, (u8) (lba & 0xFF));
    outb(0x1F4, (u8) ((lba >> 8) & 0xFF));
    outb(0x1F5, (u8) ((lba >> 16) & 0xFF));
    
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));

    outb(0x1F7, 0x20);

    while (1) {
        u8 status = inb(0x1F7);
        if ((status & 0x08) != 0) break;
        if ((status & 0x01) != 0) {
            return;
        }
    }

    insw(0x1F0, target_buffer, 256);
}

#define VGA_ADDRESS 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

u16 *terminal_buffer = (u16 *) VGA_ADDRESS;
u8 terminal_color = 0x07;
u32 terminal_row = 0;
u32 terminal_column = 0;

void tty_scroll() {
    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            terminal_buffer[(y - 1) * VGA_WIDTH + x] = terminal_buffer[y * VGA_WIDTH + x];
        }
    }

    for (int x = 0; x < VGA_WIDTH; x++) {
        terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = ((u16)terminal_color << 8) | ' ';
    }
}

void tty_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        terminal_row++;
    } else if (c == '\b') {
        terminal_column --;
        tty_putchar(' ');
        terminal_column --;
    } else {
        const u32 index = terminal_row * VGA_WIDTH + terminal_column;
        terminal_buffer[index] = ((u16)terminal_color << 8) | c;
        terminal_column++;
        
        if (terminal_column >= VGA_WIDTH) {
            terminal_column = 0;
            terminal_row++;
        }
    }

    if (terminal_row >= VGA_HEIGHT) {
        tty_scroll();
        terminal_row = VGA_HEIGHT - 1;
    }
}

void tty_print(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        tty_putchar(str[i]);
    }
}

u8 keyboard_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t','q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 
    0,  '\\','z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0,  ' '
};

u8 keyboard_map_shifted[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t','Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 
    0,  '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0,  ' '
};

static u8 shift_pressed = 0;

void __stack_chk_fail_local(void) {
    while (1) {
        __asm__("cli");
        __asm__("hlt");
    }
}

void __stack_chk_fail(void) {
    __stack_chk_fail_local();
}

char keyboard_read_char() {
    while (1) {
        if (inb(0x64) & 0x01) {
            u8 scancode = inb(0x60);

            if (scancode == 0x2A || scancode == 0x36) {
                shift_pressed = 1;
                continue;
            }
            if (scancode == (0x2A | 0x80) || scancode == (0x36 | 0x80)) {
                shift_pressed = 0;
                continue;
            }

            if (scancode & 0x80) {
                continue;
            }

            if (scancode == 0x39) {
                return ' ';
            }
            
            if (scancode < 128) {
                char ascii = shift_pressed ? keyboard_map_shifted[scancode] : keyboard_map[scancode];
                if (ascii != 0) {
                    return ascii;
                }
            }
        }
    }
}

void tty_print_dec(int n) {
    if (n == 0) {
        tty_putchar('0');
        return;
    }
    if (n < 0) {
        tty_putchar('-');
        n = -n;
    }
    char buf[12];
    int i = 0;
    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    while (i > 0) {
        tty_putchar(buf[--i]);
    }
}

void k_main()
{
	u8 buf[512];
	ata_read_sector(0, buf);
	u16* buf2 = (u16*)buf;
    
    while (1)
    {
        // Wait for the Buffer Reading
        char ky;
        char jitc[64];
        int jiti = 0;
        terminal_color = 0xA;
        tty_print("@ ");
        terminal_color = 7;
        do {
            ky = keyboard_read_char();

            if (ky != '\t')
                tty_putchar(ky);
            if (ky == '\b') {
                jitc[jiti] = 0;
                jiti--;
            }
            else if (ky == '\t') {
                jitc[jiti] = '\n';
                jiti++;
                tty_putchar('\n');
            }
            else {
                jitc[jiti] = ky;
                jiti++;
            }
        } while (ky != '\n');
        jitc[jiti++] = ' ';
        jitc[jiti++] = 'r';
        jitc[jiti++] = 'e';
        jitc[jiti++] = 't';
        jitc[jiti++] = 0;

        // Compile the code
        char* jitca = jitc;
        char jit[200];
        char* jat = jit;
        int sjit = 0;
        TryParseRt(jitca, &sjit, jat);
        typedef int (*Functor)();
        Functor fn = (Functor)jat;
        int r = fn();

        if (sjit != 1) {
            terminal_color = 0x9;
            tty_print("ok ");
            terminal_color = 0xB;
            tty_print_dec(r);
            terminal_color = 7;
            tty_putchar('\n');
        }
    }

	while(1);
};
