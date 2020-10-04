#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>
#include <mutex>
#include "side_kick.h"
using namespace std;

extern mutex io_lock;
extern mutex memory_mutex[MUT_NUM];
extern int memory_mutex_address[MUT_NUM];

void data_pass_cmd(decoded_cmd cmd, registers* reg_set_ptr, char* memory)
{
	if(cmd.reg_num2 == 0)					//special case
		reg_set_ptr->ax[cmd.reg_num1] = cmd.data_num;
	else
		put_value(reg_set_ptr, memory, cmd.reg_num1, get_value(reg_set_ptr, memory, cmd.reg_num2));
}

void num_cal_cmd(decoded_cmd cmd, registers* reg_set_ptr, char* memory)
{
	short value1, value2;
	value1 = get_value(reg_set_ptr, memory, cmd.reg_num1);
	if(cmd.reg_num2 == 0)
		value2 = cmd.data_num;
	else
		value2 = get_value(reg_set_ptr, memory, cmd.reg_num2);

	if(cmd.oper_code == 2)												//add
		put_value(reg_set_ptr, memory, cmd.reg_num1, value1 + value2);
	else if(cmd.oper_code == 3)											//sub
		put_value(reg_set_ptr, memory, cmd.reg_num1, value1 - value2);
	else if(cmd.oper_code == 4)											//mul
		put_value(reg_set_ptr, memory, cmd.reg_num1, value1 * value2);
	else if(cmd.oper_code == 5)											//div
		put_value(reg_set_ptr, memory, cmd.reg_num1, value1 / value2);
}

void logic_cal_cmd(decoded_cmd cmd, registers* reg_set_ptr, char* memory)
{
	short value1, value2;
	value1 = get_value(reg_set_ptr, memory, cmd.reg_num1);
	if(cmd.reg_num2 == 0)
		value2 = cmd.data_num;
	else
		value2 = get_value(reg_set_ptr, memory, cmd.reg_num2);
	
	if(cmd.oper_code == 6)												//and
		put_value(reg_set_ptr, memory, cmd.reg_num1, value1 && value2);
	else if(cmd.oper_code == 7)											//or
		put_value(reg_set_ptr, memory, cmd.reg_num1, value1 || value2);
	else if(cmd.oper_code == 8)											//not(special case)
	{
		if(cmd.reg_num1 != 0)
			put_value(reg_set_ptr, memory, cmd.reg_num1, !value1);
		else if(cmd.reg_num2 != 0)
			put_value(reg_set_ptr, memory, cmd.reg_num2, !value2);
	}
}

void cmp_cmd(decoded_cmd cmd, registers* reg_set_ptr, char* memory)
{
	short value1, value2;
	value1 = get_value(reg_set_ptr, memory, cmd.reg_num1);
	if(cmd.reg_num2 == 0)
		value2 = cmd.data_num;
	else
		value2 = get_value(reg_set_ptr, memory, cmd.reg_num2);

	if(value1 == value2)
		reg_set_ptr->flag = 0;
	else if(value1 > value2)
		reg_set_ptr->flag = 1;
	else if(value1 < value2)
		reg_set_ptr->flag = -1;
}

bool jmp_cmd(decoded_cmd cmd, registers* reg_set_ptr, char* memory)
{
	bool jmped = true;
	if(cmd.reg_num2 == 0)
		reg_set_ptr->ip += cmd.data_num;
	else if(cmd.reg_num2 == 1 && reg_set_ptr->flag == 0)
		reg_set_ptr->ip += cmd.data_num;
	else if(cmd.reg_num2 == 2 && reg_set_ptr->flag == 1)
		reg_set_ptr->ip += cmd.data_num;
	else if(cmd.reg_num2 == 3 && reg_set_ptr->flag == -1)
		reg_set_ptr->ip += cmd.data_num;
	else
		jmped = false;
	return jmped;
}

void io_cmd(decoded_cmd cmd, registers* reg_set_ptr, char* memory)
{
	io_lock.lock();
	if(cmd.oper_code == 11)
	{
		short value;
		cout << "in:" << endl;
		cin >> value;
		put_value(reg_set_ptr, memory, cmd.reg_num1, value);
	}
	else if(cmd.oper_code == 12)
		cout << "id = " << reg_set_ptr->id << "    out: " << get_value(reg_set_ptr, memory, cmd.reg_num1) << endl;
	io_lock.unlock();
}

void mutex_lock_cmd(decoded_cmd cmd, registers* reg_set_ptr, char* memory)
{
	for(int i = 0; i < MUT_NUM; i++)
	{
		if(memory_mutex_address[i] == cmd.data_num)
		{
			memory_mutex[i].lock();
			break;
		}
		if(memory_mutex_address[i] == -1)
		{
			memory_mutex_address[i] = cmd.data_num;
			memory_mutex[i].lock();
			break;
		}
	}
}

void mutex_release_cmd(decoded_cmd cmd, registers* reg_set_ptr, char* memory)
{
	for(int i = 0; i < MUT_NUM; i++)
		if(memory_mutex_address[i] == cmd.data_num)
		{
			memory_mutex_address[i] == -1;
			memory_mutex[i].unlock();
			break;
		}
}

void slp_cmd(decoded_cmd cmd, registers* reg_set_ptr, char* memory)
{
	this_thread::sleep_for(chrono::milliseconds(cmd.data_num));
}

int bstr_to_int(char* bstr)
{
	int value = 0;
	for(int i = 0; i < REAL_BYTE; i++)
	{
		value <<= 1;
		if(bstr[i] == '1')
			value += 1;
	}
	return value;
}

int byte_to_int(char* start, int length)
{
	int value = 0;
	for(int i = 0; i < length; i++)
	{
		value <<= REAL_BYTE;
		value += start[i] & 0b11111111;
	}
	return value;
}

void int_to_byte(char* start, short num, int length)
{
	for(int i = length - 1; i >= 0; i--)
	{
		start[i] = num & 0b11111111;
		num >>= REAL_BYTE;
	}
}

short get_value(registers* reg_set_ptr, char* memory, int ax_num)
{
	if(ax_num <= 4)
		return reg_set_ptr->ax[ax_num];
	else //if(ax_num >= 5)
		return byte_to_int(memory + reg_set_ptr->ax[ax_num], WORD_SIZE);
}

void put_value(registers* reg_set_ptr, char* memory, int ax_num, int value)
{
	if(ax_num <= 4)
		reg_set_ptr->ax[ax_num] = value;
	else if(ax_num >= 5)
		int_to_byte(memory + reg_set_ptr->ax[ax_num], value, WORD_SIZE);
}

decoded_cmd::decoded_cmd(char* original_cmd)
{
	int cur_code = byte_to_int(original_cmd, CMD_LEN);
	oper_code = cur_code >> 24;
	reg_num1 = (cur_code >> 20) & 0b1111;
	reg_num2 = (cur_code >> 16) & 0b1111;
	data_num = cur_code & 0b1111111111111111;
}

registers::registers(int num)
{
	id = num;
	ip = 0;
	ir = 0;
	flag = 0;
	memset(ax,0,sizeof(ax));
}