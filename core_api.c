/* 046267 Computer Architecture - Winter 20/21 - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>
#include <stdbool.h>

typedef enum {
    THREAD_ACTIVE,
    THREAD_HOLD,
    THREAD_FINISHED,
} thread_status;

int next_thread(int current_thread,thread_status* thread_status_array){

    int next_thread = current_thread + 1;
    if(next_thread >= SIM_GetThreadsNum()){
        next_thread = 0;
    }
    while(thread_status_array[next_thread] != THREAD_ACTIVE){
        next_thread++;
        if(next_thread >= SIM_GetThreadsNum()){
            next_thread = 0;
        }
        if(next_thread = current_thread){
            return -1;
        }
    }
    return next_thread;
}

int total_cycles;
int total_insts;
int** regs;
tcontext* tocntext_array;

void CORE_BlockedMT() {

    int num_of_active_threads = SIM_GetThreadsNum();
    thread_status thread_status_array[SIM_GetThreadsNum()];
    for (int i = 0; i < SIM_GetThreadsNum(); ++i) {
        thread_status_array[i] = THREAD_ACTIVE;
    }
    int current_thread = 0;
    int current_line_array[ SIM_GetThreadsNum()];
    for (int i = 0; i <  SIM_GetThreadsNum(); ++i) {
        current_line_array[i] = 0;
    }
    Instruction current_inst;
    total_cycles = 0;
    total_insts = 0;
//    regs = (int**)malloc(sizeof(int*)*SIM_GetThreadsNum());
    tocntext_array = (tcontext*)malloc(sizeof(tcontext)*SIM_GetThreadsNum());
    for (int i = 0; i <  SIM_GetThreadsNum(); ++i) {
//        regs[i] = (int*)malloc(sizeof(int)*REGS_COUNT);
        for (int j = 0; j < REGS_COUNT; ++j) {
            tocntext_array[i].reg[j] = 0;
        }
    }
//    regs[SIM_GetThreadsNum()][REGS_COUNT];
    int thread_wait_time[SIM_GetThreadsNum()];
    for (int i = 0; i <  SIM_GetThreadsNum(); ++i) {
        thread_wait_time[i] = 0;
    }

    while(num_of_active_threads != 0){
        total_cycles += 1;
        bool good_current_thread = true;
        for (int i = 0; i <  SIM_GetThreadsNum(); ++i) {
            if(thread_wait_time[i] != 0){
                thread_wait_time[i]--;
                if( thread_wait_time[i] == 0){
                    thread_status_array[i] = THREAD_ACTIVE;
                }
            }
        }
        if(thread_status_array[current_thread] != THREAD_ACTIVE){
            int result = next_thread(current_thread, thread_status_array);
            if(result != -1){
                current_thread = result;
                total_cycles += SIM_GetSwitchCycles();
            }else{
                good_current_thread = false;
            }
        }
        if(good_current_thread == true){
            SIM_MemInstRead(current_line_array[current_thread], &current_inst, current_thread);
            total_insts++;
            switch (current_inst.opcode) {
                case CMD_NOP:
                    current_line_array[current_thread]++;
                    break;
                case CMD_ADD:
                    tocntext_array[current_thread].reg[current_inst.dst_index] =
                            tocntext_array[current_thread].reg[current_inst.src1_index] +
                            tocntext_array[current_thread].reg[current_inst.src2_index_imm];
                    current_line_array[current_thread]++;
                    break;
                case CMD_SUB:
                    tocntext_array[current_thread].reg[current_inst.dst_index] =
                            tocntext_array[current_thread].reg[current_inst.src1_index] -
                            tocntext_array[current_thread].reg[current_inst.src2_index_imm];
                    current_line_array[current_thread]++;
                    break;
                case CMD_ADDI:
                    tocntext_array[current_thread].reg[current_inst.dst_index] =
                            tocntext_array[current_thread].reg[current_inst.src1_index] + current_inst.src2_index_imm;
                    current_line_array[current_thread]++;
                    break;
                case CMD_SUBI:
                    tocntext_array[current_thread].reg[current_inst.dst_index] =
                            tocntext_array[current_thread].reg[current_inst.src1_index] - current_inst.src2_index_imm;
                    current_line_array[current_thread]++;
                    break;
                case CMD_LOAD:
                    if(current_inst.isSrc2Imm){
                        SIM_MemDataRead( tocntext_array[current_thread].reg[current_inst.src1_index] + current_inst.src2_index_imm,
                                         &( tocntext_array[current_thread].reg[current_inst.dst_index]));
                    }else{
                        SIM_MemDataRead( tocntext_array[current_thread].reg[current_inst.src1_index],&( tocntext_array[current_thread].reg[current_inst.dst_index]));
                    }
                    current_line_array[current_thread]++;
                    thread_status_array[current_thread] = THREAD_HOLD;
                    thread_wait_time[current_thread] = SIM_GetLoadLat();
                    break;
                case CMD_STORE:
                    if(current_inst.isSrc2Imm){
                        SIM_MemDataWrite( tocntext_array[current_thread].reg[current_inst.dst_index] +  current_inst.src2_index_imm,
                                         tocntext_array[current_thread].reg[current_inst.src1_index]);
                    }else {
                        SIM_MemDataWrite(tocntext_array[current_thread].reg[current_inst.src1_index],
                                        tocntext_array[current_thread].reg[current_inst.dst_index]);
                    }
                    current_line_array[current_thread]++;
                    thread_status_array[current_thread] = THREAD_HOLD;
                    thread_wait_time[current_thread] = SIM_GetStoreLat();
                case CMD_HALT:
                    thread_status_array[current_thread] = THREAD_FINISHED;
                    num_of_active_threads--;
            }
        }
    }
}

void CORE_FinegrainedMT() {
}

double CORE_BlockedMT_CPI(){
//    for (int i = 0; i < SIM_GetThreadsNum(); ++i) {
//        free( regs[i]);
//    }
    free(tocntext_array);
	return ((double)total_cycles)/((double)total_insts);
}

double CORE_FinegrainedMT_CPI(){
	return 0;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
//    for (int i = 0; i < REGS_COUNT; ++i) {
//        context->reg[i] = regs[threadid][i];
//    }
    *(context+threadid) = tocntext_array[threadid];
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
}
