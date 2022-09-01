//#############################################################################
// KJPL Launcher
//
// FILE:   Launcher_Main.c
// TARGET: TMS320F280049C (LAUNCHXL-F280049C)
// LANGUAGE: C
// COMPILER: TI 21.6.0.LTS
// DEBUGGER: XDS110(On-board-USB-Debugger)
// REFERENCES: C2000Ware_3_04_00_00
//#############################################################################
#include "driverlib.h"
#include "device.h"

#ifdef _FLASH
// These are defined by the linker (see device linker command file)
extern uint16_t RamfuncsLoadStart;
extern uint16_t RamfuncsLoadSize;
extern uint16_t RamfuncsRunStart;
#endif

//Defines
#define AUTOBAUD 0
#define RESTRICTED_REGS 0

// Globals, for large data.
uint16_t counter = 0;
uint16_t sData[16];
uint16_t rData[16];
unsigned char *msg;
unsigned char data[16];


// Function Prototypes

//for setup
void PinMux_setup(void);

//Interrupt Service Routine
__interrupt void sciaTxISR(void);
__interrupt void sciaRxISR(void);


void main(void)
{
    // Configure Device.
    Device_init();
    Device_initGPIO();
    PinMux_setup();

#if RESTRICTED_REGS
    EALLOW;
    EDIS;
#endif

    // Disable global interrupts.
    DINT;

    // Initialize interrupt controller and vector table.
    Interrupt_initModule();
    Interrupt_initVectorTable();
    IER = 0x0000;
    IFR = 0x0000;

    // Map the ISR to the wake interrupt.
    Interrupt_register(INT_SCIA_TX, sciaTxISR);
    Interrupt_register(INT_SCIA_RX, sciaRxISR);

    // Initialize SCIA and its FIFO.
    SCI_performSoftwareReset(SCIA_BASE);

    /************************************************************************
    * Configure SCIA (UART_A)
    * Bits per second = 9600
    * Data Bits = 8
    * Parity = None
    * Stop Bits = 1
    * Hardware Control = None
    ************************************************************************/
    SCI_setConfig(SCIA_BASE, 25000000, 9600, (SCI_CONFIG_WLEN_8 |
                                             SCI_CONFIG_STOP_ONE |
                                             SCI_CONFIG_PAR_NONE));
    SCI_resetChannels(SCIA_BASE);
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_TXRDY | SCI_INT_RXRDY_BRKDT);
    SCI_enableFIFO(SCIA_BASE);
    SCI_enableModule(SCIA_BASE);
    SCI_performSoftwareReset(SCIA_BASE);

    // Enable the TXRDY and RXRDY interrupts.
    SCI_setFIFOInterruptLevel(SCIA_BASE, SCI_FIFO_TX0, SCI_FIFO_RX16);
    SCI_enableInterrupt(SCIA_BASE, SCI_INT_TXFF | SCI_INT_RXFF);

#if AUTOBAUD
    // Perform an autobaud lock.
    // SCI expects an 'a' or 'A' to lock the baud rate.
    SCI_lockAutobaud(SCIA_BASE);
#endif

    // Send starting message.
    msg = "\r\nRest\n\0";
    SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 8);

    // Clear the SCI interrupts before enabling them.
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_TXFF | SCI_INT_RXFF);


    // Enable the interrupts in the PIE: Group 9 interrupts 1 & 2.
    Interrupt_enable(INT_SCIA_RX);
    Interrupt_enable(INT_SCIA_TX);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);

    // Enable global interrupts.
    EINT;

    for(;;) {

    }
}

/*******************************************************************
* sciaTxISR - Disable the TXRDY interrupt and print message asking
*             for a character.
******************************************8************************/
__interrupt void sciaTxISR(void) {
    // Disable the TXRDY interrupt.
    SCI_disableInterrupt(SCIA_BASE, SCI_INT_TXRDY);

    msg = "\r\nEnter a character: \0";
    SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 22);

    // Ackowledge the PIE interrupt.
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
}

/*******************************************************************
* sciaRxISR - Read the character from the RXBUF and echo it back.
*******************************************************************/
__interrupt void sciaRxISR(void) {
    uint16_t receivedChar;

    // Enable the TXRDY interrupt again.
    SCI_enableInterrupt(SCIA_BASE, SCI_INT_TXRDY);

    // Read a character from the RXBUF.
    receivedChar = SCI_readCharBlockingNonFIFO(SCIA_BASE);

    // Echo back the character.
    msg = "  You sent: \0";
    SCI_writeCharArray(SCIA_BASE, (uint16_t*)data, 16);
//    SCI_writeCharBlockingNonFIFO(SCIA_BASE, receivedChar);

    // Acknowledge the PIE interrupt.
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);

    counter++;
}

void PinMux_setup(void) {
    //setup PINMUX - READ TRM
    // GPIO28 - SCI Rx pin, Pin3
    GPIO_setMasterCore(DEVICE_GPIO_PIN_SCIRXDA, GPIO_CORE_CPU1);
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCIRXDA);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_DIR_MODE_IN);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCIRXDA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_QUAL_ASYNC);

    // GPIO29 - SCI(UART) Tx pin, Pin2
    GPIO_setMasterCore(DEVICE_GPIO_PIN_SCITXDA, GPIO_CORE_CPU1);
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCITXDA);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCITXDA, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCITXDA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCITXDA, GPIO_QUAL_ASYNC);
}
