/*
 * CS 261 PA1: Mini-ELF header verifier
 *
 * Name: Ben Berry
 */

#include "p1-check.h"

/**********************************************************************
 *                         REQUIRED FUNCTIONS
 *********************************************************************/

bool read_header (FILE *file, elf_hdr_t *hdr) {

    //Fail to read file if file is null, the header information could not be read, or the magic number is incorrect
    if(file == NULL ||(fread(hdr, 16, 1, file) != 1) ||  hdr->magic != 4607045) {
        printf("Failed to read file\n");
        return false;
    }
    return true;
}

/**********************************************************************
 *                         OPTIONAL FUNCTIONS
 *********************************************************************/

void usage_p1 (char **argv) {
    printf("Usage: %s <option(s)> mini-elf-file\n", argv[0]);
    printf(" Options are:\n");
    printf("  -h      Display usage\n");
    printf("  -H      Show the Mini-ELF header\n");
}

bool parse_command_line_p1 (int argc, char **argv, bool *printHeaderFlag, char **filename) {
    //check for null parameters
    if(argc <= 1 || argv == NULL|| printHeaderFlag == NULL) {
        usage_p1(argv);
        return false;
    }
    *printHeaderFlag = false;
    int opt; 
    bool printHelpFlag = false;

    //Parse command line arguments
    while((opt = getopt(argc, argv, "hH"))!= -1) {
        switch(opt) {
            case 'h': 
                printHelpFlag = true; 
                break;
            case 'H': 
                *printHeaderFlag = true; 
                break;
            default: 
                usage_p1(argv); 
                return false;
        }
    }

    //Check for extra arguments or parameters
    if(optind != argc-1) {
        usage_p1(argv);
        return false;
    }

    //Print help text
    if(printHelpFlag) {
        *printHeaderFlag = false;
        usage_p1(argv);
        return true;
    }

    //Store file name
    if(printHeaderFlag) {
        *filename = argv[optind];
        //If no file was given, print help text
        if(*filename == NULL) {
            usage_p1(argv);
            return false;
        }
        return true;
    }
    return true;
}

void dump_header (elf_hdr_t *hdr) {
    //Print each byte in the header 1 by 1 (16 total)
    //First, make a pointer to iterate through the struct
    unsigned char *bytePointer = (unsigned char*)  hdr;
    int i;
    for(i = 0; i < 16; i++) {

        //Print an extra whitespace after the 8th byte
        if(i == 8)
            printf("%s", " ");
        printf("%02x", bytePointer[i]);
        //Only print a space if the current byte is not the last one
        if(i != 15)
            printf(" ");
    }   

    //print the file descriptions
    printf("\n%s %d%s", "Mini-ELF version",hdr->e_version, "\n");
    printf("%s %#x%s", "Entry point",hdr->e_entry, "\n");
    printf("There are %d program headers, starting at offset %d (%#x)\n",hdr->e_num_phdr,hdr->e_phdr_start, hdr->e_phdr_start);

    //Check for empty fields, otherwise print them
    if(hdr->e_symtab == 0)
        printf("%s", "There is no symbol table present\n");
    else
        printf("There is a symbol table starting at offset %d (%#x)\n",hdr->e_symtab, hdr->e_symtab);
    if(hdr->e_strtab == 0)
        printf("%s", "There is no string table present\n");
    else
        printf("There is a string table starting at offset %d (%#x)\n",hdr->e_strtab, hdr->e_strtab);
}

