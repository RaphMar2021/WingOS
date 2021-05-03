#include <arch.h>
#include <com.h>
#include <device/apic.h>
#include <device/pit.h>
#include <kernel.h>
#include <logging.h>
ASM_FUNCTION uint32_t read_pit_counter();
void PIT::init()
{
    add_device(this);
    log("pit", LOG_DEBUG, "loading pit...");
    uint16_t divisor = PIT_START_FREQUENCY /
                       PIT_TARGET_FREQUECY; // to do : make this more portable

    outb(0x43, 0x36);

    uint8_t l = (uint8_t)(divisor & 0xFF);
    wait();
    outb(0x40, l);
    wait();
    uint8_t h = (uint8_t)((divisor >> 8) & 0xFF);
    outb(0x40, h);
    log("pit", LOG_DEBUG, "loaded pit");
}

void PIT::Pwait(uint16_t ms)
{
    outb(0x43, 0x30);
    uint16_t wait_val = PIT_START_FREQUENCY / (ms * 1000);
    uint8_t l = (uint8_t)(wait_val & 0xFF);
    uint8_t h = (uint8_t)((wait_val >> 8) & 0xFF);

    outb(0x40, l);
    outb(0x40, h);
    while (true)
    {
        if ((int64_t)read_pit_counter() == 0)
        {
            break;
        }
    }
}
bool loaded = false;
