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

static uint32 timer = 0;
static uint8 timer_end = 0;

static uint32 cdcTimer = 0;

CY_ISR_PROTO(MyISR);

/**************************************
 * Debug support facilities
 *************************************/
/* Debug Print */
char dbuf[256];

#define MONITOR(...)

#define DP(...)                                                                                                                            \
    {                                                                                                                                      \
        sprintf(dbuf, __VA_ARGS__);                                                                                                        \
        UART_KitProg_PutString(dbuf);                                                                                                      \
    }

int main(void) {
    uint16 count;
    uint8 buffer[USBUART_BUFFER_SIZE + 1];
    uint8 state;
    uint32 tmp_baud;
    uint8 tmp_en;
    uint16 readByte;
    uint8 H, L;

    /* Enable global interrupts. */
    CyGlobalIntEnable;

    /* Start components. */
    isr_1_StartEx(MyISR);
    VDAC_33_Start();
    USBUART_Start(USBFS_DEVICE, USBUART_5V_OPERATION);
    UART_KitProg_Start();
    UART_ESP32_Start();

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
            /* From PC to ESP32 */
            if (USBUART_DataIsReady()) {
                if ((count = USBUART_GetAll(buffer))) {
                    UART_ESP32_PutArray(buffer, count);
                    buffer[count] = 0;
                }
            }

            /* From ESP32 to PC */
            for (count = 0; UART_ESP32_GetRxBufferSize() && count < USBUART_BUFFER_SIZE;) {
                // buffer[count] = UART_ESP32_ReadRxData();
                // buffer[count] = UART_ESP32_GetChar();
                readByte = UART_ESP32_GetByte();
                H = readByte >> 8;
                L = readByte & 0xff;
                // if (!H && L != 0xc2) {
                if (!H) {
                    buffer[count++] = L;
                    MONITOR("%c",L);
                }
            }
            state = USBUART_GetLineControl() & (USBUART_LINE_CONTROL_DTR | USBUART_LINE_CONTROL_RTS);
            // if (count && !state) {
            // if (count && !timer) {
            if (count) {
                // Wait until ready to send.
                cdcTimer = 100;
                while (!USBUART_CDCIsReady() && cdcTimer) {
                }
                if (USBUART_CDCIsReady()) {
                    USBUART_PutData(buffer, count);
                }
            }

            /* Check for Line settings change. */
            if ((state = USBUART_IsLineChanged())) {
                if ((state & USBUART_LINE_CODING_CHANGED)) {
                    if ((tmp_baud = USBUART_GetDTERate()) != baud) {
                        // baudrate is changed
                        baud = tmp_baud;

                        // Change clock freq. for UART_ESP32.
                        // UART_ESP32_Stop();
                        UART_CLK_SetDividerValue(1.0 * BCLK__BUS_CLK__HZ / 8 / baud + 0.5);
                        // UART_ESP32_Start();
                    }
                }

                if ((state & USBUART_LINE_CONTROL_CHANGED)) {
                    // DTR/RTS is changed
                    state = USBUART_GetLineControl() & (USBUART_LINE_CONTROL_DTR | USBUART_LINE_CONTROL_RTS);
                    io0 = (state == USBUART_LINE_CONTROL_DTR) ? 0 : 1;
                    tmp_en = (state == USBUART_LINE_CONTROL_RTS) ? 0 : 1;
                    if (!en && tmp_en) { // Rising edge of EN is detected.
                        timer = 150;     // Set timer to 15ms.
                    }
                    en = tmp_en;

                    if (timer) {
                        // Timer is active, keep EN low.
                        Control_Reg_Write(io0 << 1);
                    } else {
                        // Timer is inactive, set IO0 and EN as they are.
                        Control_Reg_Write((io0 << 1) | en);
                    }
                }

                state = USBUART_GetLineControl();
                DP("%9ld baud, DTR(%4s) RTS(%4s)\r", baud, ((state & USBUART_LINE_CONTROL_DTR)) ? "HIGH" : "LOW",
                   ((state & USBUART_LINE_CONTROL_RTS)) ? "HIGH" : "LOW");
            }
        }

        if (timer_end) {
            timer_end = 0;
            // timer is ended, set IO0 and EN as they are.
            Control_Reg_Write((io0 << 1) | en);
            if (!io0) {
                DP("\n\n*** Program start ***\n\n");
            }
        }
    }
}

CY_ISR(MyISR) {
    if (timer) {
        timer--;
        if (!timer) {
            timer_end = 1;
        }
    };
    if (cdcTimer) {
        cdcTimer--;
    }
}

/* [] END OF FILE */
