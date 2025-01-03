/*
 * CS 261 PA4: Mini-ELF interpreter
 *
 * Name: Ben Berry
 */

#include "p4-interp.h"
void printCpuState(y86_t *cpu);
/**********************************************************************
 *                         REQUIRED FUNCTIONS
 *********************************************************************/

y86_reg_t decode_execute (y86_t *cpu, y86_inst_t inst, bool *cnd, y86_reg_t *valA) {
    
    //Check for null parameters
    if(cpu == NULL)
        return 0;
    if(cnd == NULL || valA == NULL) {
        cpu->stat = INS;
        return 0;
    }
    //Check for out of bounds program counter
    if(cpu->pc >= MEMSIZE || cpu->pc < 0) {
        cpu->stat = ADR;
        return 0;
    }
    //Initialize values for later use
    int64_t valE = 0;
    int64_t valB = 0;
    
    //Switch over the instruction code
    switch(inst.icode) {
        case(HALT): cpu->stat = HLT; break; 
        case(NOP): break;
        case(CMOV): 
            *valA = cpu->reg[inst.ra];
            valE = *valA; 
            //Switch over the cmov function to determine how to set the condition code.
            switch(inst.ifun.cmov) {
                case(RRMOVQ): *cnd = true; break;
                case(CMOVLE): *cnd =  cpu->zf || (cpu->sf && !cpu->of) || (!cpu->sf && cpu->of); break;
                case(CMOVL):  *cnd = (cpu->sf && !cpu->of) || (!cpu->sf && cpu->of); break;
                case(CMOVE):  *cnd =  cpu->zf; break;
                case(CMOVNE): *cnd = !cpu->zf; break;
                case(CMOVGE): *cnd = cpu->sf == cpu->of; break;
                case(CMOVG):  *cnd = !cpu->zf && cpu->sf == cpu->of; break;
                case(BADCMOV): cpu->stat = INS;
            }
            break; 
        case(IRMOVQ): valE =  inst.valC.v; break;
        case(RMMOVQ): *valA = cpu->reg[inst.ra];
            valB =  cpu->reg[inst.rb];
            valE =  inst.valC.d + valB;
            break;
        case(MRMOVQ): valB = cpu->reg[inst.rb];
            valE =  inst.valC.d + valB;
            break;
        case(OPQ): 
            *valA = cpu->reg[inst.ra];
            valB = cpu->reg[inst.rb];  
            //Switch over the operation to determine which flags to set and how to set them 
            switch(inst.ifun.op) {
                
                //Only add and subtract set the overflow flag.
                case(ADD):  valE = (y86_reg_t)(valB + (int64_t)*valA);
                    cpu->of = ((valB < 0) == ((int64_t)*valA < 0)) && ((valE < 0) != (valB < 0));
                    break;
                case(SUB): valE = (y86_reg_t)(valB - (int64_t)*valA);
                    cpu->of = ((valB < 0) != ((int64_t)*valA < 0)) && ((valE < 0) != (valB < 0));
                    break;
                case(AND): valE = valB & *valA; break;
                case(XOR): valE = *valA ^ valB; break;
                //Set cpu status to INS if an out of bounds operation is given
                case(BADOP): cpu->stat = INS;
                default : cpu ->stat = INS; 
            }
            //If the sign bit (highest magnitude bit) is 1, set the sign flag to true.

            //Absolute value is used here because the first bit would sometimes
            //be returned as -1 instead of 1.
            cpu->sf = (abs((valE >> 63)) == 1);
            //If valE is 0, set the zero flag to true.
            cpu->zf = (valE == 0);
            break;
        case(JUMP): 
            //Switch over the type of jump to determine whether jump requirements are met or not.
            //All jump conditions are according to the diagram on slide 12 of Assembly Control Flow.
            switch(inst.ifun.jump) {
                //Unconditional jump
                case(JMP): *cnd = true; break;
                //Jump less than or equal
                case(JLE): *cnd = cpu->zf || (cpu->sf && !cpu->of) || (!cpu->sf && cpu->of); break;
                //Jump less than 
                case(JL):  *cnd = (cpu->sf && !cpu->of) || (!cpu->sf && cpu->of); break;
                //Jump equal to
                case(JE):  *cnd = cpu->zf; break;
                //Jump not equal to
                case(JNE): *cnd = !cpu->zf; break;
                //Jump greater than or equal to
                case(JGE): *cnd = cpu->sf == cpu->of; break;
                //Jump greater than
                case(JG):  *cnd = !cpu->zf && cpu->sf == cpu->of; break;
                //Invalid jump code
                case(BADJUMP): cpu->stat = INS;
            }
            break;
        //Set valB to point to the top of the stack, then lower the stack pointer by 8 and 
        //assign the removed value to valE
        case(CALL): valB = cpu->reg[RSP]; valE =  valB - 8; break;
        case(RET): *valA = cpu->reg[RSP]; valB = cpu->reg[RSP];  valE = valB + 8; break;
        //Push the value of valE onto the stack
        case(PUSHQ): *valA = cpu->reg[inst.ra]; valB = cpu->reg[RSP]; valE = valB - 8;  break;
        //Pop the value at the top of the stack and assign to valE
        case(POPQ): *valA = cpu->reg[RSP]; valB = cpu->reg[RSP]; valE = valB + 8; break;
        //Invalid instruction case
        case(INVALID): cpu->stat = INS; break;
        default: cpu->stat = INS; break;
    }
    return valE;
}

void memory_wb_pc (y86_t *cpu, y86_inst_t inst, byte_t *memory,
        bool cnd, y86_reg_t valA, y86_reg_t valE) {
    y86_reg_t valM;
    //char buf[100];

    //Check for null cpu
    if(cpu == NULL)
        return;
    //Check for null memory
    if(memory == NULL) {
        cpu->stat = INS;
        return;
    }
    //Check for out of bounds program counter or invalid valE or valA
    //Note that a valE or valA will be checked against an upper bound later if needed.
    if(cpu->pc >= MEMSIZE || cpu->pc < 0 || valA < 0 || valE < 0) {
        cpu->stat = ADR;
        return;
    }
    
    //Switch over the instruction code
    switch(inst.icode) {
        case(HALT): cpu->pc = inst.valP; break;
        case(NOP):  cpu->pc = inst.valP; break;
        case(CMOV):
            //Only move data if the move condition is true.
            if(cnd)
                cpu->reg[inst.rb] = valE;
            cpu->pc = inst.valP;
            break;
        case(IRMOVQ): cpu->reg[inst.rb] = valE;  cpu->pc = inst.valP; break;
        case(RMMOVQ): 
            //Check for invalid memory address
            if(valE >=MEMSIZE) {
                cpu->stat = ADR; 
                break;
            }
            //Set 8 bytes of memory at address valE to the value in valA
            *((uint64_t *)&memory[valE]) = valA;
            cpu->pc = inst.valP;
            break;
        case(MRMOVQ): 
            //Check for invalid memory address
            if(valE >=MEMSIZE) {
                cpu->stat = ADR; 
                break; 
            }
            //Assign 8 bytes of memory at address valE to valM
            valM = *((uint64_t *)&memory[valE]);
            //Store valM in register A.
            cpu->reg[inst.ra] = valM;
            cpu->pc = inst.valP;
            break;
        case(OPQ): 
            //Store valE(answer of the operation) in register B
            cpu->reg[inst.rb] = valE;
            cpu->pc = inst.valP;
            break;      
        case(JUMP):
            //Change the program counter to the destination location if the condition is true.
            if(cnd) {
                cpu->pc = inst.valC.dest;
                break;
            }
            cpu->pc = inst.valP;
            break;
        case(CALL):  
            //Check for invalid memory address
            if(valE >=MEMSIZE){ 
                cpu->stat = ADR;
                cpu->pc = inst.valC.dest;
                break;  
            }
            //Set 8 bytes of memory at address valE to valP
            *((uint64_t *)&memory[valE]) = inst.valP;
            //Change the value at the top of the stack to valE
            cpu->reg[RSP] = valE; 
            cpu->pc = inst.valC.dest;
            break;
        case(RET): 
            //Check for invalid memory address
            if(valA >=MEMSIZE) { 
                cpu->stat = ADR; 
                break; 
            }
            //Assign valM to 8 bytes of memory at address valA
            valM = *((uint64_t *)&memory[valA]);
            //Change the value at the top of the stack to valE
            cpu->reg[RSP] = valE;
            cpu->pc = valM; 
            break;
            
        case(PUSHQ): 
            //Check for invalid memory address
            if(valE >=MEMSIZE) { 
                cpu->stat = ADR;
                break; 
            }
            //Assign 8 bytes of memory at address valE to valE
            *((uint64_t *)&memory[valE]) = valA;
            //Change the value at the top of the stack to valE
            cpu->reg[RSP] = valE; 
            cpu->pc = inst.valP;
            break;
            
        case(POPQ):
            //Check for invalid memory address
            if(valA >=MEMSIZE) { 
                cpu->stat = ADR;
                break; 
            }
            //Set valM to 8 bytes of memory at address valA
            valM = *((uint64_t *)&memory[valA]);
            //Change the value at the top of the stack to valE
            cpu->reg[RSP] = valE; 
            //Assign valM to register A
            cpu->reg[inst.ra] = valM;
            cpu->pc = inst.valP;
            break;
        
        //Unimplemented :(
        case (IOTRAP): break;
        //     switch(inst.ifun.trap){
        //         case(CHAROUT):
        //             printf("CHAROUT0");
        //             snprintf(buf, 1, "%s", stdin);
        //             printf("[BUF:%s]", buf);
        //             break;
        //         case(CHARIN):
        //             snprintf(buf, 1, "%s", stdin);
        //             printf("[BUF:%s]", buf);
        //             printf("CHARIN1"); break;
        //         case(DECOUT):
        //             printf("DECOUT2"); break;
        //         case(DECIN):
        //             printf("DECIN3"); break;
        //         case(STROUT):
        //             printf("STROUT4"); break;
        //         case(FLUSH):
        //             printf("FLUSH5"); break;
        //         case(BADTRAP):
        //             printf("BADTRAP6"); break;
        //     }
        //     break;
        case(INVALID): cpu->stat = INS; break;
    }


}

/**********************************************************************
 *                         OPTIONAL FUNCTIONS
 *********************************************************************/

void usage_p4 (char **argv) {
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
    printf("  -e      Execute program\n");
    printf("  -E      Execute program (trace mode)\n");
}

bool parse_command_line_p4 (int argc, char **argv,
        bool *print_header, bool *print_phdrs,
        bool *print_membrief, bool *print_memfull,
        bool *disas_code, bool *disas_data,
        bool *exec_normal, bool *exec_debug, char **filename) {

    //Check for null params or too many arguments
    if(argc <= 1 ||argv == NULL || print_header == NULL || 
    print_phdrs == NULL || print_membrief == NULL || print_memfull == NULL || disas_code == NULL 
    || disas_data == NULL || exec_normal == NULL || exec_debug == NULL) {
        usage_p4(argv);
        return false;
    }

    //Create variables needed for command line parsing with getopt
    char *optionStr = "hHafsmMDdeE";
    int opt = -1;
    bool printHelp = false;
    
    //Parse each command line option
    while((opt = getopt(argc, argv, optionStr))!= -1) {
        //Switch over individual command line options
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
            case 'E': *exec_debug = true; break;
            case 'e': *exec_normal = true; break;
            default: usage_p4(argv); return false;
        }
    }
    
    //Print help message
    if(printHelp) {
        *print_header = false;
        usage_p4(argv);
        return false;
    }
    //Membrief and memfull cannot be printed at the same time
    else if(*print_membrief && *print_memfull) {
        usage_p4(argv);
        return false;
    }
    //Exec normal and exec debug cannot be run at the same time
    else if( *exec_normal && *exec_debug){
        usage_p4(argv);
        return false;
    }
    else {
        //Check for invalid file or too many files given
        *filename = argv[optind];
        if(*filename == NULL || argc > optind + 1) {
            usage_p4(argv);
            return false;
        }
        return true;
    }
}

void dump_cpu_state (y86_t *cpu) {
    printf("Y86 CPU state:\n");
    //Print program counter and flags
    printf("    PC: %016lx   flags: Z%d S%d O%d     ",  cpu->pc, cpu->zf, cpu->sf, cpu->of);
    printCpuState(cpu);
    
    //Print registers and associated values
    printf("  %%rax: %016lx    %%rcx: %016lx\n",  cpu->reg[RAX],  cpu->reg[RCX]);
    printf("  %%rdx: %016lx    %%rbx: %016lx\n", cpu->reg[RDX], cpu->reg[RBX]);
    printf("  %%rsp: %016lx    %%rbp: %016lx\n",   cpu->reg[RSP],  cpu->reg[RBP]);
    printf("  %%rsi: %016lx    %%rdi: %016lx\n",  cpu->reg[RSI], cpu->reg[RDI]);
    printf("   %%r8: %016lx     %%r9: %016lx\n",  cpu->reg[R8], cpu->reg[R9]);
    printf("  %%r10: %016lx    %%r11: %016lx\n",  cpu->reg[R10], cpu->reg[R11]);
    printf("  %%r12: %016lx    %%r13: %016lx\n",  cpu->reg[R12], cpu->reg[R13]);
    printf("  %%r14: %016lx\n",  cpu->reg[R14]);
}

/**********************************************************************
 *                         HELPER METHODS
 *********************************************************************/

//Print the current cpu state in text form
void printCpuState(y86_t *cpu) {
    switch(cpu->stat) {
        case(AOK): printf("AOK\n"); break;
        case(HLT): printf("HLT\n"); break;
        case(ADR): printf("ADR\n"); break;
        case(INS): printf("INS\n"); break;
    }
    return;
}

/*
Eagle Scout - November 2021 - Present
Conceptualized, fundraised, and implemented a ‘Catio’ for a local animal 
shelter so cats could spend time outside in a safe environment.
Led volunteers over the course of several months to complete the project.

*/