
#include <kern/mouse_keyboard.h>
#include <kern/process_message.h>
#include <kern/syscall.h>
#include <stdio.h>
namespace sys
{
    int32_t get_mouse_x()
    {
        int32_t result = sys$get_process_global_data(0, "ps2_device_service");
        if (result < 0)
        {
            result *= -1;
        }
        return result;
    }

    int32_t get_mouse_y()
    {
        int32_t result = sys$get_process_global_data(sizeof(uint64_t), "ps2_device_service");
        if (result < 0)
        {
            result *= -1;
        }
        return result;
    }

    bool get_mouse_button(int button_id)
    {
        uint64_t result = sys$get_process_global_data(sizeof(uint64_t) * (button_id + 2), "ps2_device_service");
        return (bool)result;
    }

    char get_last_key_press()
    {
        ps2_device_request request = {0};
        request.device_target = TARGET_KEYBOARD;
        request.request_type = GET_KEYBOARD_KEY;
        request.get_key_down.unused = true;
        service_message proc_msg = service_message("ps2_device_service", (uint64_t)&request, sizeof(ps2_device_request));
        return proc_msg.read();
    }
} // namespace sys
