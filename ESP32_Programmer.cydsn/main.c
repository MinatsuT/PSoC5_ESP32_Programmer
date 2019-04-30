/* ========================================
 *
 * ESP32 Programmer
 *
 * Copyright Minatsu, 2018
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * ========================================
 */
#include "project.h"
#include "stdio.h"

#define USBFS_DEVICE (0u)
#define USBUART_BUFFER_SIZE (64)
#define CDC_TIMEOUT (100u)

static uint8 en = 1;
static uint8 io0 = 1;
static uint32 baud = 115200;

static uint32 en_delay_timer = 0;
static uint8 en_delay_end = 0;

static uint32 cdc_timer = 0;

CY_ISR_PROTO(ISR_1kHz);

/**************************************
 * Debug support facilities
 *************************************/
#define Debug 1

/* Debug Print */
char dbuf[256];

#if defined(Debug)
#define DP(...)                                                                                                                            \
    {                                                                                                                                      \
        sprintf(dbuf, __VA_ARGS__);                                                                                                        \
        UART_KitProg_PutString(dbuf);                                                                                                      \
    }
#else
#define DP(...)
#endif

/**************************************
 * Main
 *************************************/
int main(void) {
    uint16 count_rx_pc, count_rx_esp32;
    uint8 buffer[USBUART_BUFFER_SIZE + 1];
    uint8 line_changed, control_state;
    uint32 tmp_baud;
    uint8 tmp_en;
    uint16 readByte;
    uint8 H_byte, L_byte;

    /* Enable global interrupts. */
    CyGlobalIntEnable;

    /* Start components. */
    isr_1_StartEx(ISR_1kHz);
    VDAC_33_Start();
    USBUART_Start(USBFS_DEVICE, USBUART_5V_OPERATION);
    UART_KitProg_Start();
    UART_ESP32_Start();
    PWM_LED_Start();

    DP("\nUSB UART for ESP32\n");

    for (;;) {
        if (USBUART_IsConfigurationChanged()) {
            if (USBUART_GetConfiguration()) {
                // Enumeration is done.
                USBUART_CDC_Init();
                DP("USBUART init\n");
            }
        }

        if (USBUART_GetConfiguration()) {
            // ----------------------------------------
            // From PC to ESP32
            // ----------------------------------------
            if (USBUART_DataIsReady()) {
                if ((count_rx_pc = USBUART_GetAll(buffer))) {
                    UART_ESP32_PutArray(buffer, count_rx_pc);
                    buffer[count_rx_pc] = 0;
                }
            }

            // ----------------------------------------
            // From ESP32 to PC
            // ----------------------------------------
            for (count_rx_esp32 = 0; UART_ESP32_GetRxBufferSize() && count_rx_esp32 < USBUART_BUFFER_SIZE;) {
                readByte = UART_ESP32_GetByte();
                H_byte = readByte >> 8;
                L_byte = readByte & 0xff;
                if (!H_byte) {
                    buffer[count_rx_esp32++] = L_byte;
                }
            }
            if (count_rx_esp32) {
                // Wait until ready to send.
                cdc_timer = 100;
                while (!USBUART_CDCIsReady() && cdc_timer) {
                }
                if (USBUART_CDCIsReady()) {
                    USBUART_PutData(buffer, count_rx_esp32);
                }
            }

            /* Check for Line settings change. */
            if ((line_changed = USBUART_IsLineChanged())) {
                if ((line_changed & USBUART_LINE_CODING_CHANGED)) {
                    if ((tmp_baud = USBUART_GetDTERate()) != baud) {
                        // baudrate is changed
                        baud = tmp_baud;

                        // Change clock freq. for UART_ESP32.
                        // UART_ESP32_Stop();
                        UART_CLK_SetDividerValue(1.0 * BCLK__BUS_CLK__HZ / 16 / baud + 0.5);
                        // UART_ESP32_Start();
                    }
                }

                if (line_changed & USBUART_LINE_CONTROL_CHANGED) {
                    control_state = USBUART_GetLineControl();

                    // DTR and RTS are active-low.
                    //  When DTR==true, IO0 should be LOW.
                    //  When RTS==true,  EN should be LOW.
                    io0 = !(control_state & USBUART_LINE_CONTROL_DTR);
                    tmp_en = !(control_state & USBUART_LINE_CONTROL_RTS);

                    /*
                     * reset-to-bootloader sequence is as follows:
                     *  IO0=HIGH, EN=HIGH : normal state
                     *  IO0=HIGH, EN=LOW  : chip in reset
                     *  IO0=LOW,  EN=HIGH : chip out of reset
                     *  IO0=HIGH, EN=HIGH : back to normal state
                     */

                    if (!en && tmp_en) {
                        // Rising edge of EN is detected.
                        // EN should rise after IO0 is surely LOW and before IO0 will be HIGH.
                        // Set delay timer to 40ms because IO0 will rise within 50ms in the shortest case.
                        en_delay_timer = 40;

                        LED_Reg_Write((!io0) ? 1 : 0);
                    }
                    en = tmp_en;

                    if (en_delay_timer) {
                        // Delay timer is active, set IO0 as it is and keep EN LOW.
                        Control_Reg_Write((io0 << 1) | 0);
                    } else {
                        // Delay timer is inactive, set IO0 and EN as they are.
                        Control_Reg_Write((io0 << 1) | en);
                    }
                }

#if defined(Debug)
                control_state = USBUART_GetLineControl();
                DP("%9ld baud, DTR(%4s) RTS(%4s)\r", baud, ((control_state & USBUART_LINE_CONTROL_DTR)) ? "HIGH" : "LOW",
                   ((control_state & USBUART_LINE_CONTROL_RTS)) ? "HIGH" : "LOW");
#endif
            }
        }

        if (en_delay_end) {
            en_delay_end = 0;
            // Delay timer is ended, set IO0 and EN as they are.
            Control_Reg_Write((io0 << 1) | en);
#if defined(Debug)
            if (!io0) {
                DP("\n\n*** Program start ***\n\n");
            }
#endif
        }
    }
}

/**************************************
 * Interrupt handlers
 *************************************/
// ISR for 1kHz (1 ms) timer
CY_ISR(ISR_1kHz) {
    if (en_delay_timer) {
        en_delay_timer--;
        if (!en_delay_timer) {
            en_delay_end = 1;
        }
    };

    if (cdc_timer) {
        cdc_timer--;
    }
}

/* [] END OF FILE */
