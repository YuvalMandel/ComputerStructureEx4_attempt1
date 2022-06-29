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
        if(next_thread == current_thread){
            return -1;
        }
    }
    return next_thread;
}

int blocked_total_cycles;
int blocked_total_insts;
tcontext* blocked_tocntext_array;
int debug;

int fg_total_cycles;
int fg_total_insts;
tcontext* fg_tocntext_array;

int switch_cycles;
int store_latency;
int load_latency;

void CORE_BlockedMT() {

    switch_cycles = SIM_GetSwitchCycles();
    store_latency = SIM_GetStoreLat();
    load_latency = SIM_GetLoadLat();

    debug = 0;
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
    blocked_total_cycles = 0;
    blocked_total_insts = 0;
//    regs = (int**)malloc(sizeof(int*)*SIM_GetThreadsNum());
    blocked_tocntext_array = (tcontext*)malloc(sizeof(tcontext) * SIM_GetThreadsNum());
    for (int i = 0; i <  SIM_GetThreadsNum(); ++i) {
//        regs[i] = (int*)malloc(sizeof(int)*REGS_COUNT);
        for (int j = 0; j < REGS_COUNT; ++j) {
            blocked_tocntext_array[i].reg[j] = 0;
        }
    }
//    regs[SIM_GetThreadsNum()][REGS_COUNT];
    int thread_wait_time[SIM_GetThreadsNum()];
    for (int i = 0; i <  SIM_GetThreadsNum(); ++i) {
        thread_wait_time[i] = 0;
    }

    while(num_of_active_threads != 0){
        blocked_total_cycles += 1;
        bool good_current_thread = true;
        for (int i = 0; i <  SIM_GetThreadsNum(); ++i) {
            if(thread_wait_time[i] != 0){
                thread_wait_time[i]--;
            }else if(thread_status_array[i] == THREAD_HOLD){
                thread_status_array[i] = THREAD_ACTIVE;
            }
        }
        if(thread_status_array[current_thread] != THREAD_ACTIVE){
            int result = next_thread(current_thread, thread_status_array);
            if(result != -1){
                current_thread = result;
                debug++;
                blocked_total_cycles += switch_cycles;
                for (int i = 0; i <  SIM_GetThreadsNum(); ++i) {
                    if(thread_wait_time[i] != 0){
                        if(thread_wait_time[i] - switch_cycles < 0){
                            thread_wait_time[i] = 0;
                            thread_status_array[i] = THREAD_ACTIVE;
                        }else{
                            thread_wait_time[i] -=  switch_cycles;
                        }
                    }else if(thread_status_array[i] == THREAD_HOLD){
                        thread_status_array[i] = THREAD_ACTIVE;
                    }
                }
            }else{
                good_current_thread = false;
            }
        }
        if(good_current_thread == true){
            SIM_MemInstRead(current_line_array[current_thread], &current_inst, current_thread);
            blocked_total_insts++;
            switch (current_inst.opcode) {
                case CMD_NOP:
                    current_line_array[current_thread]++;
                    break;
                case CMD_ADD:
                    blocked_tocntext_array[current_thread].reg[current_inst.dst_index] =
                            blocked_tocntext_array[current_thread].reg[current_inst.src1_index] +
                            blocked_tocntext_array[current_thread].reg[current_inst.src2_index_imm];
                    current_line_array[current_thread]++;
                    break;
                case CMD_SUB:
                    blocked_tocntext_array[current_thread].reg[current_inst.dst_index] =
                            blocked_tocntext_array[current_thread].reg[current_inst.src1_index] -
                            blocked_tocntext_array[current_thread].reg[current_inst.src2_index_imm];
                    current_line_array[current_thread]++;
                    break;
                case CMD_ADDI:
                    blocked_tocntext_array[current_thread].reg[current_inst.dst_index] =
                            blocked_tocntext_array[current_thread].reg[current_inst.src1_index] + current_inst.src2_index_imm;
                    current_line_array[current_thread]++;
                    break;
                case CMD_SUBI:
                    blocked_tocntext_array[current_thread].reg[current_inst.dst_index] =
                            blocked_tocntext_array[current_thread].reg[current_inst.src1_index] - current_inst.src2_index_imm;
                    current_line_array[current_thread]++;
                    break;
                case CMD_LOAD:
                    if(current_inst.isSrc2Imm){
                        SIM_MemDataRead(blocked_tocntext_array[current_thread].reg[current_inst.src1_index] + current_inst.src2_index_imm,
                                        &( blocked_tocntext_array[current_thread].reg[current_inst.dst_index]));
                    }else{
                        SIM_MemDataRead(blocked_tocntext_array[current_thread].reg[current_inst.src1_index], &( blocked_tocntext_array[current_thread].reg[current_inst.dst_index]));
                    }
                    current_line_array[current_thread]++;
                    thread_status_array[current_thread] = THREAD_HOLD;
                    thread_wait_time[current_thread] = load_latency;
                    break;
                case CMD_STORE:
                    if(current_inst.isSrc2Imm){
                        SIM_MemDataWrite(blocked_tocntext_array[current_thread].reg[current_inst.dst_index] + current_inst.src2_index_imm,
                                         blocked_tocntext_array[current_thread].reg[current_inst.src1_index]);
                    }else {
                        SIM_MemDataWrite(blocked_tocntext_array[current_thread].reg[current_inst.src1_index],
                                         blocked_tocntext_array[current_thread].reg[current_inst.dst_index]);
                    }
                    current_line_array[current_thread]++;
                    thread_status_array[current_thread] = THREAD_HOLD;
                    thread_wait_time[current_thread] = store_latency;
                    break;
                case CMD_HALT:
                    thread_status_array[current_thread] = THREAD_FINISHED;
                    num_of_active_threads--;
                    break;
            }
        }
    }
}

void CORE_FinegrainedMT(){

    debug = 0;
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
    fg_total_cycles = 0;
    fg_total_insts = 0;
    fg_tocntext_array = (tcontext*)malloc(sizeof(tcontext) * SIM_GetThreadsNum());
    for (int i = 0; i <  SIM_GetThreadsNum(); ++i) {
        for (int j = 0; j < REGS_COUNT; ++j) {
            fg_tocntext_array[i].reg[j] = 0;
        }
    }
//    regs[SIM_GetThreadsNum()][REGS_COUNT];
    int thread_wait_time[SIM_GetThreadsNum()];
    for (int i = 0; i <  SIM_GetThreadsNum(); ++i) {
        thread_wait_time[i] = 0;
    }

    while(num_of_active_threads != 0){
//        bool good_current_thread = true;
        for (int i = 0; i <  SIM_GetThreadsNum(); ++i) {
            if(thread_wait_time[i] != 0){
                thread_wait_time[i]--;
            }else if(thread_status_array[i] == THREAD_HOLD){
                thread_status_array[i] = THREAD_ACTIVE;
            }
        }
//        if(thread_status_array[current_thread] != THREAD_ACTIVE) {
            int result = next_thread(current_thread, thread_status_array);
            if(result != -1){
                current_thread = result;
            }
//        }
        if(thread_status_array[current_thread] == THREAD_ACTIVE){
            SIM_MemInstRead(current_line_array[current_thread], &current_inst, current_thread);
            fg_total_insts++;
            switch (current_inst.opcode) {
                case CMD_NOP:
                    current_line_array[current_thread]++;
                    break;
                case CMD_ADD:
                    fg_tocntext_array[current_thread].reg[current_inst.dst_index] =
                            fg_tocntext_array[current_thread].reg[current_inst.src1_index] +
                            fg_tocntext_array[current_thread].reg[current_inst.src2_index_imm];
                    current_line_array[current_thread]++;
                    break;
                case CMD_SUB:
                    fg_tocntext_array[current_thread].reg[current_inst.dst_index] =
                            fg_tocntext_array[current_thread].reg[current_inst.src1_index] -
                            fg_tocntext_array[current_thread].reg[current_inst.src2_index_imm];
                    current_line_array[current_thread]++;
                    break;
                case CMD_ADDI:
                    fg_tocntext_array[current_thread].reg[current_inst.dst_index] =
                            fg_tocntext_array[current_thread].reg[current_inst.src1_index] + current_inst.src2_index_imm;
                    current_line_array[current_thread]++;
                    break;
                case CMD_SUBI:
                    fg_tocntext_array[current_thread].reg[current_inst.dst_index] =
                            fg_tocntext_array[current_thread].reg[current_inst.src1_index] - current_inst.src2_index_imm;
                    current_line_array[current_thread]++;
                    break;
                case CMD_LOAD:
                    if(current_inst.isSrc2Imm){
                        SIM_MemDataRead(fg_tocntext_array[current_thread].reg[current_inst.src1_index] + current_inst.src2_index_imm,
                                        &( fg_tocntext_array[current_thread].reg[current_inst.dst_index]));
                    }else{
                        SIM_MemDataRead(fg_tocntext_array[current_thread].reg[current_inst.src1_index], &( fg_tocntext_array[current_thread].reg[current_inst.dst_index]));
                    }
                    current_line_array[current_thread]++;
                    thread_status_array[current_thread] = THREAD_HOLD;
                    thread_wait_time[current_thread] = load_latency;
                    break;
                case CMD_STORE:
                    if(current_inst.isSrc2Imm){
                        SIM_MemDataWrite(fg_tocntext_array[current_thread].reg[current_inst.dst_index] + current_inst.src2_index_imm,
                                         fg_tocntext_array[current_thread].reg[current_inst.src1_index]);
                    }else {
                        SIM_MemDataWrite(fg_tocntext_array[current_thread].reg[current_inst.src1_index],
                                         fg_tocntext_array[current_thread].reg[current_inst.dst_index]);
                    }
                    current_line_array[current_thread]++;
                    thread_status_array[current_thread] = THREAD_HOLD;
                    thread_wait_time[current_thread] = store_latency;
                    break;
                case CMD_HALT:
                    thread_status_array[current_thread] = THREAD_FINISHED;
                    num_of_active_threads--;
                    break;
            }
        }
//        int result = next_thread(current_thread, thread_status_array);
//        if(result != -1){
//            current_thread = result;
//        }
        fg_total_cycles += 1;
    }
}

double CORE_BlockedMT_CPI(){
//    for (int i = 0; i < SIM_GetThreadsNum(); ++i) {
//        free( regs[i]);
//    }
    free(blocked_tocntext_array);
	return ((double)blocked_total_cycles) / ((double)blocked_total_insts);
}

double CORE_FinegrainedMT_CPI(){
    free(fg_tocntext_array);
    return ((double)fg_total_cycles) / ((double)fg_total_insts);
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
//    for (int i = 0; i < REGS_COUNT; ++i) {
//        context->reg[i] = regs[threadid][i];
//    }
    *(context+threadid) = blocked_tocntext_array[threadid];
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
    *(context+threadid) = fg_tocntext_array[threadid];
}
