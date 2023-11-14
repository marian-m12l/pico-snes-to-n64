#include <stdio.h>
#include "pico/stdlib.h"
#include <pico/multicore.h>
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "snes.pio.h"

#include "N64Console.hpp"
#include "n64_definitions.h"

#define SODIUM64_MAPPING
 
#define LATCH_PIN 13
#define CLOCK_PIN 14
#define DATA_PIN 15

#define BTN_B 15
#define BTN_Y 14
#define BTN_SELECT 13
#define BTN_START 12
#define BTN_UP 11
#define BTN_DOWN 10
#define BTN_LEFT 9
#define BTN_RIGHT 8
#define BTN_A 7
#define BTN_X 6
#define BTN_L 5
#define BTN_R 4

#define JOYBUS_PIN 16
n64_report_t n64_report = default_n64_report;

uint16_t buttons = 0xffff;

bool get_button(int btn) {
    return (buttons >> btn) & 0x1;
}

void core1_entry() {
    printf("[core1] Initializing joybus on core 1\n");
    N64Console *console = new N64Console(JOYBUS_PIN, pio1);
    // Handle poll commands
    printf("[core1] Entering loop\n");
    while(true) {
        //printf("[core1] Waiting for POLL command from console...\n");
        console->WaitForPoll();
        //printf("[core1] sending report\n");
        // TODO convert current snes status to n64 report??

        // Update N64 controller report
#ifdef SODIUM64_MAPPING
        n64_report.dpad_right = get_button(BTN_RIGHT) == 0;
        n64_report.dpad_left = get_button(BTN_LEFT) == 0;
        n64_report.dpad_down = get_button(BTN_DOWN) == 0;
        n64_report.dpad_up = get_button(BTN_UP) == 0;
        n64_report.start = 0;
        n64_report.z = 0;
        n64_report.b = get_button(BTN_SELECT) == 0;
        n64_report.a = get_button(BTN_START) == 0;
        n64_report.c_right = get_button(BTN_A) == 0;
        n64_report.c_left = get_button(BTN_Y) == 0;
        n64_report.c_down = get_button(BTN_B) == 0;
        n64_report.c_up = get_button(BTN_X) == 0;
        n64_report.r = get_button(BTN_R) == 0;
        n64_report.l = get_button(BTN_L) == 0;
        n64_report.stick_x = 0;
        n64_report.stick_y = 0;
#else
        n64_report.dpad_right = get_button(BTN_RIGHT) == 0;
        n64_report.dpad_left = get_button(BTN_LEFT) == 0;
        n64_report.dpad_down = get_button(BTN_DOWN) == 0;
        n64_report.dpad_up = get_button(BTN_UP) == 0;
        n64_report.start = get_button(BTN_START) == 0;
        n64_report.z = 0;
        n64_report.b = get_button(BTN_B) == 0;
        n64_report.a = get_button(BTN_A) == 0;
        n64_report.c_right = 0;
        n64_report.c_left = 0;
        n64_report.c_down = 0;
        n64_report.c_up = 0;
        n64_report.r = get_button(BTN_R) == 0;
        n64_report.l = get_button(BTN_L) == 0;
        n64_report.stick_x = 0;
        n64_report.stick_y = 0;
#endif

        console->SendReport(&n64_report);
    }
}

int main(void) 
{
    // TODO 130MHz for joybus pio ?!! --> fix clkdiv
    set_sys_clock_khz(130'000, true);

    stdio_init_all();

    printf("Pico SNES-to-N64 joypad\n");

    printf("[core0] Spawn joybus process on core 1\n");
    multicore_launch_core1(core1_entry);

    // Initialize PIO program to poll controller continuously
    PIO pio = pio0;
    int sm = pio_claim_unused_sm(pio, true);
    uint offset = pio_add_program(pio, &snes_program);
    snes_program_init(pio, sm, offset, LATCH_PIN);

    // PIO RX FIFO to read from
    const volatile void* ptr = &pio->rxf[sm];

    // Setup DMA to copy latest controller status as soon as data is available
    int dma_data = dma_claim_unused_channel(true);
    int dma_ctrl = dma_claim_unused_channel(true);
    dma_channel_config c_data = dma_channel_get_default_config(dma_data);
    channel_config_set_transfer_data_size(&c_data, DMA_SIZE_16);    // Controller status is 16 bits
    channel_config_set_read_increment(&c_data, false);              // Always read from PIO RX FIFO
    channel_config_set_write_increment(&c_data, false);             // Always write to `buttons` in RAM
    channel_config_set_dreq(&c_data, pio_get_dreq(pio, sm, false)); // Read is paced by the PIO input
    channel_config_set_chain_to(&c_data, dma_ctrl);                 // Chains to control DMA when done
    dma_channel_configure(dma_data, &c_data,
        &buttons,           // Destination pointer is buttons status in RAM
        ptr,                // Source pointer is PIO RX FIFO
        1024,               // Arbitrary number of transfers
        false               // Do not start immediately
    );
    // The control DMA triggers the data DMA continuously via ping-pong chain
    dma_channel_config c_ctrl = dma_channel_get_default_config(dma_ctrl);
    channel_config_set_transfer_data_size(&c_ctrl, DMA_SIZE_32);    // RX FIFO address is 32 bits
    channel_config_set_read_increment(&c_ctrl, false);              // Always read RX FIFO's address
    channel_config_set_write_increment(&c_ctrl, false);             // Always write to data DMA's read-address trigger register
    dma_channel_configure(dma_ctrl, &c_ctrl,
        &dma_hw->ch[dma_data].al3_read_addr_trig,  // Destination pointer is data DMA's read-address trigger register
        &ptr,               // Source pointer is the address of RX FIFO
        1,                  // A single transfer is needed to restart data DMA
        false               // Do not start immediately
    );
    // Start DMA
    dma_channel_start(dma_ctrl);

    // Start PIO program
    pio_sm_set_enabled(pio, sm, true);
    printf("Start polling SNES controller\n");
    
    while (true)
    {
        // Print controller status 60 times per second
        sleep_us(16000);
        printf("Joypad(SNES): A=%d B=%d X=%d Y=%d / 0x%04x\n", get_button(BTN_A), get_button(BTN_B), get_button(BTN_X), get_button(BTN_Y), buttons);
        printf("Joypad(N64): A=%d B=%d C-Down=%d C-Right=%d / 0x%08x\n", n64_report.a, n64_report.b, n64_report.c_down, n64_report.c_right, n64_report);
    }

    return 0;
}
