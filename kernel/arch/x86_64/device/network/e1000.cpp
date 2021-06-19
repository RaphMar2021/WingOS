#include <arch.h>
#include <device/debug/com.h>
#include <device/network/e1000.h>
#include <interrupt.h>
#include <logging.h>
#include <mem/virtual.h>
#include <utils/memory/liballoc.h>
e1000 main_e1000;

e1000::e1000()
{
    packet_reception_list.clear();
}

e1000 *e1000::the()
{
    return &main_e1000;
}

void e1000::update()
{
}

void e1000::write(uint16_t addr, uint32_t val)
{
    if (bar_t == 0)
    {
        (*(uint32_t volatile *)(((char *)mm_address) + (addr))) = val;
    }
    else
    {
        outl(io_base_addr, addr);
        outl(io_base_addr + 4, val);
    }
}

uint32_t e1000::read(uint16_t addr)
{
    if (bar_t == 0)
    {
        return (*(uint32_t volatile *)(((char *)mm_address) + (addr)));
    }
    else
    {
        outl(io_base_addr, addr);
        return inl(io_base_addr + 4);
    }
}

bool e1000::eerp_rom_detection()
{
    uint32_t val = 0;
    write(E_EEPROM, 0x1);
    does_eerprom_exists = false;
    for (int i = 0; i < 1000 && !does_eerprom_exists; i++)
    {
        val = read(E_EEPROM);
        if (val & 0x10)
        {
            does_eerprom_exists = true;
        }
        else
        {
            does_eerprom_exists = false;
        }
    }
    return does_eerprom_exists;
}

uint32_t e1000::errp_rom_read(uint8_t addr)
{
    uint16_t data = 0;
    uint32_t tmp = 0;

    if (does_eerprom_exists)
    {
        write(E_EEPROM, (1) | ((uint32_t)(addr) << 8));
        while (!((tmp = read(E_EEPROM)) & (1 << 4)))
            ;
    }

    else
    {
        write(E_EEPROM, (1) | ((uint32_t)(addr) << 2));
        while (!((tmp = read(E_EEPROM)) & (1 << 1)))
            ;
    }

    data = (uint16_t)((tmp >> 16) & 0xFFFF);
    return data;
}

bool e1000::mac_detection()
{
    if (does_eerprom_exists)
    {

        uint32_t temp;

        temp = errp_rom_read(0);
        maddr.mac[0] = temp & 0xff;
        maddr.mac[1] = temp >> 8;

        temp = errp_rom_read(1);
        maddr.mac[2] = temp & 0xff;
        maddr.mac[3] = temp >> 8;

        temp = errp_rom_read(2);
        maddr.mac[4] = temp & 0xff;
        maddr.mac[5] = temp >> 8;
        return true;
    }
    else
    {
        uint8_t *base_8 = (uint8_t *)(mm_address + 0x5400);

        for (int i = 0; i < 6; i++)
        {
            maddr.mac[i] = base_8[i];
        }
        return true;
    }
    return true;
}

void e1000::setup_rx()
{
    uint8_t *tmp_ptr;
    rx_desc *descriptor;
    tmp_ptr = (uint8_t *)malloc(sizeof(rx_desc) * RX_DESCRIPTOR_COUNT + sizeof(rx_desc));
    descriptor = (rx_desc *)tmp_ptr;

    for (int i = 0; i < RX_DESCRIPTOR_COUNT; i++)
    {
        rx_descriptor[i] = (rx_desc *)((uint8_t *)descriptor + i * sizeof(rx_desc));
        rx_descriptor[i]->address = (uint64_t)(uint8_t *)(malloc(8192 + sizeof(rx_desc)));
        rx_descriptor[i]->status = 0;
    }

    write(E_TX_DESCRIPTOR_LOW, (uint64_t)tmp_ptr >> 32);
    write(E_TX_DESCRIPTOR_HIGH, (uint64_t)tmp_ptr & 0xFFFFFFFF);
    write(E_RX_DESCRIPTOR_LOW, (uint64_t)tmp_ptr);

    write(E_RX_DESCRIPTOR_HIGH, 0);

    write(E_RX_DESCRIPTOR_LENGTH, RX_DESCRIPTOR_COUNT * sizeof(rx_desc));
    write(E_RX_DESCRIPTOR_HEAD, 0);
    write(E_RX_DESCRIPTOR_TAIL, RX_DESCRIPTOR_COUNT - 1);

    rx_current_buf = 0;
    write(E_RCONTROL, (ENABLE) | (STORE_BAD_PACKET) | (UNICAST_ENABLE) | (MULTICAST_ENABLE) | NO_LOOPBACK | THRESHOLD | BROADCAST_ACCEPT | (STRIP_ETHERNET) | RCONTROL_SIZE_8192);
}

void e1000::setup_tx()
{

    uint8_t *tmp_ptr;
    tx_desc *descriptor;
    tmp_ptr = (uint8_t *)malloc(sizeof(tx_desc) * TX_DESCRIPTOR_COUNT + sizeof(tx_desc));
    descriptor = (tx_desc *)tmp_ptr;
    for (int i = 0; i < TX_DESCRIPTOR_COUNT; i++)
    {
        tx_descriptor[i] = (tx_desc *)((uint8_t *)descriptor + i * sizeof(tx_desc));
        tx_descriptor[i]->address = 0;
        tx_descriptor[i]->command = 0;
        tx_descriptor[i]->status = DESC_DONE;
    }

    write(E_TX_DESCRIPTOR_LOW, (uint64_t)tmp_ptr >> 32);
    write(E_TX_DESCRIPTOR_HIGH, (uint64_t)tmp_ptr & 0xFFFFFFFF);

    write(E_TX_DESCRIPTOR_LENGTH, TX_DESCRIPTOR_COUNT * sizeof(tx_desc));

    write(E_TX_DESCRIPTOR_HEAD, 0);
    write(E_TX_DESCRIPTOR_TAIL, 0);

    tx_current_buf = 0;

    write(E_TCONTROL, TRANSMIT_ON | PAD_SHORT_PACKET | (15 << COLLISION_THRESHOLD) | (64 << COLLISION_DISTANCE) | RE_SEND_ON_LATE);
    write(E_TCONTROL, 0b0110000000000111111000011111010);
    write(E_TRANSMIT_INTER_PACKET_GAP, 0x0060200A);
}

void e1000::start()
{
    uint32_t flags = read(0);
    write(0, flags | 0x40);
}

void e1000::handle_packet_reception()
{
    uint16_t old_cursor;

    while (rx_descriptor[rx_current_buf]->status & 0x1)
    {
        rx_descriptor[rx_current_buf]->status = 0;
        log("e1000", LOG_INFO, "received packets (size: {})", rx_descriptor[rx_current_buf]->length);

        {
            utils::context_lock locker{packet_reception_lock};
            void *data = malloc(8192);
            memcpy(data, (void *)(uint64_t)(rx_descriptor[rx_current_buf]->address), rx_descriptor[rx_current_buf]->length);
            packet_reception_list.push_back(data);
        }

        old_cursor = rx_current_buf;
        rx_current_buf = (rx_current_buf + 1) % RX_DESCRIPTOR_COUNT;
        write(E_RX_DESCRIPTOR_TAIL, old_cursor);
    }
}

int e1000::send_packet(uint8_t *data, uint16_t length)
{
    tx_desc *target = tx_descriptor[tx_current_buf];

    target->address = (uint64_t)data;
    target->length = length;
    target->command = END_OF_PACKET | INSERT_FCS | REPORT_STATUS;
    target->status = 0;

    uint8_t old_cursor = tx_current_buf;
    tx_current_buf = (tx_current_buf + 1) % TX_DESCRIPTOR_COUNT;
    write(E_TX_DESCRIPTOR_TAIL, tx_current_buf);

    while (!(tx_descriptor[old_cursor]->status & 0xff))
        ;
    return 0;
}

void *e1000::read_last_packet()
{

    utils::context_lock locker{packet_reception_lock};
    if (packet_reception_list.size() == 0)
    {
        return nullptr;
    }
    void *target = packet_reception_list[packet_reception_list.size() - 1];
    packet_reception_list.remove(packet_reception_list.size() - 1);
    return target;
}

void e1000::irq_handle()
{
    write(E_IMASK, 0x1);

    uint32_t status = read(0xc0);

    if (status & 0x04)
    {
        start();
    }
    else if (status & 0x10)
    {
    }
    else if (status & (1 << 6))
    {
        log("e1000", LOG_ERROR, "buffer overrun");
    }
    else if (status & 0x80)
    {
        handle_packet_reception();
    }
}

void e1000::turn_on_int()
{
    write(E_INTERRUPT_RATE, 6000);
    write(E_IMASK, (1 << 2) | (1 << 7) | (1 << 6));

    read(0xc0);
}

void e1000_int_handler(unsigned int v)
{
    e1000::the()->irq_handle();
}

void e1000::init(pci_device *dev)
{

    log("e1000", LOG_DEBUG, "loading e1000");
    pci_bar_data d = dev->get_bar(0);
    add_irq_handler(e1000_int_handler, 11);

    if (d.type == pci_bar_type::MM_IO_32)
    {
        bar_t = 0;
        for (size_t i = 0; i < d.size / PAGE_SIZE; i++)
        {
            map_page((d.base / PAGE_SIZE) * PAGE_SIZE + i * PAGE_SIZE, get_mem_addr((d.base / PAGE_SIZE) * PAGE_SIZE + i * PAGE_SIZE), true, false);
        }
        update_paging();
        mm_address = get_mem_addr(d.base);
        log("e1000", LOG_INFO, "e1000 is mm: {}", mm_address);
        log("e1000", LOG_INFO, "e1000 from: {}", d.base);
        log("e1000", LOG_INFO, "e1000 is length: {}", d.size);
    }
    else
    {
        bar_t = 1;
        io_base_addr = d.base;
        log("e1000", LOG_INFO, "e1000 is io: {}", io_base_addr);
    }

    if (eerp_rom_detection())
    {
        log("e1000", LOG_INFO, "e1000 has eeprom");
    }
    log("e1000", LOG_INFO, " == mac address == ");

    mac_detection();

    for (int i = 0; i < 6; i++)
    {
        log("e1000", LOG_INFO, "mac addr[{}] = {}", i, (uint32_t)(get_mac_addr().mac[i]));
    }

    for (int i = 0; i < 0x80; i++)
        write(0x5200 + i * 4, 0);
    log("e1000", LOG_INFO, "setup rx");

    setup_rx();
    log("e1000", LOG_INFO, "setup tx");
    setup_tx();

    log("e1000", LOG_INFO, "turn on interrupt for E1000");
    turn_on_int();
}
