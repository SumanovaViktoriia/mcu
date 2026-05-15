#include "stdio-task.h"
#include <stdio.h>
#include "pico/stdlib.h"

#define COMMAND_BUF_LEN 128

static char command[COMMAND_BUF_LEN] = {0};
static int command_buf_idx = 0;

void stdio_task_init(void)
{
    command_buf_idx = 0;
    command[0] = '\0';
}

char* stdio_task_handle(void)
{
    int symbol = getchar_timeout_us(0);
    if (symbol == PICO_ERROR_TIMEOUT)
    {
        return NULL;
    }
    
    if (symbol == '\r' || symbol == '\n')
    {
        command[command_buf_idx] = '\0';
        command_buf_idx = 0;
        
        if (command[0] != '\0')
        {
            return command;
        }
        return NULL;
    }
    
    if (command_buf_idx >= COMMAND_BUF_LEN - 1)
    {
        command_buf_idx = 0;
        command[0] = '\0';
        return NULL;
    }
    
    command[command_buf_idx] = (char)symbol;
    command_buf_idx++;
    return NULL;
}