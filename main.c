/*
 * CS 261: Main driver
 *
 * Name: Ben Berry
 */

#include "p1-check.h"
#include "p2-load.h"
#include "p3-disas.h"
#include "p4-interp.h"

int main (int argc, char **argv)
{
    //Initialize flags.
    char *filename = NULL;
    bool print_header = false; 
    bool print_phdrs = false;
    bool print_membrief = false;
    bool print_memfull = false;
    bool disas_code = false;
    bool disas_data = false;
    bool exec_normal = false;
    bool exec_debug = false;

    //Parse command line arguments
     if(!parse_command_line_p4 ( argc,  argv, &print_header,  &print_phdrs, &print_membrief,  
     &print_memfull, &disas_code,  &disas_data, &exec_normal,  &exec_debug,  &filename))
        return EXIT_FAILURE;

    //Open file
    FILE *input = fopen(filename, "r");

    //Initialize an elf struct to store the header.
    struct elf hdr;

    bool headerRead = read_header(input, &hdr);
    if(!headerRead){
        printf("Failed to read file\n");
        return EXIT_FAILURE;
    }

    //Initialize an array with size equal to the number of program headers for program headers.
    struct elf_phdr phdrs[hdr.e_num_phdr]; 

    //Read each program header into the struct.
    for(int i = 0; i < hdr.e_num_phdr; i++)  {
        int offset = hdr.e_phdr_start + (i * sizeof(elf_phdr_t));   
        if(!(read_phdr(input, offset, &phdrs[i]))){
	        printf("Failed to read file\n");
            return EXIT_FAILURE;
        }
    }

    //Create "virtual memory" in the heap.
    byte_t* memory = (byte_t*) calloc(MEMSIZE, sizeof(byte_t));  
    //Load each segment into the allocated memory.
    for(int i = 0; i < hdr.e_num_phdr; i++) {
        if(!(load_segment(input, memory, &phdrs[i]))) {
	        printf("Failed to read file\n");
            free(memory);
            return EXIT_FAILURE;
        }
    }
    //Print output based on what flags are set.
    //Note that print_memfull and print_membrief cannot be active at the same time.
    if(print_header)
        dump_header(&hdr); 
    if(print_phdrs)
        dump_phdrs(hdr.e_num_phdr, phdrs);
    if(print_memfull)
        dump_memory(memory, 0, MEMSIZE); 
    else if(print_membrief) {
        for(int i = 0; i < hdr.e_num_phdr; i++)
            dump_memory(memory, phdrs[i].p_vaddr, phdrs[i].p_vaddr + phdrs[i].p_size); 
    }
    //Disassemble code
    int header;
    if(disas_code) {
        printf("Disassembly of executable contents:\n");
        for(header = 0; header < hdr.e_num_phdr; header++) {
            //Print each code header
            if(phdrs[header].p_type == CODE)
                disassemble_code(memory, &phdrs[header], &hdr);
        }  
    }
    //Disassemble data or rodata if applicable
    if(disas_data) {
        printf("Disassembly of data contents:\n");
      for(header = 0; header < hdr.e_num_phdr; header++) {
            //Disassemble data
            if(phdrs[header].p_type == DATA && phdrs[header].p_flags == 6)
                 disassemble_data(memory, &phdrs[header]);  
            //Disassemble rodata
            else if(phdrs[header].p_type == DATA && phdrs[header].p_flags == 4)
                disassemble_rodata(memory, &phdrs[header]);
      }  
    }
    //Initialize some variables needed for p4
    y86_t cpu;
    memset(&cpu, 0x00, sizeof(cpu));
    cpu.pc = hdr.e_entry;
    cpu.stat = AOK;
    int numInstructions = 0;
    bool cnd = false; 
    y86_reg_t valA = 0;
    y86_reg_t valE = 0;
    y86_inst_t ins;
    
    //Normal execution, cpu state printed only after cpu status changes from AOK
    if(exec_normal) {
        printf("Beginning execution at 0x%04x\n", hdr.e_entry);
        while(cpu.stat == AOK) {
            //Fetch
            ins = fetch(&cpu, memory);
            //Disinclude invalid instructions in total count
            if(cpu.stat == INS)
                numInstructions --;
            cnd = false;
            //Decode and execute
            valE = decode_execute(&cpu, ins, &cnd, &valA);
            //Memory, writeback, program counter increment
            memory_wb_pc(&cpu, ins, memory, cnd, valA, valE);
            numInstructions++;
            //Change cpu status to ADR if program counter leaves allocated memory
            if(cpu.pc >= MEMSIZE)
                cpu.stat = ADR;
        }
        //Update program counter if bad address was given
        if(cpu.stat == ADR){
            cpu.pc = ins.valP;
            //Increment by 1 extra for failed call intructions
            if(ins.icode == CALL)
                cpu.pc ++;
        }
        //Print final state of cpu
        dump_cpu_state(&cpu);
        printf("Total execution count: %d\n", numInstructions);
    }

    //Debug execution, print cpu state after each intruction
    //and print full memory dump after execution
    if(exec_debug) {
        printf("Beginning execution at 0x%04x\n", hdr.e_entry);
        while(cpu.stat == AOK) {
            //Print current cpu state
            dump_cpu_state(&cpu);
            //fetch
            ins = fetch(&cpu, memory);
            //Handle invalid instructions
            if(cpu.stat == INS){
                printf("\nInvalid instruction at 0x%04lx\n", cpu.pc);
                numInstructions --;
            //Otherwise, continue with diassembly
            } else {
                printf("\nExecuting: ");  
                disassemble(&ins); 
                printf("\n");
            }
            cnd = false;
            //Decode and execute
            valE = decode_execute(&cpu, ins, &cnd, &valA);
            //memory, writeback, program counter increment
            memory_wb_pc(&cpu, ins, memory, cnd, valA, valE);
            numInstructions++;
            //Change cpu status to ADR if program counter leaves allocated memory
            if(cpu.pc >= MEMSIZE)
                cpu.stat = ADR;
        }
        if(cpu.stat == ADR)
            cpu.pc = ins.valP;
        //Dump final cpu state
        dump_cpu_state(&cpu);
        printf("Total execution count: %d\n\n", numInstructions);
        //Dump full memory
        dump_memory(memory, 0, MEMSIZE); 
    }
    //Free allocated memory to prevent memory leaks.
    free(memory);
    return EXIT_SUCCESS;
}

