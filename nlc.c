/**
 * Naca Lang Compiler
 */
void tty_putchar(char c) ;
 #include "types.h"

#define MAX_FUNCTIONS 64
#define MAX_NAME_LEN  32

typedef struct {
    u8 name[MAX_NAME_LEN];
    u32 pc;
} FunctionEntry;

typedef struct {
    FunctionEntry entries[MAX_FUNCTIONS];
    u32 count;
} FunctionTable;

// Check if is space
static u8 IsSpace(char chr) {
    return (chr == ' ' || chr == '\t' || chr == '\n' || chr == '\r');
}

// Registers a function
static u8 RegisterFunction(FunctionTable* table, const char* name, u32 pc) {
    for (u32 i = 0; i < table->count; i++) {
        u32 j = 0;
        u8 match = 1;
        while (table->entries[i].name[j] != '\0' && name[j] != '\0') {
            if (table->entries[i].name[j] != name[j]) {
                match = 0;
                break;
            }
            j++;
        }
        if (match && table->entries[i].name[j] == name[j]) {
            table->entries[i].pc = pc;
            return 1; 
        }
    }

    if (table->count >= MAX_FUNCTIONS) return 0;
    
    u32 i = 0;
    while (name[i] != '\0' && i < MAX_NAME_LEN - 1) {
        table->entries[table->count].name[i] = name[i];
        i++;
    }
    table->entries[table->count].name[i] = '\0';
    table->entries[table->count].pc = pc;
    table->count++;
    
    return 1;
}

// Find a function
static u32 FindFunctionPC(FunctionTable* table, const char* name) {
    for (u32 i = 0; i < table->count; i++) {
        u32 j = 0;
        u8 match = 1;
        while (table->entries[i].name[j] != '\0' && name[j] != '\0') {
            if (table->entries[i].name[j] != name[j]) {
                match = 0;
                break;
            }
            j++;
        }
        if (match && table->entries[i].name[j] == name[j]) {
            return table->entries[i].pc;
        }
    }
    return 0xFFFFFFFF;
}

static u32 ParseU32(char* code, u32* it) {
    u32 val = 0;
    u8 base = 10;

    if (code[*it] == '0' && (code[*it + 1] == 'x' || code[*it + 1] == 'X')) {
        base = 16;
        *it += 2;
    }

    while (code[*it] != '\0') {
        char c = code[*it];
        u32 digit = 0;

        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (base == 16 && c >= 'a' && c <= 'f') {
            digit = c - 'a' + 10;
        } else if (base == 16 && c >= 'A' && c <= 'F') {
            digit = c - 'A' + 10;
        } else {
            break; // Fin del número
        }

        val = val * base + digit;
        (*it)++;
    }
    return val;
}

static int strcmp(char* a1, char* a2) {
    char* s1 = a1;
    char* s2 = a2;
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

u8 LetterToRegister(char letter) {
    switch (letter)
    {
    case 'a': return 0;
    case 'b': return 3;
    case 'c': return 1;
    case 'd': return 2;
    return 0;
    }
}

u8 TryParse(char* code, u32* size, u8* jit, FunctionTable* func_table) {
    *size = 0;
    u32 it = 0;
    u8 rd = 0;
    
    while (code[it] != '\0') {
        // Functions Declarations
        if (
            (code[it] == 'f' && code[it + 1] == 'n' || code[it] == 'd' && code[it + 1] == 'f') && 
            IsSpace(code[it + 2])
        ) {
            it += 2;
            
            while (IsSpace(code[it])) it++;
            
            char func_name[MAX_NAME_LEN];
            u32 name_idx = 0;
            while (code[it] != '\0' && !IsSpace(code[it]) && code[it] != '(') {
                if (name_idx < MAX_NAME_LEN - 1) {
                    func_name[name_idx++] = code[it];
                }
                it++;
            }
            func_name[name_idx] = '\0';
            
            RegisterFunction(func_table, func_name, *size);
        }
        // Mathematic Expretions
        else if (code[it] == '$') {

            if (rd != 0) {
                jit[*size] = 0x50; // push eax
                (*size)++;
            }

            it++;
            while (IsSpace(code[it])) it++;

            u8 bn = 4; // operant size in bytes

            // Byte Operations
            if (code[it] == 'b') {
                bn = 1;
                it ++;
            }
            // Word Operations
            else if (code[it] == 'w') {
                bn = 2;
                it ++;
            }
            // Explicit Dword Operations
            else if (code[it] == 'd') {
                it ++;
            }

            // Initialize It
            if (code[it] >= '0' && code[it] <= '9') {
                u32 first_val = ParseU32(code, &it);
                if (bn == 1) {
                    jit[*size] = 0xB0; // mov al, imm8
                    jit[*size + 1] = (u8)first_val;
                    *size += 2;
                } else if (bn == 2) {
                    jit[*size] = 0x66; // Operand-size override prefix
                    jit[*size + 1] = 0xB8; // mov ax, imm16
                    *(u16*)(jit + *size + 2) = (u16)first_val;
                    *size += 4;
                } else {
                    jit[*size] = 0xB8; // mov eax, imm32
                    *(u32*)(jit + *size + 1) = first_val;
                    *size += 5;
                }
            }

            while (code[it] != '\0' && code[it] != ';') {
                while (IsSpace(code[it])) it++;
                
                if (code[it] == ';') break;

                char op = code[it];
                it++;
                
                while (IsSpace(code[it])) it++;

                u8 is_self = 0;
                u32 next_val = 0;

                if (code[it] == '.') {
                    is_self = 1;
                    it++;
                } else {
                    if (op != '!') next_val = ParseU32(code, &it);
                }

                // --- ADD (+) ---
                if (op == '+') {
                    if (is_self) {
                        if (bn == 2) jit[*size++] = 0x66;
                        jit[*size]     = (bn == 1) ? 0x00 : 0x01; // add al, al / add eax, eax
                        jit[*size + 1] = 0xC0;
                        *size += 2;
                    } else {
                        if (bn == 1) {
                            jit[*size]     = 0x04; // add al, imm8
                            jit[*size + 1] = (u8)next_val;
                            *size += 2;
                        } else if (bn == 2) {
                            jit[*size]     = 0x66;
                            jit[*size + 1] = 0x05; // add ax, imm16
                            *(u16*)(jit + *size + 2) = (u16)next_val;
                            *size += 4;
                        } else {
                            jit[*size]     = 0x05; // add eax, imm32
                            *(u32*)(jit + *size + 1) = next_val;
                            *size += 5;
                        }
                    }
                } 
                // --- SUB (-) ---
                else if (op == '-') {
                    if (is_self) {
                        if (bn == 2) jit[*size++] = 0x66;
                        jit[*size]     = (bn == 1) ? 0x28 : 0x29; // sub al, al / sub eax, eax
                        jit[*size + 1] = 0xC0;
                        *size += 2;
                    } else {
                        if (bn == 1) {
                            jit[*size]     = 0x2C; // sub al, imm8
                            jit[*size + 1] = (u8)next_val;
                            *size += 2;
                        } else if (bn == 2) {
                            jit[*size]     = 0x66;
                            jit[*size + 1] = 0x2D; // sub ax, imm16
                            *(u16*)(jit + *size + 2) = (u16)next_val;
                            *size += 4;
                        } else {
                            jit[*size]     = 0x2D; // sub eax, imm32
                            *(u32*)(jit + *size + 1) = next_val;
                            *size += 5;
                        }
                    }
                } 
                // --- CMP (?) ---
                else if (op == '?') {
                    if (is_self) {
                        if (bn == 2) jit[*size++] = 0x66;
                        jit[*size]     = (bn == 1) ? 0x38 : 0x39; // cmp al, al / cmp eax, eax
                        jit[*size + 1] = 0xC0;
                        *size += 2;
                    } else {
                        if (bn == 1) {
                            jit[*size]     = 0x3C; // cmp al, imm8
                            jit[*size + 1] = (u8)next_val;
                            *size += 2;
                        } else if (bn == 2) {
                            jit[*size]     = 0x66;
                            jit[*size + 1] = 0x3D; // cmp ax, imm16
                            *(u16*)(jit + *size + 2) = (u16)next_val;
                            *size += 4;
                        } else {
                            jit[*size]     = 0x3D; // cmp eax, imm32
                            *(u32*)(jit + *size + 1) = next_val;
                            *size += 5;
                        }
                    }
                }
                // --- MULTIPLY (*) ---
                else if (op == '*') {
                    if (is_self) {
                        if (bn == 2) jit[*size++] = 0x66;
                        jit[*size]     = 0x0F;
                        jit[*size + 1] = 0xAF;
                        jit[*size + 2] = 0xC0;
                        *size += 3;
                    } else {
                        if (bn == 2) {
                            jit[*size++] = 0x66;
                            jit[*size]   = 0x69;
                            jit[*size + 1] = 0xC0;
                            *(u16*)(jit + *size + 2) = (u16)next_val;
                            *size += 4;
                        } else {
                            jit[*size]     = 0x69;
                            jit[*size + 1] = 0xC0;
                            *(u32*)(jit + *size + 2) = next_val;
                            *size += 6;
                        }
                    }
                }
                // --- FUNCTION CALL (!) ---
                else if (op == '!') {
                    jit[*size] = 0xE8;
                    *(u32*) (jit + *size + 1) = 0;
                    *size += 5;
                    jit[*size++] = 0x58;
                    jit[*size] = 0x5;
                    *(u32*) (jit + *size + 1) = 4;
                    *size += 5;
                    char id_name[MAX_NAME_LEN];
                    int uidx = 0;
                    while (code[it] != '\0' && (
                   (code[it] >= 'a' && code[it] <= 'z') || 
                   (code[it] >= 'A' && code[it] <= 'Z') || 
                   (code[it] >= '0' && code[it] <= '9') || 
                   code[it] == '_')) {
                        if (uidx < MAX_NAME_LEN - 1) { 
                            id_name[uidx++] = code[it];
                        } 
                        it++;
                    } 
                    id_name[uidx] = 0;
                    u32 target_pc = FindFunctionPC(func_table, id_name);
                    if (target_pc != 0xFFFFFFFF) {
                        jit[*size] = 0x05;
                        i32 rel_offset = (i32)target_pc - (i32)(*size + 5);
                        *(u32*) (jit + *size + 1) = rel_offset;
                        *size += 5;
                    } 
                }
            }
            if (code[it] == ';') {
                it++;
            }

            if (rd != 0) {
                if (bn == 2) { 
                    jit[*size] = 0x66;
                    (*size)++;
                }
                
                jit[*size] = (bn == 1) ? 0x88 : 0x89;
                jit[*size + 1] = 0xC0 | rd;
                *size += 2;

                jit[*size] = 0x58; // pop eax
                (*size)++;

                rd = 0;
            }
        }
        // Return Expretion
        else if (code[it] == 'r' && code[it + 1] == 'e' && code[it + 2] == 't') {
            it += 3;
            jit[*size] = 0xC3;
            *size += 1;
        }
        // Byte insertion
        else if (code[it] == 'b' && code[it + 1] == '$') {
            it += 2;
            while (IsSpace(code[it])) it++;
            while (code[it] != ';') {
                i32 x = ParseU32(code, &it);
                u8 b = ((u8)x) ;
                jit[*size] = b;
                (*size)++;
                while (IsSpace(code[it])) it++;
            }
            it++;
        } 
        // Lang identifiers
        else if ((code[it] >= 'a' && code[it] <= 'z') || (code[it] >= 'A' && code[it] <= 'Z') || code[it] == '_') {
            char id_name[MAX_NAME_LEN];
            u32 id_idx = 0;
            
            while (code[it] != '\0' && (
                   (code[it] >= 'a' && code[it] <= 'z') || 
                   (code[it] >= 'A' && code[it] <= 'Z') || 
                   (code[it] >= '0' && code[it] <= '9') || 
                   code[it] == '_')) {
                if (id_idx < MAX_NAME_LEN - 1) {
                    id_name[id_idx++] = code[it];
                }
                it++;
            }
            id_name[id_idx] = '\0';

            u32 target_pc = FindFunctionPC(func_table, id_name);
            
            if (target_pc != 0xFFFFFFFF) {
                jit[*size] = 0xE8;
                i32 rel_offset = (i32)target_pc - (i32)(*size + 5);
                *(i32*)(jit + *size + 1) = rel_offset;
                *size += 5;
            } else {
                jit[*size] = 0xE8;
                i32 rel_offset = 0;
                *(i32*)(jit + *size + 1) = rel_offset;
                *size += 5;
            }
        }
        // Explicit Register Indicator / Returned Reg
        else if (code[it] == '#') {
            it++;
            while (IsSpace(code[it])) it++;
            if (code[it] == 'a') {
                it++;
                while (IsSpace(code[it])) it++;
                if (code[it] == '=') {
                    it++;
                    while (IsSpace(code[it])) it++;
                    u8 rc = 0xC0 | (LetterToRegister(code[it]) << 3);
                    it++;
                    jit[*size] = 0x89;
                    jit[*size+1] = rc;
                    *size += 2;
                }
            }
            else { 
                rd = LetterToRegister(code[it]);
                it++;
            }
        }
        // Comments
        else if (code[it] == '/' && code[it + 1] == '/') {
            it += 2;
            while (code[it] != '\n' && code[it] != '\r' && code[it] != 0) {
                it++;
            }
            it++;
        }
        // Ebx = Eax
        else if (code[it] == '@' && code[it + 1] == 'l') {
            it += 2;
            jit[*size] = 0x89;
            jit[*size + 1] = 0xC3;
            *size += 2;
        }
        // Write Value: mov [ebx], al / ax / eax
        else if (code[it] == '@' && code[it + 1] == 'w') {
            it += 2;
            if (code[it] == 'b') {
                jit[*size] = 0x88;       // mov [ebx], al
                jit[*size + 1] = 0x03;
                *size += 2;
                it++;                    // ¡Avanzar el iterador del modificador!
            }
            else if (code[it] == 'w') {
                jit[*size] = 0x66;       // Prefijo 16-bits
                jit[*size + 1] = 0x89;   // mov [ebx], ax
                jit[*size + 2] = 0x03;
                *size += 3;
                it++;                    // ¡Avanzar el iterador!
            }
            else if (code[it] == 'd') {
                jit[*size] = 0x89;       // mov [ebx], eax
                jit[*size + 1] = 0x03;
                *size += 2;
                it++;                    // ¡Avanzar el iterador!
            }
        }
        // Read Value: mov al / ax / eax, [ebx]
        else if (code[it] == '@' && code[it + 1] == 'r') {
            it += 2;
            if (code[it] == 'b') {
                jit[*size] = 0x8A;       // mov al, [ebx]
                jit[*size + 1] = 0x03;
                *size += 2;
                it++;
            }
            else if (code[it] == 'w') {
                jit[*size] = 0x66;       // 16-bits Prefix
                jit[*size + 1] = 0x8B;   // mov ax, [ebx]
                jit[*size + 2] = 0x03;
                *size += 3;
                it++;
            }
            else if (code[it] == 'd') {
                jit[*size] = 0x8B;       // mov eax, [ebx]
                jit[*size + 1] = 0x03;
                *size += 2;
                it++;
            }
        }
        // Conditional Jumps (ex: @je target, @jne target, o @je_ target)
        else if (code[it] == '@' && code[it + 1] == 'j') {
            it += 2;

            char cond[3] = {0};
            cond[0] = code[it++];
            cond[1] = code[it++];

            if (cond[1] == '_') {
                cond[1] = '\0';
            }

            while (IsSpace(code[it])) it++;

            char id_name[MAX_NAME_LEN];
            int uidx = 0;
            while (code[it] != '\0' && (
                   (code[it] >= 'a' && code[it] <= 'z') || 
                   (code[it] >= 'A' && code[it] <= 'Z') || 
                   (code[it] >= '0' && code[it] <= '9') || 
                   code[it] == '_')) {
                if (uidx < MAX_NAME_LEN - 1) { 
                    id_name[uidx++] = code[it];
                } 
                it++;
            } 
            id_name[uidx] = 0;

            u8 cond_code = 0x4;
            
            if      (strcmp(cond, "o") == 0)  cond_code = 0x0;
            else if (strcmp(cond, "no") == 0) cond_code = 0x1;
            else if (strcmp(cond, "b") == 0 || strcmp(cond, "c") == 0)  cond_code = 0x2;
            else if (strcmp(cond, "ae") == 0 || strcmp(cond, "nc") == 0) cond_code = 0x3;
            else if (strcmp(cond, "e") == 0 || strcmp(cond, "z") == 0)  cond_code = 0x4;
            else if (strcmp(cond, "ne") == 0 || strcmp(cond, "nz") == 0) cond_code = 0x5;
            else if (strcmp(cond, "be") == 0) cond_code = 0x6;
            else if (strcmp(cond, "a") == 0)  cond_code = 0x7;
            else if (strcmp(cond, "s") == 0)  cond_code = 0x8;
            else if (strcmp(cond, "ns") == 0) cond_code = 0x9;
            else if (strcmp(cond, "p") == 0)  cond_code = 0xA;
            else if (strcmp(cond, "np") == 0) cond_code = 0xB;
            else if (strcmp(cond, "l") == 0)  cond_code = 0xC;
            else if (strcmp(cond, "ge") == 0) cond_code = 0xD;
            else if (strcmp(cond, "le") == 0) cond_code = 0xE;
            else if (strcmp(cond, "g") == 0)  cond_code = 0xF;

            jit[*size]     = 0x0F;
            jit[*size + 1] = 0x80 | cond_code;
            *size += 2;

            u32 target_pc = FindFunctionPC(func_table, id_name);
            i32 rel_offset = 0;
            if (target_pc != 0xFFFFFFFF) {
                rel_offset = (i32)target_pc - (i32)(*size + 4);
            }
            
            *(i32*)(jit + *size) = rel_offset;
            *size += 4;
        }
        else {
            it++;
        }
    }
    return 1;
}

u8 TryParseRt(char* code, u32* size, u8* jit) {
    FunctionTable pm;
    pm.count = 0;
    TryParse(code, size, jit, &pm);
    *size = 0;
    return TryParse(code, size, jit, &pm);
}
