#pragma once

typedef struct
{
    const char* command_name;
    void (*callback)(const char* args);
    const char* command_help;
} api_t;

void protocol_task_init(api_t* api);
void protocol_task_handle(char* command_string);