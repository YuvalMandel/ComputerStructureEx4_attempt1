/* 046267 Computer Architecture - Winter 20/21 - HW #4 */

//LIDOR ELFASI - 307854430
//DANIEL STUMLER - 318250032

#include "core_api.h"
#include "sim_api.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>


/*Typedefs:*/
typedef enum {IDLE,RUNNING,COMPLETED} STATE_E;
typedef struct {
    tcontext register_file;
    /*additional fields, fill whatever you need*/
    STATE_E thread_status;
    uint32_t current_inst_line;
    int thread_latency_counter;
}thread_info;

/*Global variables:*/

/*BLOCKED:*/
int number_of_threadsBLOCKED;
int number_of_clocksBLOCKED=0; // counts the number of clocks until every thread is COMPLETED.
int number_of_instructionsBLOCKED=0; //counts the number of instructions in the program.
int number_completed_threadsBLOCKED=0; //counts the number of completed threads.
int current_thread_idBLOCKED=0;
thread_info* threads_arrayBLOCKED=NULL;
/*FINE-GRAINED:*/
int number_of_threadsFINE;
int number_of_clocksFINE=0; // counts the number of clocks until every thread is COMPLETED.
int number_of_instructionsFINE=0; //counts the number of instructions in the program.
int number_completed_threadsFINE=0; //counts the number of completed threads.
int current_thread_idFINED=0;
thread_info* threads_arrayFINE=NULL;

/*functions:*/

/*this function gets threads_arrayBLOCKED and reset the instruction line and status of every cell in it.
 * (The size is a global variable)*/
void InitializesThread(thread_info* threadsArray){
    for(int i=0;i<number_of_threadsBLOCKED;i++){
        //threads_arrayBLOCKED[i]=*(thread_info*)malloc(sizeof(thread_info));
        for(int j=0;j<REGS_COUNT;j++){
            threadsArray[i].register_file.reg[j]=0;
        }
        threadsArray[i].thread_status=RUNNING;
        threadsArray[i].current_inst_line=0;
        threadsArray[i].thread_latency_counter=0;
    }
}
void updateLatency(thread_info* threadsArray){
    for (int i = 0;i<number_of_threadsBLOCKED;i++){
        if (threadsArray[i].thread_latency_counter > 0){
            threadsArray[i].thread_latency_counter--;
        }
        if (threadsArray[i].thread_latency_counter == 0 && threadsArray[i].thread_status==IDLE){
            threadsArray[i].thread_status = RUNNING;
        }
    }
}

bool IsAnyoneRunning(thread_info* threads){
    for(int i=0;i<number_of_threadsBLOCKED;i++){
        if(threads[i].thread_status==RUNNING){
            return true;
        }
    }
    return false;
}

int NextNotCompleted(int thread_id, thread_info* threadsArray){ //(current_thread_idBLOCKED,threads_arrayBLOCKED)
    int next_thread=thread_id+1;
    if(thread_id==number_of_threadsBLOCKED-1){
        next_thread=0;
    }
    while(next_thread!=thread_id){
        if(threadsArray[next_thread].thread_status!=COMPLETED){
            return next_thread;
        }
    }
    return next_thread; // NEVER GOING TO REACH HERE
}

/*this function gets current_thread_idBLOCKED, number_of_threadsBLOCKED and an array of thread_info structs and
 * returns the id of the next thread*/
int NextThreadId(int current_thread_id,thread_info* threadsArray) {

    while(!IsAnyoneRunning(threadsArray) ){
        //we need to stall one cycle
        number_of_clocksBLOCKED++;
        updateLatency(threadsArray);
    }
    if (threadsArray[current_thread_id].thread_status == RUNNING){
        return current_thread_id;
    }
    int thread_id_to_return = current_thread_id + 1;
    if (thread_id_to_return == number_of_threadsBLOCKED) {
        thread_id_to_return = 0;
    }
    while (thread_id_to_return != current_thread_id) {
        if (threadsArray[thread_id_to_return].thread_status == RUNNING) {
            return thread_id_to_return;
        }
        thread_id_to_return++;
        if (thread_id_to_return == number_of_threadsBLOCKED) {
            thread_id_to_return = 0;
        }
    }
    return thread_id_to_return;
}

int NextThreadIdFINE (int current_thread_id,thread_info* threadsArray){
    while(!IsAnyoneRunning(threadsArray) ){
        //we need to stall one cycle
        number_of_clocksFINE++;
        updateLatency(threadsArray);
    }
    int thread_id_to_return = current_thread_id + 1;
    if (thread_id_to_return == number_of_threadsBLOCKED) {
        thread_id_to_return = 0;
    }
    while (thread_id_to_return != current_thread_id) {
        if (threadsArray[thread_id_to_return].thread_status == RUNNING) {
            return thread_id_to_return;
        }
        thread_id_to_return++;
        if (thread_id_to_return == number_of_threadsBLOCKED) {
            thread_id_to_return = 0;
        }
    }
    return current_thread_id;
}

void PrintCurrentThread(int thread_id){
    thread_info current_thread=threads_arrayBLOCKED[thread_id];
    printf("Thread current_inst_line: %d \n",current_thread.current_inst_line);
    if(current_thread.thread_status==RUNNING){
        printf("thread_status is: RUNNING\n");
    }
    else{
        if(current_thread.thread_status==IDLE){
            printf("thread_status is: IDLE\n");
        }
        else{
            printf("thread_status is: COMPLETED\n");
        }
    }
    printf("thread_latency_counter is :%d \n",current_thread.thread_latency_counter);
} //TODO: make it generic by adding thread_info ptr instead of using the global variable blocked,,

void print_registers(thread_info current_thread){
    printf("Registers: \n");
    for(int i=0;i<REGS_COUNT;i++){
        printf("Reg[%d]=%d \n",i,current_thread.register_file.reg[i]);
    }
}

void CORE_BlockedMT() {
    //printf("CORE_BlockedMT is called! \n");
    number_of_threadsBLOCKED=SIM_GetThreadsNum();
    int load_latency=SIM_GetLoadLat();
    int store_latency=SIM_GetStoreLat();
    int context_switch_cycles=SIM_GetSwitchCycles();
    //printf("*** number of stalls (LW): %d , number of stalls (SW): %d , number of stalls (Context switch): %d *** \n",load_latency,store_latency,context_switch_cycles);
    threads_arrayBLOCKED=(thread_info*)malloc(sizeof(thread_info)*number_of_threadsBLOCKED);
//    if(threads_arrayBLOCKED==NULL){
//        printf("threads_arrayBLOCKED is null! \n");
//        return;
//    }
    /*Memory check: TODO: can't return -1 because the function is void! */
    //TODO: EVENTUALLY WILL HAVE TO INTIALIZE REG_FILE WITH SOME VALUE.
    InitializesThread(threads_arrayBLOCKED);
    while(number_completed_threadsBLOCKED<number_of_threadsBLOCKED) {
        Instruction current_inst;
        uint32_t addr;

        //printf("******************************* BEGIN OF THREAD SESSION \n");
       // printf("Thread ID is:  %d \n",current_thread_idBLOCKED);
        //PrintCurrentThread(current_thread_idBLOCKED);

        SIM_MemInstRead(threads_arrayBLOCKED[current_thread_idBLOCKED].current_inst_line, &current_inst, current_thread_idBLOCKED);
        //printf("The problem is probably not is SIM_MemInstRead! \n");
        /*now current_inst contains the instruction we about to execute*/
        switch (current_inst.opcode) {
            case CMD_NOP:
                //do nothing;
                break;
            case CMD_ADD:
                //dst <- src1 + src2
                threads_arrayBLOCKED[current_thread_idBLOCKED].register_file.reg[current_inst.dst_index] =
                        threads_arrayBLOCKED[current_thread_idBLOCKED].register_file.reg[current_inst.src1_index] +
                        threads_arrayBLOCKED[current_thread_idBLOCKED].register_file.reg[current_inst.src2_index_imm];
                //printf("The problem is not in CMD_ADD! \n");
                break;
            case CMD_SUB:
                //dst <- src1 - src2
                threads_arrayBLOCKED[current_thread_idBLOCKED].register_file.reg[current_inst.dst_index] =
                        threads_arrayBLOCKED[current_thread_idBLOCKED].register_file.reg[current_inst.src1_index] -
                        threads_arrayBLOCKED[current_thread_idBLOCKED].register_file.reg[current_inst.src2_index_imm];
                //printf("The problem is not in CMD_SUB! \n");
                break;
            case CMD_ADDI:
                //dst <- src1 + imm
                threads_arrayBLOCKED[current_thread_idBLOCKED].register_file.reg[current_inst.dst_index] =
                        threads_arrayBLOCKED[current_thread_idBLOCKED].register_file.reg[current_inst.src1_index] +
                                current_inst.src2_index_imm;
               // printf("The problem is not in CMD_ADDI! \n");
                break;
            case CMD_SUBI:
                // dst <- src1 - imm
                threads_arrayBLOCKED[current_thread_idBLOCKED].register_file.reg[current_inst.dst_index] =
                        threads_arrayBLOCKED[current_thread_idBLOCKED].register_file.reg[current_inst.src1_index] -
                                current_inst.src2_index_imm;
                break;
            case CMD_LOAD:
                //printf("We are inside load condition \n");
                // dst <- Mem[src1 + src2]  (src2 may be an immediate)
                //TODO: IMPORTANT! check pdf comment.
                 addr = threads_arrayBLOCKED[current_thread_idBLOCKED].register_file.reg[current_inst.src1_index]; //this part is fixed
               // printf("the problem is not when calculating addr LW 1 ! \n");
                if (current_inst.isSrc2Imm == false) {
                   // printf("current_inst.src2_index_imm is: %d \n",current_inst.src2_index_imm);
                    addr = addr + threads_arrayBLOCKED[current_thread_idBLOCKED].register_file.reg[current_inst.src2_index_imm];
                    //printf("the problem is not when calculating addr LW 2 ! \n");
                } else {
                   // printf("source 2 is: 0x%x \n",current_inst.src2_index_imm);
                    addr = addr + current_inst.src2_index_imm;
                }
                SIM_MemDataRead(addr, &threads_arrayBLOCKED[current_thread_idBLOCKED].register_file.reg[current_inst.dst_index]);
                //printf("The problem is not in SIM_MemDataRead! \n");
                threads_arrayBLOCKED[current_thread_idBLOCKED].thread_latency_counter=load_latency+1;
                threads_arrayBLOCKED[current_thread_idBLOCKED].thread_status=IDLE;
               //printf("The problem is not in CMD_LOAD! \n");
                break;
            case CMD_STORE:
                // Mem[dst + src2] <- src1  (src2 may be an immediate)
                //TODO: IMPORTANT! check pdf comment.
                 addr = threads_arrayBLOCKED[current_thread_idBLOCKED].register_file.reg[current_inst.dst_index]; //this part is fixed
                if (current_inst.isSrc2Imm == false) {
                    addr = addr + threads_arrayBLOCKED[current_thread_idBLOCKED].register_file.reg[current_inst.src2_index_imm];
                } else {
                    addr = addr + current_inst.src2_index_imm;
                }
                SIM_MemDataWrite(addr, threads_arrayBLOCKED[current_thread_idBLOCKED].register_file.reg[current_inst.src1_index]);
               // printf("The problem is not in SIM_MemDataWrite! \n");
                threads_arrayBLOCKED[current_thread_idBLOCKED].thread_latency_counter=store_latency+1;
                threads_arrayBLOCKED[current_thread_idBLOCKED].thread_status=IDLE;
               // printf("The problem is not in CMD_STORE! \n");
                break;
            case CMD_HALT:
                threads_arrayBLOCKED[current_thread_idBLOCKED].thread_status = COMPLETED;
                number_completed_threadsBLOCKED++;
               // printf("The problem is not in CMD_HALT! \n");
                break;
        }
        //print_registers(threads_arrayBLOCKED[current_thread_idBLOCKED]);
        number_of_instructionsBLOCKED++;
        number_of_clocksBLOCKED++;
        updateLatency(threads_arrayBLOCKED);
        threads_arrayBLOCKED[current_thread_idBLOCKED].current_inst_line++;
       // printf("The problem is not here!! \n");
        /*checks if the current thread can continue running:*/
        if(threads_arrayBLOCKED[current_thread_idBLOCKED].thread_status==RUNNING){
          //  printf("The problem is not either!! \n");
            //threads_arrayBLOCKED[current_thread_idBLOCKED].thread_latency_counter++;
            continue;
            /*we are assuming that eventually, by proceeding threads_arrayBLOCKED[current_thread_idBLOCKED].current_inst_line we'll
             * get HALT instruction*/
        }
        else{ /*the thread can not continue running and therefore, a context switch is required:*/
            if(number_completed_threadsBLOCKED==number_of_threadsBLOCKED){
                break;
            }
            //printf("******************************* END OF THREAD SESSION \n");
            //current_thread_idBLOCKED=NextThreadId(current_thread_idBLOCKED,threads_arrayBLOCKED);
            int temp=NextThreadId(current_thread_idBLOCKED,threads_arrayBLOCKED);
            if(temp!=current_thread_idBLOCKED){
                //printf("CONTEXT SWITCH! \n"); TODO: UNCOMMENT WHEN NEEDED
                number_of_clocksBLOCKED=number_of_clocksBLOCKED+context_switch_cycles;
                for(int i=0;i<context_switch_cycles;i++){
                    updateLatency(threads_arrayBLOCKED);
                }
            }
            current_thread_idBLOCKED=temp;
           // printf("The problem is not in NextThreadId! \n");
        }
    }
}

void CORE_FinegrainedMT() {

    number_of_threadsFINE=SIM_GetThreadsNum();
    int load_latency=SIM_GetLoadLat();
    int store_latency=SIM_GetStoreLat();
    threads_arrayFINE=(thread_info*)malloc(sizeof(thread_info)*number_of_threadsFINE);
    /*Memory check: TODO: can't return -1 because the function is void! */
    //TODO: EVENTUALLY WILL HAVE TO INTIALIZE REG_FILE WITH SOME VALUE.
    InitializesThread(threads_arrayFINE);
    while(number_completed_threadsFINE<number_of_threadsFINE) {
        Instruction current_inst;
        uint32_t addr;

        //printf("******************************* BEGIN OF THREAD SESSION \n");
        //printf("Thread ID is:  %d \n",current_thread_idFINED);
        //PrintCurrentThread(current_thread_idFINED);
        SIM_MemInstRead(threads_arrayFINE[current_thread_idFINED].current_inst_line, &current_inst, current_thread_idFINED);
        //printf("The problem is probably not is SIM_MemInstRead! \n");
        /*now current_inst contains the instruction we about to execute*/
        switch (current_inst.opcode) {
            case CMD_NOP:
                //do nothing;
                break;
            case CMD_ADD:
                //dst <- src1 + src2
                threads_arrayFINE[current_thread_idFINED].register_file.reg[current_inst.dst_index] =
                        threads_arrayFINE[current_thread_idFINED].register_file.reg[current_inst.src1_index] +
                        threads_arrayFINE[current_thread_idFINED].register_file.reg[current_inst.src2_index_imm];
                //printf("The problem is not in CMD_ADD! \n");
                break;
            case CMD_SUB:
                //dst <- src1 - src2
                threads_arrayFINE[current_thread_idFINED].register_file.reg[current_inst.dst_index] =
                        threads_arrayFINE[current_thread_idFINED].register_file.reg[current_inst.src1_index] -
                        threads_arrayFINE[current_thread_idFINED].register_file.reg[current_inst.src2_index_imm];
                //printf("The problem is not in CMD_SUB! \n");
                break;
            case CMD_ADDI:
                //dst <- src1 + imm
                threads_arrayFINE[current_thread_idFINED].register_file.reg[current_inst.dst_index] =
                        threads_arrayFINE[current_thread_idFINED].register_file.reg[current_inst.src1_index] +
                        current_inst.src2_index_imm;
                // printf("The problem is not in CMD_ADDI! \n");
                break;
            case CMD_SUBI:
                // dst <- src1 - imm
                threads_arrayFINE[current_thread_idFINED].register_file.reg[current_inst.dst_index] =
                        threads_arrayFINE[current_thread_idFINED].register_file.reg[current_inst.src1_index] -
                        current_inst.src2_index_imm;
                break;
            case CMD_LOAD:
                //printf("We are inside load condition \n");
                // dst <- Mem[src1 + src2]  (src2 may be an immediate)
                //TODO: IMPORTANT! check pdf comment.
                addr = threads_arrayFINE[current_thread_idFINED].register_file.reg[current_inst.src1_index]; //this part is fixed
                // printf("the problem is not when calculating addr LW 1 ! \n");
                if (current_inst.isSrc2Imm == false) {
                    // printf("current_inst.src2_index_imm is: %d \n",current_inst.src2_index_imm);
                    addr = addr + threads_arrayFINE[current_thread_idFINED].register_file.reg[current_inst.src2_index_imm];
                    //printf("the problem is not when calculating addr LW 2 ! \n");
                } else {
                    // printf("source 2 is: 0x%x \n",current_inst.src2_index_imm);
                    addr = addr + current_inst.src2_index_imm;
                }
                SIM_MemDataRead(addr, &threads_arrayFINE[current_thread_idFINED].register_file.reg[current_inst.dst_index]);
                //printf("The problem is not in SIM_MemDataRead! \n");
                threads_arrayFINE[current_thread_idFINED].thread_latency_counter=load_latency+1;
                threads_arrayFINE[current_thread_idFINED].thread_status=IDLE;
                //printf("The problem is not in CMD_LOAD! \n");
                break;
            case CMD_STORE:
                // Mem[dst + src2] <- src1  (src2 may be an immediate)
                //TODO: IMPORTANT! check pdf comment.
                addr = threads_arrayFINE[current_thread_idFINED].register_file.reg[current_inst.dst_index]; //this part is fixed
                if (current_inst.isSrc2Imm == false) {
                    addr = addr + threads_arrayFINE[current_thread_idFINED].register_file.reg[current_inst.src2_index_imm];
                } else {
                    addr = addr + current_inst.src2_index_imm;
                }
                SIM_MemDataWrite(addr, threads_arrayFINE[current_thread_idFINED].register_file.reg[current_inst.src1_index]);
                // printf("The problem is not in SIM_MemDataWrite! \n");
                threads_arrayFINE[current_thread_idFINED].thread_latency_counter=store_latency+1;
                threads_arrayFINE[current_thread_idFINED].thread_status=IDLE;
                // printf("The problem is not in CMD_STORE! \n");
                break;
            case CMD_HALT:
                threads_arrayFINE[current_thread_idFINED].thread_status = COMPLETED;
                number_completed_threadsFINE++;
                // printf("The problem is not in CMD_HALT! \n");
                break;
        }
       // print_registers(threads_arrayFINE[current_thread_idFINED]);
        number_of_instructionsFINE++;
        number_of_clocksFINE++;
        updateLatency(threads_arrayFINE);
        threads_arrayFINE[current_thread_idFINED].current_inst_line++;
        // printf("The problem is not here!! \n");
        /*checks if the current thread can continue running:*/
            if(number_completed_threadsFINE==number_of_threadsFINE){
                break;
            }
            //printf("******************************* END OF THREAD SESSION \n");
            //current_thread_idFINED=NextThreadId(current_thread_idFINED,threads_arrayFINE);
            current_thread_idFINED=NextThreadIdFINE(current_thread_idFINED,threads_arrayFINE); //TODO: CREATE A NEW FUNCTION
            // printf("The problem is not in NextThreadId! \n");
        }
    }

double CORE_BlockedMT_CPI(){
    if(threads_arrayBLOCKED!=NULL){
        free((threads_arrayBLOCKED));
    }
    if(number_of_instructionsBLOCKED==0){
        return 0;
    }
    double temp=(double)number_of_clocksBLOCKED;
    return temp/number_of_instructionsBLOCKED;
}

double CORE_FinegrainedMT_CPI(){
    if(threads_arrayFINE!=NULL){
        free((threads_arrayFINE));
    }
    if(number_of_instructionsFINE==0){
        return 0;
    }
    double temp=(double)number_of_clocksFINE;
    return temp/number_of_instructionsFINE;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
    for(int i=0;i<REGS_COUNT;i++){
        context[threadid].reg[i]=threads_arrayBLOCKED[threadid].register_file.reg[i]; //TODO: DANIEL HAS CHANGED IT
    }
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
    for(int i=0;i<REGS_COUNT;i++){
        context[threadid].reg[i]=threads_arrayFINE[threadid].register_file.reg[i];
    }
}
