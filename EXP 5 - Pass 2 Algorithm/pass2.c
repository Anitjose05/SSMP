#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_LINE 100
#define MAX_TOKEN 20

int main() {
    FILE *intermediate, *optab, *symtab, *output, *prgmlength, *objectcode;
    char label[MAX_TOKEN], opcode[MAX_TOKEN], operand[MAX_TOKEN];
    char mnemonic[MAX_TOKEN], code[MAX_TOKEN], value[MAX_TOKEN];
    char textRecord[MAX_LINE];
    int locctr, start, prgmLength, length = 0, textstartAddr;
    bool opcode_found;

    // Open files with error checking
    intermediate = fopen("intermediate.txt", "r");
    optab = fopen("optab.txt", "r");
    symtab = fopen("symtab.txt", "r");
    prgmlength = fopen("prgmlength.txt", "r");
    output = fopen("output.txt", "w");
    objectcode = fopen("objectcode.txt", "w");

    if (intermediate == NULL || optab == NULL || symtab == NULL ||
        prgmlength == NULL || output == NULL || objectcode == NULL) {
        printf("Error: Unable to open one or more files\n");
        return 1;
    }

    // Read first line
    if (fscanf(intermediate, "%s %s %s", label, opcode, operand) != 3) {
        printf("Error: Invalid intermediate file format\n");
        return 1;
    }

    // Handle START directive
    if (strcmp(opcode, "START") == 0) {
        if (fscanf(prgmlength, "%d", &prgmLength) != 1) {
            printf("Error: Invalid program length file\n");
            return 1;
        }
        if (sscanf(operand, "%x", &start) != 1) {  // Changed to %x for hex start addr
            printf("Error: Invalid starting address\n");
            return 1;
        }
        fprintf(output, "\t\t%s\t%s\t%s\n", label, opcode, operand);
        fprintf(objectcode, "H^%-6s^%06X^%06X\n", label, start, prgmLength);
        textstartAddr = start; // Initialize text start address
    } else {
        printf("Error: Missing START directive\n");
        return 1;
    }

    // Initialize text record
    strcpy(textRecord, "");
    length = 0;

    // Main processing loop
    while (fscanf(intermediate, "%x %s %s %s", &locctr, label, opcode, operand) == 4 &&
           strcmp(opcode, "END") != 0) {
        char objectCode[MAX_LINE] = "";
        opcode_found = false;

        // Write to output file
        fprintf(output, "%04X\t%s\t%s\t%s\t", locctr, label, opcode, operand);

        // Handle opcodes and directives
        if (strcmp(opcode, "BYTE") == 0) {
            if (operand[0] == 'C' && operand[1] == '\'') {
                // Convert chars to hex
                for (int i = 2; i < (int)strlen(operand) - 1; i++) {
                    sprintf(objectCode + strlen(objectCode), "%02X", operand[i]);
                }
                opcode_found = true;
            } else if (operand[0] == 'X' && operand[1] == '\'') {
                // Copy hex digits inside quotes directly
                int len = (int)strlen(operand) - 3; // exclude X' and trailing '
                strncpy(objectCode, operand + 2, len);
                objectCode[len] = '\0';
                opcode_found = true;
            } else {
                printf("Error: Invalid BYTE constant\n");
                return 1;
            }
        } else if (strcmp(opcode, "WORD") == 0) {
            int value_int;
            if (sscanf(operand, "%d", &value_int) != 1) {
                printf("Error: Invalid WORD operand\n");
                return 1;
            }
            sprintf(objectCode, "%06X", value_int);
            opcode_found = true;
        } else if (strcmp(opcode, "RESB") == 0 || strcmp(opcode, "RESW") == 0) {
            // No object code for RESB or RESW - flush text record if any
            if (length > 0) {
                // Remove trailing '^'
                if (textRecord[strlen(textRecord) - 1] == '^') {
                    textRecord[strlen(textRecord) - 1] = '\0';
                }
                fprintf(objectcode, "T^%06X^%02X^%s\n", textstartAddr, length / 2, textRecord);
                strcpy(textRecord, "");
                length = 0;
            }
            opcode_found = true;
        } else {
            // Check optab for opcode
            rewind(optab);
            while (fscanf(optab, "%s %s", mnemonic, code) == 2) {
                if (strcmp(opcode, mnemonic) == 0) {
                    strcpy(objectCode, code);
                    // Check symtab for operand address
                    int sym_addr = -1;
                    rewind(symtab);
                    while (fscanf(symtab, "%s %s", label, value) == 2) {
                        if (strcmp(operand, label) == 0) {
                            if (sscanf(value, "%x", &sym_addr) != 1) {
                                printf("Error: Invalid symbol address in symtab\n");
                                return 1;
                            }
                            break;
                        }
                    }
                    if (sym_addr == -1) {
                        printf("Error: Symbol %s not found in symtab\n", operand);
                        return 1;
                    }
                    sprintf(objectCode + strlen(objectCode), "%04X", sym_addr);
                    opcode_found = true;
                    break;
                }
            }
        }

        // Write object code to output file
        fprintf(output, "%s\n", objectCode);

        if (!opcode_found) {
            printf("Error: Invalid opcode %s\n", opcode);
            return 1;
        }

        if (strlen(objectCode) > 0) {
            // If adding object code exceeds 30 bytes (60 hex chars), flush current text record
            if (length + (int)strlen(objectCode) > 60) {
                if (textRecord[strlen(textRecord) - 1] == '^') {
                    textRecord[strlen(textRecord) - 1] = '\0';
                }
                fprintf(objectcode, "T^%06X^%02X^%s\n", textstartAddr, length / 2, textRecord);
                strcpy(textRecord, "");
                textstartAddr = locctr;
                length = 0;
            }
            // Append object code to text record with '^' separator
            strcat(textRecord, objectCode);
            strcat(textRecord, "^");
            length += (int)strlen(objectCode);
        }
    }

    // Write final text record if exists
    if (length > 0) {
        if (textRecord[strlen(textRecord) - 1] == '^') {
            textRecord[strlen(textRecord) - 1] = '\0';
        }
        fprintf(objectcode, "T^%06X^%02X^%s\n", textstartAddr, length / 2, textRecord);
    }

    // Write END record
    fprintf(output, "%04X\t%s\t%s\t%s\n", locctr, label, opcode, operand);
    fprintf(objectcode, "E^%06X\n", start);

    // Clean up
    fclose(intermediate);
    fclose(optab);
    fclose(symtab);
    fclose(prgmlength);
    fclose(output);
    fclose(objectcode);

    printf("FINISHED EXECUTION!!\n");
    return 0;
}


