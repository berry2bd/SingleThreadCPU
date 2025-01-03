/*
 * CS 261 PA2: Mini-ELF loader
 *
 * Name: Ben Berry
 */

#include "p2-load.h"

/**********************************************************************
 *                         REQUIRED FUNCTIONS
 *********************************************************************/

bool read_phdr (FILE *file, uint16_t offset, elf_phdr_t *phdr) {
    //Check that no parameters are null.
    if(!file || !offset || !phdr)
        return false;

    //Seek to the part of the file that program headers start. If this fails, return false.
    if(fseek(file, offset, SEEK_SET) != 0)
        return false;

    //Read the header from the file, and save to the phdr struct. If this fails, return false.
    if(fread(phdr, sizeof(elf_phdr_t), 1, file) != 1)
        return false;

    //Check that magic number is correct.
    if(phdr->magic != 0xDEADBEEF)
        return false;
    return true;
}

bool load_segment (FILE *file, byte_t *memory, elf_phdr_t *phdr) {
    //Check if the program header is null.
    if (phdr == NULL)
        return false;
    
    //Check for null parameters and an invalid offset.
    if(!file || !memory || phdr->p_offset < 0)  
        return false;
    
    //Return false if seeking fails.
    if (fseek(file, phdr->p_offset , SEEK_SET) != 0)
        return false;

    //Check that virtual address is valid.
    if(phdr->p_vaddr < 0 || phdr->p_vaddr > 4096)
        return false;
    
    //Read the program header into the allocated virtual memory. If this fails not due to null size, return false.
    if (fread(&memory[phdr->p_vaddr], phdr->p_size, 1, file) != 1 && phdr->p_size)
        return false;
    return true;
}

/**********************************************************************
 *                         OPTIONAL FUNCTIONS
 *********************************************************************/

//Prints the R, W, X permissions flags of the program header. 
void printFlags(uint16_t flags) {
    
    //Checks each bit of the flags integer from left to right, one at a time.
    //Ex. if flags is 6, the first if will compute 110 & 100, which is 100 (4).

        //If the first bit is 1, print an R.
        if((flags & 1 << 2) == 4)
            printf("R");
        else
            printf(" ");

        //If the second bit is 1, print a W.
        if((flags & 1 << 1) == 2)
            printf("W");
        else
            printf(" ");
        
        //If the third bit is 1, print an X.
        if((flags & 1 << 0) == 1)
            printf("X");
        else
            printf(" ");
}

//Returns a string representation of the program header type based on the integer value.
//This includes leading spaces for formatting output correctly.
char* getType(int typeVal) {
    if (typeVal == 0)
        return "DATA      ";
    else if (typeVal == 1)
        return "CODE      ";
    else if (typeVal == 2)
        return"STACK     ";
    else if (typeVal == 3)
        return "HEAP      ";
    else    
        return "UNKNOWN";
}

void usage_p2 (char **argv) {
    printf("Usage: %s <option(s)> mini-elf-file\n", argv[0]);
    printf(" Options are:\n");
    printf("  -h      Display usage\n");
    printf("  -H      Show the Mini-ELF header\n");
    printf("  -a      Show all with brief memory\n");
    printf("  -f      Show all with full memory\n");
    printf("  -s      Show the program headers\n");
    printf("  -m      Show the memory contents (brief)\n");
    printf("  -M      Show the memory contents (full)\n");
}

bool parse_command_line_p2 (int argc, char **argv, bool *print_header, bool *print_phdrs,
        bool *print_membrief, bool *print_memfull, char **filename) {
    
    //Check for any null parameters
    if(argc <= 1 || !argv|| !print_header || !print_membrief || 
            !print_phdrs || !print_memfull) {
        usage_p2(argv);
        return false;
    }

    char *optionStr = "hHafsmM";
    int opt = -1;
    
    while((opt = getopt(argc, argv, optionStr))!= -1) {
        switch(opt) {
            case 'h': 
                break;
            case 'H': 
                *print_header = true;
                break;
            case 'm':
                *print_membrief = true;
                break;
            case 'M': 
                *print_memfull = true;
                break;
            case 's': 
                *print_phdrs = true;
                break;
            case 'a': 
                *print_header = true;
                *print_phdrs = true;
                *print_membrief = true;
                break;
            case 'f': 
                *print_header = true;
                *print_phdrs = true;
                *print_memfull = true;
                break;
            default:
                usage_p2(argv); 
                return false;
        }
    }

    //Check for invalid filename or too many parameters.
        *filename = argv[optind];
        if(!*filename || argc > optind + 1) {
            usage_p2(argv);
            return false;
        }
        

    //Using flags "-m" and "-M" together is invalid.
    if(*print_membrief && *print_memfull) {
        usage_p2(argv);
        return false;
    }
    return true;
}

void dump_phdrs (uint16_t numphdrs, elf_phdr_t *phdrs) {
    
    //Prints the format of the header information.
    printf(" Segment   Offset    Size      VirtAddr  Type      Flags\n");  

    //Parses each header and prints its information.    
    for(int i = 0; i < numphdrs; i++) {
        printf("  %02d       0x%04x    0x%04x    0x%04x    %s", i, 
	    phdrs[i].p_offset, phdrs[i].p_size, phdrs[i].p_vaddr, getType(phdrs[i].p_type));
        printFlags(phdrs[i].p_flags);
        printf("\n");
        
    }
}

void dump_memory (byte_t *memory, uint16_t start, uint16_t end) {
    
    //Total number of bytes printed.
    //Used to add an extra space after the 8th byte and determine when to start a new line.
    int total = 0;

    //Number of bytes printed to the current line.
    //Used to keep track of how many bytes in current line to omit trailing spaces.
    int printedCount = 0;

    //Creates a bitmask to determine the starting point of unaligned values.
    //Ex. If the start was 0x0118, it will be changed to 0x0110 for printing values.
    uint16_t mask = 0xfff0;
    uint16_t newStart = mask&start;
    printf("Contents of memory from %04x to %04x:\n",start,end);
    if(start == end)
	    return;
    printf("  %04x ", newStart);
    
    //Traverses each byte in memory from the start location to the end location and prints them according to the format.
    for(int i = newStart; i < end; i++) {

        //Prints an extra space after the 8th byte.
        //Also checks if the current byte is the last one to be printed in this row to omit trailing spaces.
        if((total % 8) == 0 && printedCount <= 15)
            printf(" ");
        
        //Creates a newline and prints the start location of the next line.
        if((total % 16) == 0 && total != 0) {
            printf("\n");
            printf("  %04x  ", i);

            //Reset printedCount as a new line has started.
            printedCount = 0;
        }

        //Print empty bytes before the true start value as spaces for unaligned data.
        if(memory[i] == 0x0000 && i < start)
            printf("  ");
        else
            printf("%02x", memory[i]);

        //Check whether to add a trailing space after the current byte or not.
        if(printedCount != 15 && i + 1 != end)
            printf(" ");   
        printedCount++;
        total++;
    }
    printf("\n");
}
