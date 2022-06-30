/* 046267 Computer Architecture - Spring 2021 - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>

typedef enum {NOT_ACCESSED, LOAD, STORE} memoryState;

typedef struct thread_info_t {
	bool halted = false;
	uint32_t current_line = 0;
	memoryState memory_state = NOT_ACCESSED;
	int mem_access_ending_cycle = 0;
} thread_info;

tcontext *blocked_regs, *finegrained_regs;
double total_cycles_blocked = 0;
double total_cycles_finegrained = 0;
double total_instructions_blocked = 0;
double total_instructions_finegrained = 0;

//update nextThread & return updated Cycle
int contextSwitch(thread_info* threads, int threads_num, int previous_thread, int cycle, bool blocked,
					int* next_thread)
{
	while (true)
	{
		int current_thread = previous_thread;

		if (!blocked) //scan from next (don't give priority to the last thread that worked)
			current_thread++;

		for (int i = 0; i < threads_num; i++)
		{
			if (current_thread == threads_num)
				current_thread = 0;			
			if (!threads[current_thread].halted)
			{
				int switch_price = (!blocked || current_thread == previous_thread) ? 0 : SIM_GetSwitchCycles();
				if (threads[current_thread].memory_state == NOT_ACCESSED)
				{
					*next_thread = current_thread;
					return cycle + switch_price;
				}
				//if here then status is LOAD / STORE
				if (cycle >= threads[current_thread].mem_access_ending_cycle) 
				{
					threads[current_thread].memory_state = NOT_ACCESSED;
					*next_thread = current_thread;
					return cycle + switch_price;
				}
			}
			current_thread++;
		}
		//none of the threads is availble - go to next cycle
		cycle += 1;
	}
	return 0; //won't get here...
}

void CORE_BlockedMT() 
{
	int threads_num = SIM_GetThreadsNum();
	int halted_num = 0;
	blocked_regs = new tcontext[threads_num];
	thread_info* threads_arr = new thread_info[threads_num];
	Instruction* curr_inst = new Instruction;
	int curr_tid = 0;
	int cycle = 0;
	int operand1, operand2;
	bool all_threads_finished = false;

	while (!all_threads_finished)
	{
		SIM_MemInstRead(threads_arr[curr_tid].current_line, curr_inst, curr_tid);
		operand1 = blocked_regs[curr_tid].reg[curr_inst->src1_index];
		operand2 = curr_inst->isSrc2Imm ? curr_inst->src2_index_imm : blocked_regs[curr_tid].reg[curr_inst->src2_index_imm];
		threads_arr[curr_tid].current_line++;
		++cycle;

		switch (curr_inst->opcode)
		{
		case CMD_NOP:
			break;
        case CMD_ADD:
		case CMD_ADDI:
			blocked_regs[curr_tid].reg[curr_inst->dst_index] = operand1 + operand2;
            break;
        case CMD_SUB:
		case CMD_SUBI:
			blocked_regs[curr_tid].reg[curr_inst->dst_index] = operand1 - operand2;
			break;			
		case CMD_LOAD:
			SIM_MemDataRead(operand1 + operand2, blocked_regs[curr_tid].reg + curr_inst->dst_index);
			threads_arr[curr_tid].memory_state = LOAD;
			threads_arr[curr_tid].mem_access_ending_cycle = cycle + SIM_GetLoadLat();
			cycle = contextSwitch(threads_arr, threads_num, curr_tid, cycle, true, &curr_tid);
			break;
		case CMD_STORE:
			SIM_MemDataWrite(blocked_regs[curr_tid].reg[curr_inst->dst_index] + operand2, operand1);
			threads_arr[curr_tid].memory_state = STORE;
			threads_arr[curr_tid].mem_access_ending_cycle = cycle + SIM_GetStoreLat();
			cycle = contextSwitch(threads_arr, threads_num, curr_tid, cycle, true, &curr_tid);
			break;	
		case CMD_HALT:
			threads_arr[curr_tid].halted = true;
			if (++halted_num == threads_num)
				all_threads_finished = true;
			if (!all_threads_finished)
				cycle = contextSwitch(threads_arr, threads_num, curr_tid, cycle, true, &curr_tid);
			break;
		default:
			break;
		}

		total_instructions_blocked++;
	}
	total_cycles_blocked = cycle;

	delete curr_inst;
	delete[] threads_arr;
}

void CORE_FinegrainedMT() {
	int threads_num = SIM_GetThreadsNum();
	int halted_num = 0;
	finegrained_regs = new tcontext[threads_num];
	thread_info* threads_arr = new thread_info[threads_num];
	Instruction* curr_inst = new Instruction;
	int curr_tid = 0;
	int cycle = 0;
	int operand1, operand2;
	bool all_threads_finished = false;

	while (!all_threads_finished)
	{
		SIM_MemInstRead(threads_arr[curr_tid].current_line, curr_inst, curr_tid);
		operand1 = finegrained_regs[curr_tid].reg[curr_inst->src1_index];
		operand2 = curr_inst->isSrc2Imm ? curr_inst->src2_index_imm : finegrained_regs[curr_tid].reg[curr_inst->src2_index_imm];
		threads_arr[curr_tid].current_line++;
		++cycle;

		switch (curr_inst->opcode)
		{
		case CMD_NOP:
			break;
		case CMD_ADD:
		case CMD_ADDI:
			finegrained_regs[curr_tid].reg[curr_inst->dst_index] = operand1 + operand2;
			break;
		case CMD_SUB:	
		case CMD_SUBI:
			finegrained_regs[curr_tid].reg[curr_inst->dst_index] = operand1 - operand2;
			break;			
		case CMD_LOAD:
			SIM_MemDataRead(operand1 + operand2, finegrained_regs[curr_tid].reg + curr_inst->dst_index);		
			threads_arr[curr_tid].memory_state = LOAD;
			threads_arr[curr_tid].mem_access_ending_cycle = cycle + SIM_GetLoadLat();
			break;
		case CMD_STORE:
			SIM_MemDataWrite(finegrained_regs[curr_tid].reg[curr_inst->dst_index] + operand2, operand1);
			threads_arr[curr_tid].memory_state = STORE;
			threads_arr[curr_tid].mem_access_ending_cycle = cycle + SIM_GetStoreLat();
			break;
		case CMD_HALT:
			threads_arr[curr_tid].halted = true;
			if (++halted_num == threads_num)
				all_threads_finished = true;
			break;
		default:
			break;
		}
		total_instructions_finegrained++;	
		if (!all_threads_finished)
			cycle = contextSwitch(threads_arr, threads_num, curr_tid, cycle, false, &curr_tid);
	}
	total_cycles_finegrained = cycle;

	delete curr_inst;
	delete[] threads_arr;
}

double CORE_BlockedMT_CPI()
{
	//delete[] blocked_regs;
	return total_cycles_blocked / total_instructions_blocked;
}

double CORE_FinegrainedMT_CPI()
{
	//delete[] finegrained_regs;
	return total_cycles_finegrained / total_instructions_finegrained;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid)
{
	context[threadid] = blocked_regs[threadid];
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid)
{
	context[threadid] = finegrained_regs[threadid];
}