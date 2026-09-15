/**
 * Naca Lang Compiler
 */

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

u8 TryParse(char* code, u32* size, u8* jit, FunctionTable* func_table) {
    *size = 0;
    u32 it = 0;
    
    while (code[it] != '\0') {
        // Functions Declarations
        if (
            code[it] == 'f' && code[it + 1] == 'n' && 
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
            it++;
            while (IsSpace(code[it])) it++;

            // Initialize It
            if (code[it] >= '0' && code[it] <= '9') {
                jit[*size] = 0xB8;
                u32 first_val = ParseU32(code, &it);
                *(u32*)(jit + *size + 1) = first_val;
                *size += 5;
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

                if (op == '+') {
                    if (is_self) {
                        // add eax, eax (Opcode: 0x01 0xC0)
                        jit[*size]     = 0x01;
                        jit[*size + 1] = 0xC0;
                        *size += 2;
                    } else {
                        // add eax, imm32 (Opcode: 0x05)
                        jit[*size] = 0x05;
                        *(u32*)(jit + *size + 1) = next_val;
                        *size += 5;
                    }
                } 
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
                            id_name[uidx++] = code[it] ;
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
                else if (op == '-') {
                    if (is_self) {
                        // sub eax, eax (Opcode: 0x29 0xC0)
                        jit[*size]     = 0x29;
                        jit[*size + 1] = 0xC0;
                        *size += 2;
                    } else {
                        // sub eax, imm32 (Opcode: 0x2D)
                        jit[*size] = 0x2D;
                        *(u32*)(jit + *size + 1) = next_val;
                        *size += 5;
                    }
                } 
                else if (op == '*') {
                    if (is_self) {
                        // imul eax, eax (Opcode: 0x0F 0xAF 0xC0)
                        jit[*size]     = 0x0F;
                        jit[*size + 1] = 0xAF;
                        jit[*size + 2] = 0xC0;
                        *size += 3;
                    } else {
                        // imul eax, eax, imm32 (Opcodes: 0x69 0xC0)
                        jit[*size]     = 0x69;
                        jit[*size + 1] = 0xC0;
                        *(u32*)(jit + *size + 2) = next_val;
                        *size += 6;
                    }
                }
                else if (op == '/') {
                    if (is_self) {
                        // mov ebx, eax (89 C3), cdq (99), idiv ebx (F7 FB)
                        jit[*size]     = 0x89;
                        jit[*size + 1] = 0xC3;
                        *size += 2;
                        jit[*size]     = 0x99;
                        *size += 1;
                        jit[*size]     = 0xF7;
                        jit[*size + 1] = 0xFb;
                        *size += 2;
                    } else {
                        // mov ebx, imm32 (0xBB)
                        jit[*size]     = 0xBB;
                        *(u32*)(jit + *size + 1) = next_val;
                        *size += 5;
                        // cdq (0x99)
                        jit[*size]     = 0x99;
                        *size += 1;
                        // idiv ebx (0xF7 0xFB)
                        jit[*size]     = 0xF7;
                        jit[*size + 1] = 0xFB;
                        *size += 2;
                    }
                }
            }
            if (code[it] == ';') {
                it++;
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
                *size++;
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
