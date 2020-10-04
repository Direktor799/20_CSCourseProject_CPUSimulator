#include <iostream>
#include <fstream>
#include <cstring>
#include <thread>
#include <mutex>
#include "side_kick.h"
using namespace std;

mutex io_lock;
mutex memory_mutex[MUT_NUM];
int memory_mutex_address[MUT_NUM];

int main()
{
	char memory[SIZE];
	memset(memory, 0, sizeof(memory));
	for(int i = 0; i < MUT_NUM; i++)
		memory_mutex_address[i] = -1;
	int_to_byte(memory + 16384, 100, WORD_SIZE);
	thread core1(new_core, 1, memory);
	thread core2(new_core, 2, memory);
	core1.join();
	core2.join();
	shut_down_output(memory);
}

void new_core(int core_id, char* memory)
{
	registers reg_set(core_id);
	bool running = true;
	cmd_input(memory + TO_NEXT_CORE * (core_id - 1));
	while(running)
	{
		running = cmd_decode(&reg_set, memory, core_id);
	 	register_state_output(&reg_set);
	}
}

void cmd_input(char* memory)
{
	char tmp[REAL_BYTE];
	ifstream fp("dict.dic");
	for(int i = 0; i < SIZE / CMD_LEN; i++)					//max line num
	{
		bool end_flag = true;
		for(int j = 0; j < CMD_LEN; j++)					//one line
		{
			for(int k = 0; k < REAL_BYTE; k++)				//one byte
				fp >> tmp[k];
			memory[i*CMD_LEN+j] = bstr_to_int(tmp);
			if(memory[i*CMD_LEN+j] != 0)					//test if it's shutdown cmd
				end_flag = false;
		}
		if(end_flag)
			break;
	}
}

bool cmd_decode(registers* reg_set_ptr, char* memory, int core_id)
{
	bool running = true, jmped = false;
	decoded_cmd cur_cmd(memory + TO_NEXT_CORE * (core_id - 1) + BYTE * reg_set_ptr->ip);						//decode cmd(using construct function)
	reg_set_ptr->ir = byte_to_int(memory + TO_NEXT_CORE * (core_id - 1) + BYTE * reg_set_ptr->ip, WORD_SIZE);	//update ir
	if(cur_cmd.oper_code == 0)
		running = false;
	else if(cur_cmd.oper_code == 1)
		data_pass_cmd(cur_cmd, reg_set_ptr, memory);
	else if(cur_cmd.oper_code >= 2 && cur_cmd.oper_code <= 5)
		num_cal_cmd(cur_cmd, reg_set_ptr, memory);
	else if(cur_cmd.oper_code >= 6 && cur_cmd.oper_code <= 8)
		logic_cal_cmd(cur_cmd, reg_set_ptr, memory);
	else if(cur_cmd.oper_code == 9)
		cmp_cmd(cur_cmd, reg_set_ptr, memory);
	else if(cur_cmd.oper_code == 10)
		jmped = jmp_cmd(cur_cmd, reg_set_ptr, memory);
	else if(cur_cmd.oper_code >= 11 && cur_cmd.oper_code <= 12)
		io_cmd(cur_cmd, reg_set_ptr, memory);
	else if(cur_cmd.oper_code == 13)
		mutex_lock_cmd(cur_cmd, reg_set_ptr, memory);
	else if(cur_cmd.oper_code == 14)
		mutex_release_cmd(cur_cmd, reg_set_ptr, memory);
	else if(cur_cmd.oper_code == 15)
		slp_cmd(cur_cmd, reg_set_ptr, memory);
	if(!jmped)
		reg_set_ptr->ip += 4;
	return running;
}

void shut_down_output(char* memory)
{
	cout << endl << "codeSegment :" << endl;
	for(int i = 0; i < 16 ; i++)
		for(int j = 0; j < 8; j++)
		{
			cout << byte_to_int(memory + 8 * CMD_LEN * i + CMD_LEN * j, CMD_LEN);
			(j == 7) ? (cout << endl) : (cout << " ");
		}
	cout << endl << "dataSegment :" << endl;
	for(int i = 0; i < 16 ; i++)
		for(int j = 0; j < 16; j++)
		{
			cout << short(byte_to_int(memory + TO_DATA_SEGMENT + 16 * WORD_SIZE * i + WORD_SIZE * j, WORD_SIZE));
			(j == 15) ? (cout << endl) : (cout << " ");
		}
}

void register_state_output(registers* reg_set_ptr)
{
	io_lock.lock();
	cout << "id = " << reg_set_ptr->id << endl;
	cout << "ip = " << reg_set_ptr->ip << endl;
	cout << "flag = " << reg_set_ptr->flag << endl;
	cout << "ir = " << reg_set_ptr->ir << endl;
	for(int i = 1; i <= 8; i++)
	{
		cout <<	"ax" << i << " = " << reg_set_ptr->ax[i];
		(i % 4 == 0) ? (cout << endl) : (cout << " ");
	}
	io_lock.unlock();
}