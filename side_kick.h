#ifndef SIDE_KICK_H 
#define SIDE_KICK_H
const int REAL_BYTE = 8;
const int SIZE = 32 * 1024;
const int BYTE = sizeof(char);
const int CMD_LEN = 4 * BYTE;
const int WORD_SIZE = 2 * BYTE;
const int TO_DATA_SEGMENT = 16384;
const int TO_NEXT_CORE = 256;
const int MUT_NUM = 10;

class registers
{
public:
    int id;
    short ip;
    short ir;
    short flag;
    short ax[9];
    registers(int num);
};

class decoded_cmd
{
public:
    int oper_code;
    int reg_num1;
    int reg_num2;
    short data_num;
    decoded_cmd(char* original_cmd);
};

void cmd_input(char* memory);													//main function
bool cmd_decode(registers* reg_set_ptr, char* memory, int core_id);
void shut_down_output(char* memory);
void register_state_output(registers* reg_set_ptr);	
void data_pass_cmd(decoded_cmd cmd, registers* reg_set_ptr, char* memory);		//command
void num_cal_cmd(decoded_cmd cmd, registers* reg_set_ptr, char* memory);
void logic_cal_cmd(decoded_cmd cmd, registers* reg_set_ptr, char* memory);
void cmp_cmd(decoded_cmd cmd, registers* reg_set_ptr, char* memory);
bool jmp_cmd(decoded_cmd cmd, registers* reg_set_ptr, char* memory);
void io_cmd(decoded_cmd cmd, registers* reg_set_ptr, char* memory);
void mutex_lock_cmd(decoded_cmd cmd, registers* reg_set_ptr, char* memory);
void mutex_release_cmd(decoded_cmd cmd, registers* reg_set_ptr, char* memory);
void slp_cmd(decoded_cmd cmd, registers* reg_set_ptr, char* memory);
int bstr_to_int(char* bstr);													//data convert
int byte_to_int(char* start, int length);
void int_to_byte(char* start, short num, int length);
short get_value(registers* reg_set_ptr, char* memory, int ax_num);				//register operation
void put_value(registers* reg_set_ptr, char* memory, int ax_num, int value);
void new_core(int core_id, char* memory);                                       //multithread function
#endif