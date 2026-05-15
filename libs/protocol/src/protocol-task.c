#include "protocol-task.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static api_t* task_api = NULL;

void protocol_task_init(api_t* api)
{
    task_api = api;
}

void protocol_task_handle(char* command_string)
{
    if (command_string == NULL || task_api == NULL) return;
    
    char* space = strchr(command_string, ' ');
    char* cmd = command_string;
    char* args = "";
    
    if (space != NULL)
    {
        *space = '\0';
        args = space + 1;
    }
    
    for (int i = 0; task_api[i].command_name != NULL; i++)
    {
        if (strcmp(cmd, task_api[i].command_name) == 0)
        {
            task_api[i].callback(args);
            return;
        }
    }
    
    printf("Unknown command: %s\n", cmd);
}