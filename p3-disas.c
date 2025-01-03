/*
 * CS 261 PA3: Mini-ELF disassembler
 *
 * Name: Ben Berry
 */

#include "p3-disas.h"

void printRegister(uint32_t reg);
void printSpaces(int num);
/**********************************************************************
 *                         REQUIRED FUNCTIONS
 *********************************************************************/

y86_inst_t fetch (y86_t *cpu, byte_t *memory) {

    y86_inst_t instruction;
     if (!cpu || !memory) {
        instruction.icode = INVALID;
        return instruction;
    }
    uint64_t *ptr;
    uint8_t code = (memory[cpu->pc] & 0xF0) >> 4;
    uint8_t fun = (memory[cpu->pc]& 0x0F);
    uint8_t b2;

    //populate ins with 0s
    memset(&instruction, 0x00, sizeof(instruction));

    //switch for the first byte of the instruction
    //Use this to set instruction code, flags and other needed values for ins
    switch(code) {
        //HALT
        case (HALT): 
            instruction.icode = HALT; 
            instruction.ifun.b = fun;
            instruction.valP = cpu->pc + 1;
            if(fun != 0)
                instruction.icode = INVALID;
            if(cpu->pc + instruction.valP >= MEMSIZE)
                instruction.icode = INVALID;
            break;

        //NOP  
        case (NOP):
            instruction.icode = NOP;
            instruction.ifun.b = fun;
            instruction.valP = cpu->pc + 1;
            if(fun != 0)
                instruction.icode = INVALID;
            if(cpu->pc + instruction.valP >= MEMSIZE)
                instruction.icode = INVALID;
            break;

        //CMOVXX
        case (CMOV):
            instruction.icode = CMOV; 
            instruction.ifun.b = fun; 
            instruction.ifun.cmov = instruction.ifun.b; 
            instruction.valP = cpu->pc + 2;
            b2 = memory[cpu->pc + 1];    
            instruction.ra = ((b2 & 0xF0) >> 4); 
            instruction.rb = (b2 & 0x0F);
            //Check for out of bounds registers or instruction code
            if(instruction.ra >= NOREG || instruction.rb >= NOREG || instruction.ifun.cmov >= BADCMOV) {
                instruction.icode = INVALID; 
                cpu->stat=INS;
            }
            break;

        //IRMOVQ      
        case (IRMOVQ): 
            instruction.icode = IRMOVQ;
            instruction.ifun.b = fun;
            instruction.valP = cpu->pc + 10;
            b2 = memory[cpu->pc + 1];
            instruction.rb = (b2 & 0x0F);
            //Check that first register is equal to NOREG
            //Since IRMOVQ has an immediate value as the source
            //Also checks that destination register is not out of bounds 
            if(((b2 & 0xF0) >> 4) != NOREG || instruction.rb >= NOREG || instruction.ifun.b != 0) {
                instruction.icode = INVALID; 
                break;
            }
            //Store the address of the bytes representing the value location in p
            //This is cast to a int pointer since byte_t objects are char pointers.
            ptr = (uint64_t *) &memory[cpu->pc + 2];
            //Dereference p and assign it to the value field of the instruction
            instruction.valC.v = *ptr;
            break;  

        //RMMOVQ      
        case (RMMOVQ): 
            instruction.icode = RMMOVQ; 
            instruction.valP = cpu->pc + 10;
            instruction.ifun.b = fun;
            b2 = memory[cpu->pc + 1];
            instruction.ra = ((b2 & 0xF0) >> 4);
            instruction.rb = (b2 & 0x0F);
            //Check for out of bounds registers
            if(instruction.ra > NOREG || instruction.rb > NOREG || instruction.ifun.b != 0) {
                instruction.icode = INVALID; 
                cpu->stat=ADR;
                break;
            }
            //Store the address of the bytes representing the destination location in p
            //This is cast to a int pointer since byte_t objects are char pointers.
            ptr = (uint64_t *) &memory[cpu->pc + 2];
            //Dereference p and assign it to the destination field of the instruction
            instruction.valC.d = *ptr;
            break;  

        //MRMOVQ    
        case (MRMOVQ): 
            instruction.icode = MRMOVQ; 
            instruction.valP = cpu->pc + 10;
            instruction.ifun.b = fun;
            b2 = memory[cpu->pc + 1];
            instruction.ra = ((b2 & 0xF0) >> 4);
            instruction.rb = (b2 & 0x0F);
            //Check for out of bounds registers
            if( instruction.ra >= NOREG || instruction.ifun.b != 0) {
                instruction.icode = INVALID;
                cpu->stat=ADR; 
                break;
            }

            //Store the address of the bytes representing the destination location in p
            //This is cast to a int pointer since byte_t objects are char pointers.
            ptr = (uint64_t *) &memory[cpu->pc + 2];
            //Dereference p and assign it to the destination field of the instruction
            instruction.valC.d = *ptr;
            break;  
        
        //OPQ
        case (OPQ):
            instruction.icode = OPQ; 
            instruction.ifun.op = fun; 
            instruction.valP = cpu->pc + 2;
            b2 = memory[cpu->pc + 1];
            instruction.ra = ((b2 & 0xF0) >> 4);
            instruction.rb = (b2 & 0x0F);
            //Check for out of bounds registers or instruction code
            if(instruction.ra > NOREG || instruction.rb > NOREG|| instruction.ifun.op >= BADOP) {
                instruction.icode = INVALID;
                cpu->stat=ADR;
            }
            break;
        
        //JXX
        case (JUMP):
            instruction.icode = JUMP; 
            instruction.ifun.jump = fun; 
            instruction.valP = cpu->pc + 9;

            if (instruction.ifun.jump >= BADJUMP)
                instruction.icode = INVALID;
            //Save jump desitination to p in the same way as above
            ptr = (uint64_t *) &memory[cpu->pc + 1];
            //If p is null, set cpu stat to ADR
            if (!ptr)
                cpu->stat=ADR;
            //Otherwise, dereference p and store in the dest field.
            instruction.valC.dest = *ptr;
            break;  

        //CALL 
        case (CALL): 
            instruction.icode = CALL; 
            instruction.valP = cpu->pc + 9;
            if(fun != 0)
                instruction.icode = INVALID;
            //Save call desitination to p in the same way as above
            ptr = (uint64_t *) &memory[cpu->pc + 1];
            //If p is null, set cpu stat to ADR
            if (!ptr)
                cpu->stat=ADR;
            //Otherwise, dereference p and store in the dest field.
            instruction.valC.dest = *ptr;
            break;

        //RET     
        case (RET):
            instruction.icode = RET; 
            instruction.valP = cpu->pc + 1;
            if(fun != 0)
                instruction.icode = INVALID;
            break;
        
        //PUSHQ
        case (PUSHQ): 
            instruction.icode = PUSHQ; 
            instruction.valP = cpu->pc + 2;

            b2 = memory[cpu->pc + 1];
            instruction.ra = ((b2 & 0xF0) >> 4);
            //Check that first register is equal to NOREG
            //Also checks that destination register is not out of bounds 
            if(((b2 & 0xF)) != NOREG || instruction.ra >= NOREG)
                instruction.icode = INVALID;
            if(fun != 0)
                instruction.icode = INVALID;          
            break;

        //POPQ        
        case (POPQ): 
            instruction.icode = POPQ;
            instruction.valP = cpu->pc + 2;

            b2 = memory[cpu->pc + 1];
            instruction.ra = ((b2 & 0xF0) >> 4);
            //Check that first register is equal to NOREG
            //Also checks that destination register is not out of bounds 
            if(((b2 & 0xF)) != NOREG || instruction.ra > NOREG)
                instruction.icode = INVALID; 
            if(fun != 0)
                instruction.icode = INVALID;       
            break;
        
        //IOTRAP
        case (IOTRAP):
            instruction.icode = IOTRAP; 
            instruction.valP = cpu->pc + 1; 
            instruction.ifun.b = fun;
            instruction.ifun.trap = instruction.ifun.b;
            if (instruction.ifun.trap >= BADTRAP)
                instruction.icode = INVALID;
            break;

        //INVALID INSTRUCTIONS
        default: 
            instruction.icode = INVALID;
            instruction.ifun.b = 0xF;
            break;     
   }

    //If instruction is invalid, set the cpu status to INS
    if(instruction.icode == INVALID)
        cpu->stat = INS;
    
    //If the instruction is too large to fit in memory, set cpu status to ADR
    if(cpu->pc + instruction.valP >= MEMSIZE){
        instruction.icode = INVALID;
        cpu->stat = ADR;
    }
   return instruction;
}

/**********************************************************************
 *                         OPTIONAL FUNCTIONS
 *********************************************************************/

void usage_p3 (char **argv) {
    printf("Usage: %s <option(s)> mini-elf-file\n", argv[0]);
    printf(" Options are:\n");
    printf("  -h      Display usage\n");
    printf("  -H      Show the Mini-ELF header\n");
    printf("  -a      Show all with brief memory\n");
    printf("  -f      Show all with full memory\n");
    printf("  -s      Show the program headers\n");
    printf("  -m      Show the memory contents (brief)\n");
    printf("  -M      Show the memory contents (full)\n");
    printf("  -d      Disassemble code contents\n");
    printf("  -D      Disassemble data contents\n");
}

bool parse_command_line_p3 (int argc, char **argv, bool *print_header, bool *print_phdrs,
        bool *print_membrief, bool *print_memfull, bool *disas_code, bool *disas_data, char **filename) {

    if(argc <= 1 || argv == NULL || print_header == NULL || print_membrief == NULL || print_phdrs == NULL 
            || print_memfull == NULL || disas_code == NULL || disas_data == NULL) {

        usage_p3(argv);
        return false;
    }

    *print_header = false;
    *print_phdrs = false;
    *print_membrief = false;
    *print_memfull = false;
    *disas_code = false;
    *disas_data = false;
    int opt = -1;
    char *optionStr = "hHafsmMDd";
    bool printHelp = false;

    while((opt = getopt(argc, argv, optionStr))!= -1) {
        switch(opt) {
            case 'h': printHelp = true; break;
            case 'H': *print_header = true; break;
            case 'm': *print_membrief = true; break;
            case 'M': *print_memfull = true; break;
            case 's': *print_phdrs = true; break;
            case 'a': *print_header = true; *print_membrief = true; *print_phdrs = true; break; 
            case 'f': *print_header = true; *print_memfull = true; *print_phdrs = true; break;
            case 'd': *disas_code = true; break;
            case 'D': *disas_data = true; break;
            default: usage_p3(argv); return false;
        }
    }
    
    if(printHelp) {
        *print_header = false;
        usage_p3(argv);
        return false;
    } else if(*print_membrief && *print_memfull) {
        usage_p3(argv);
        return false;
    } else {
        *filename = argv[optind];
        if(*filename == NULL || argc > optind + 1) {
            usage_p3(argv);
            return false;
        }
        return true;
    }
    return true;
}

void disassemble (y86_inst_t *inst) {
    //switch over the instruction code
    switch(inst->icode) {

        case HALT: printf("halt"); break;
        case NOP: printf("nop"); break;
        case CMOV: 
            //Multiple options for CMOV
            switch(inst->ifun.cmov) {
                case RRMOVQ: printf("rrmovq "); break; 
                case CMOVLE: printf("cmovle "); break;
                case CMOVL: printf("cmovl "); break;
                case CMOVE: printf("cmove "); break;
                case CMOVNE: printf("cmovne "); break;
                case CMOVGE: printf("cmovge " ); break;
                case CMOVG: printf("cmovg "); break;
                case BADCMOV: return; 
            } 
            //Print registers
            printRegister(inst->ra);
            printf(", ");
            printRegister(inst->rb);
            break;
            
        case IRMOVQ: 
            printf("irmovq ");
            //Print memory location and register 
            printf("0x%lx, ", inst->valC.v);
            printRegister(inst->rb);
            break;
            
        case RMMOVQ: 
            printf("rmmovq "); 
            //Print registers, and check destination is offset or absolute
            printRegister(inst->ra);
            //Offset
            if(inst->rb != 0xf) {
                printf(", 0x%lx(", inst->valC.d); 
                printRegister(inst->rb);
                printf(")");
            //Absolute
            } else
                printf(", %#lx", inst->valC.d); 
            break;
            
        case MRMOVQ: 
            printf("mrmovq "); 
            //Check source is offset or absolute
            //Offset
            if(inst->rb != 0xF) {
                printf("0x%lx(", inst->valC.d); 
                printRegister(inst->rb);
                printf("), ");
            //Absolute
            } else
                printf("%#lx, ", inst->valC.d);
            //Print the destination register
            printRegister(inst->ra);
            break;
            
        case OPQ:
            //Multiple options for OPQ 
            switch(inst->ifun.op) {
                case ADD: printf("addq "); break;
                case SUB: printf("subq "); break;
                case AND: printf("andq "); break;
                case XOR: printf("xorq "); break;
                case BADOP:return;  
            }
            //Print registers
            printRegister(inst->ra);
            printf(", ");
            printRegister(inst->rb);
            break;
            
        case JUMP: 
            //Multiple options for JUMP
            switch(inst->ifun.jump) {
                case JMP: printf("jmp "); break;
                case JLE: printf("jle "); break;
                case JL: printf("jl "); break;
                case JE: printf("je "); break;
                case JNE: printf("jne "); break;
                case JGE: printf("jge "); break;
                case JG: printf("jg "); break;
                case BADJUMP: return;

            }
            //Print memory destination
            printf("%#lx", inst->valC.dest); 
            break;
            
        case CALL: 
            printf("call "); 
            //Print memory destination
            printf("%#lx", inst->valC.dest); 
            break;
        case RET: 
            printf("ret"); 
            break;
        case PUSHQ: 
            printf("pushq "); 
            //Print register
            printRegister(inst->ra); 
            break;
        case POPQ: 
            printf("popq ");
            //Print register 
            printRegister(inst->ra); 
            break;
        case IOTRAP: 
            printf("iotrap %d", inst->ifun.trap); 
            break;
        case INVALID: 
            break;
       
    }


}

void disassemble_code (byte_t *memory, elf_phdr_t *phdr, elf_hdr_t *hdr) {

    //Check for null parameters
    if(memory == NULL || phdr == NULL || hdr == NULL) 
        return;

    y86_t cpu;
    y86_inst_t isntruction;
    uint32_t addr = phdr->p_vaddr;
    cpu.pc = addr;
   
    //Print the start of the segment at the given virtual address
    printf("  0x%03lx", cpu.pc);
    printf(":%30s","");
    printf(" | .pos 0x");
    printf("%03lx code",  cpu.pc);
    printf("\n");
    
    //Loops until the program counter reaches the end of the current program header
    while(cpu.pc < addr + phdr->p_size) {
        //Counter to determine how many spaces need to be printed
        //to fill gaps if bytes don't fill the entire space.
        int totalSpaces = 8;

        //Print the start of the code segment
        if(cpu.pc == hdr->e_entry){
            printf("  0x%03lx", cpu.pc);
            printf(":%31s", "");
            printf("| _start:\n");
        }
        isntruction = fetch(&cpu, memory);
        //End dissassembly if invalid instruction is found.
        if(isntruction.icode == INVALID) {
            printf("Invalid opcode: 0x%x%x\n\n", isntruction.ifun.b, isntruction.icode);
            cpu.pc += isntruction.valP;
            return;
        }
        //Print the program counter
        printf("  0x%03lx: ", cpu.pc);
        //Print the hex for current instruction
        for(int i = cpu.pc; i < isntruction.valP; i++) {
            printf("%02x ", memory[i]);
            //Decrement total spaces for each byte that is printed.
            totalSpaces -= 1; 
        }
        
        //Print the number of spaces required to fill the rest of the space
        printSpaces(totalSpaces);
        printf("|   ");
        //Disassemble the instruction code
        disassemble(&isntruction);
        printf("\n");
        //Increment the program counter, move to the next line and loop back
        cpu.pc += isntruction.valP - cpu.pc;
    }
    printf("\n");
}

void disassemble_data (byte_t *memory, elf_phdr_t *phdr) {

    //Check for null parameters
    if(memory == NULL || phdr == NULL )
        return;

    y86_t cpu;
    uint32_t addr = phdr->p_vaddr;
    cpu.pc = addr;
    
    //Print the program counter and the start of the data segment
    printf("  0x%03lx:", cpu.pc);
    printf("%30s","");
    printf(" | .pos 0x");
    printf("%03lx data\n", cpu.pc);
    //Loop until the program counter reaches the end of the program header
    while(cpu.pc <  addr + phdr->p_size) {
        //Print the program counter
        printf("  0x%03lx: ", cpu.pc);
        //Print each byte stored in the data location (8 total as they are quads)
        //Use a temporary value 'i' instead of incrementing program counter
        //so it can be used again later
        for(int i = cpu.pc; i < cpu.pc + 8; i++)
            printf("%02x ", memory[i]);
        printf("%6s",""); 
        printf("|   .quad ");
        
        //Assign p to the location in memory where the 8 bytes of data starts
        uint64_t *data;
        data = (uint64_t *) &memory[cpu.pc];
        //Print p
        printf("0x%lx\n", *data);
        //Increment the program counter to move to the next data entry 
        cpu.pc += 8;
         
    }
    printf("\n");
}

void disassemble_rodata (byte_t *memory, elf_phdr_t *phdr) {
    //Keeps track of the amount of spaces needed to fill gaps if not enough bytes are entered.
    int totalSpaces = 8;

    //Check for null parameters
    if(memory == NULL || phdr == NULL)
        return;
    y86_t cpu;
    uint32_t addr = phdr->p_vaddr;
    cpu.pc = addr;
    //Switched to true when the end of the string is reached.
    bool finished = false;
    //Used in place of the program counter to ensure the program counter doesn't
    //become misaligned with what is currently happening
    int counter;
    //Print starting information
    printf("  0x%03lx:  ", cpu.pc);
    printf("%28s","");
    printf(" | .pos 0x%03lx rodata\n", cpu.pc);
    
    //Loops until the program counter reaches the end of the header
    while(cpu.pc <  addr + phdr->p_size) {
        finished = false;
        printf("  0x%03lx: ", cpu.pc);

        //Prints up to 10 bytes of data
        //Breaks out of the loop if the end of the string is reached
        for(counter = cpu.pc; counter < cpu.pc + 10; counter++) {
            printf("%02x ", memory[counter]);
            totalSpaces --;
            //End of string is reached
            if(memory[counter] == 0x00) {
                finished = true;
                printSpaces(totalSpaces);
                totalSpaces = 8;
                break;
            }  
        }
        //Move the counter back to the program counter (start of the data)
        counter = cpu.pc;
        printf("|   .string \"");
        //Prints the bytes as chars to repesent the string at the end of the line
        //NOTE: This is the entire string up until the 0x00 instance, 
        //not just the 10 bytes printed in this line.
        while(memory[counter] != 0x00)
            printf("%c", memory[counter++]);
        //Stores the total number of bytes in the string
        int bytes = (counter - cpu.pc) + 1;
        printf("\"");

        //If the string has more bytes to print, continue here
        if(!finished) {
            totalSpaces = 7;
            
            //Increment counter to the 
            counter = cpu.pc + 10;
            int j = 0;
            printf("\n  0x%03x: ", counter);
            
            //Loop until end byte found
            while(memory[counter] != 0x00) {
                printf("%02x ", memory[counter++]);
                totalSpaces --;

                //10 bytes printed, move to next line
                if(++j % 10 == 0) {
                    printf("| \n  ");
                    printf("0x%03x: ", counter);
                    totalSpaces = 7;
                }
            }
            //Prints the 0x00 byte
            printf("%02x", memory[counter++]);
            //Prints spaces as needed to fill remaining space
            printSpaces(totalSpaces);
            printf(" | ");
        }
        printf("\n"); 
        //Move program counter to the end of the current string.
        cpu.pc += bytes; 
            
    }
    printf("\n");
}



/**********************************************************************
 *                         HELPER METHODS
 *********************************************************************/


//Prints register name based on the given register
void printRegister(uint32_t reg) {

    switch(reg) {
        case 0x0: printf("%%rax"); break;
        case 0x1: printf("%%rcx"); break;
        case 0x2: printf("%%rdx"); break;
        case 0x3: printf("%%rbx"); break;
        case 0x4: printf("%%rsp"); break;
        case 0x5: printf("%%rbp"); break;
        case 0x6: printf("%%rsi"); break;
        case 0x7: printf("%%rdi"); break;
        case 0x8: printf("%%r8"); break;
        case 0x9: printf("%%r9"); break;
        case 0xa: printf("%%r10"); break;
        case 0xb: printf("%%r11"); break;
        case 0xc: printf("%%r12"); break;
        case 0xd: printf("%%r13"); break;
        case 0xe: printf("%%r14"); break;
        //NOREG
        case 0xf: break;
        //Any other case, nothing will happen
    }

}
//Prints x number of spaces to fill x missing bytes at the end of a line
//Each byte equates to 3 spaces (ex: if num = 8, 24 spaces will be printed)
void printSpaces(int num) {
    for(int i = 0; i <= num + 1; i++)
        printf("   ");
}



             
        
            
        
            
        
            
        
            
        
        
        
        
        
            
       
            
        
            
        