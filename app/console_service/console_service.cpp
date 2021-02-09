#include <gui/graphic_system.h>
#include <gui/raw_graphic.h>
#include <gui/widget/button.h>
#include <gui/widget/rectangle.h>
#include <gui/window.h>
#include <kern/kernel_util.h>
#include <kern/mem_util.h>
#include <kern/process_message.h>
#include <kern/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
struct console_per_process
{
    char *data; // null terminated
    uint64_t length;
    uint64_t allocated_length;
    uint64_t pid;
    bool is_free_to_use = false;
};
console_per_process console_list[100];
void init_console()
{
    for (int i = 0; i < 100; i++)
    {
        console_list[i].data = nullptr;
        console_list[i].length = 0;
        console_list[i].is_free_to_use = true;
        console_list[i].pid = 0;
    }
}
void create_console(sys::raw_process_message *msg)
{
    for (int i = 0; i < 100; i++)
    {
        if (console_list[i].is_free_to_use)
        {
            printf("creating console for %x \n", msg->from_pid);
            console_list[i].data = (char *)malloc(100);
            memset(console_list[i].data, 0, 100);
            console_list[i].is_free_to_use = false;
            console_list[i].allocated_length = 100;
            console_list[i].length = 0;
            console_list[i].pid = msg->from_pid;
            return;
        }
    }
}
console_per_process *find(uint64_t pid)
{
    for (int i = 0; i < 100; i++)
    {
        if (!console_list[i].is_free_to_use)
        {
            if (console_list[i].pid == pid)
            {
                return &console_list[i];
            }
        }
    }
    return nullptr;
}
void increase_console(console_per_process *p, uint64_t added_length)
{
    if (p->allocated_length < p->length + added_length)
    {
        p->allocated_length += 512;
        p->data = (char *)realloc(p->data, p->allocated_length);
    }
    p->length += added_length;
}
void write(sys::raw_process_message *msg)
{
    sys::console_service_request *pr = (sys::console_service_request *)msg->content_address;
    console_per_process *target = find(msg->from_pid);
    if (target == nullptr)
    {
        create_console(msg);
        return write(msg);
    }
    uint64_t last_length = target->length;
    increase_console(target, strlen(pr->write.raw_data) + 1);
    memcpy((char *)((uint64_t)target->data + last_length), pr->write.raw_data, strlen(pr->write.raw_data) + 1);
    printf(target->data);
}
*/
int main()
{
    while (true)
    {
    }

    return 0;
}
