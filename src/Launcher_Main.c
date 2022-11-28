//#############################################################################
// KJPL Launcher
//
// FILE:   Launcher_Main.c
// TARGET: TMS320F280049C (LAUNCHXL-F280049C)
// LANGUAGE: C
// COMPILER: TI 21.6.0.LTS
// DEBUGGER: XDS110(On-board-USB-Debugger)
// REFERENCE: C2000Ware_3_04_00_00
//#############################################################################
#include "driverlib.h"
#include "device.h"
#include <string.h>

#include "inc/protocols.h"

#ifdef _FLASH
// These are defined by the linker
extern uint16_t RamfuncsLoadStart;
extern uint16_t RamfuncsLoadSize;
extern uint16_t RamfuncsRunStart;
#endif

//Defines
#define AUTOBAUD 0
#define RESTRICTED_REGS 1
#define USE_TX_INTERRUPT 0
#define LOOPBACK 0
#define SLAVE_ADDRESS   0x3C

/***************************************************************************************
* Globals, for large data (Do not use malloc)
* Read stdint.h.In TMS320C2000, uin16_t is unsigned int.
***************************************************************************************/
uint16_t counter = 0; //unsigned int.
volatile uint16_t sData[16];
volatile uint16_t rData[16];
unsigned char data[16];

uint16_t cmd[16];
//char* gpsData[];

// Function Prototypes
void GPIO_controlStepper(uint32_t pin, uint16_t angle, uint32_t speed);
void I2C_init();
void ctrlStepper(uint16_t*);

//uint16_t* parseGPS(char* nmea);

//for setup


//Interrupt Service Routine
#if USE_TX_INTERRUPT
__interrupt void sciaTxISR(void);
__interrupt void scibTxISR(void);
#endif
__interrupt void sciaRxISR(void);
__interrupt void scibRxISR(void);


void main(void)
{
    // Configure Device.
    Device_init();
    Device_initGPIO();
    PinMux_setup_I2C();
    PinMux_setup_SCI();
    PinMux_setup_GPIO();

    I2C_init();

#if RESTRICTED_REGS
    EALLOW;
    PinMux_setup_EPWM();
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
#if USE_TX_INTERRUPT
    Interrupt_register(INT_SCIA_TX, sciaTxISR);
    Interrupt_register(INT_SCIB_TX, sciaTxISR);
#endif
    Interrupt_register(INT_SCIA_RX, sciaRxISR);
    Interrupt_register(INT_SCIB_RX, sciaRxISR);

    // Initialize SCIA and its FIFO.
    SCI_performSoftwareReset(SCIA_BASE);
    SCI_performSoftwareReset(SCIB_BASE);

    /************************************************************************
    * Configure SCIA (UART_A)
    * Bits per second = 9600
    * Data Bits = 8
    * Parity = None
    * Stop Bits = 1
    * Hardware Control = None
    *
    * Configure SCIB (UART_B)
    * Bits per second = 9600
    * Data Bits = 8
    * Parity = None
    * Stop Bits = 1
    * Hardware Control = Yes
    ************************************************************************/
    //SCIA - CMD
    SCI_setConfig(SCIA_BASE, 25000000, 9600, (SCI_CONFIG_WLEN_8 |
                                             SCI_CONFIG_STOP_ONE |
                                             SCI_CONFIG_PAR_NONE));
    //SCIB - GPS
    SCI_setConfig(SCIB_BASE, 25000000, 9600, (SCI_CONFIG_WLEN_8 |
                                             SCI_CONFIG_STOP_ONE |
                                             SCI_CONFIG_PAR_NONE));

#if LOOPBACK
    SCI_enableLoopback(SCIA_BASE);
    SCI_enableLoopback(SCIB_BASE);
#else
    SCI_disableLoopback(SCIA_BASE);
    SCI_disableLoopback(SCIB_BASE);
#endif
    SCI_resetChannels(SCIA_BASE);
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_TXRDY | SCI_INT_RXRDY_BRKDT);
    SCI_enableFIFO(SCIA_BASE);
    SCI_enableModule(SCIA_BASE);
    SCI_performSoftwareReset(SCIA_BASE);

    SCI_resetChannels(SCIB_BASE);
#if 0
    SCI_clearInterruptStatus(SCIB_BASE, SCI_INT_TXRDY | SCI_INT_RXRDY_BRKDT);
    SCI_enableFIFO(SCIB_BASE);
#endif
    SCI_enableModule(SCIB_BASE);
    SCI_performSoftwareReset(SCIB_BASE);

    // Enable the TXRDY and RXRDY interrupts.
    SCI_setFIFOInterruptLevel(SCIA_BASE, SCI_FIFO_TX0, SCI_FIFO_RX16);
    SCI_enableInterrupt(SCIA_BASE, SCI_INT_TXFF | SCI_INT_RXFF);
#if 0
    SCI_setFIFOInterruptLevel(SCIB_BASE, SCI_FIFO_TX0, SCI_FIFO_RX0);
    SCI_enableInterrupt(SCIB_BASE, SCI_INT_TXFF | SCI_INT_RXFF);
#endif

#if AUTOBAUD
    // Perform an autobaud lock.
    // SCI expects an 'a' or 'A' to lock the baud rate.
    SCI_lockAutobaud(SCIA_BASE);
    SCI_lockAutobaud(SCIB_BASE);
#endif

    // Clear the SCI interrupts before enabling them.
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_TXFF | SCI_INT_RXFF);
    SCI_clearInterruptStatus(SCIB_BASE, SCI_INT_TXFF | SCI_INT_RXFF);


    // Enable the interrupts in the PIE: Group 9 interrupts 1 & 2.
    Interrupt_enable(INT_SCIA_RX);
    Interrupt_enable(INT_SCIB_RX);
//    Interrupt_enable(INT_SCIA_TX);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);

    /************************************************************************
    * Configure EPWM 1&2 (PWM)
    * MODE = Chopper
    * duty cycle = (1/8)
    * One-Shot Pulse = Disabled
    *************************************************************************
    * ! Disable sync(Freeze clock to PWM as well). GTBCLKSYNC is applicable
    * ! only for multiple core devices. Uncomment the below statement if
    * ! applicable.
    ************************************************************************/
#if 0
    // SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_GTBCLKSYNC);
    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    initEPWM(EPWM1_BASE);
    EPWM_setChopperDutyCycle(EPWM1_BASE, 0);
    EPWM_setChopperFreq(EPWM1_BASE, 3);
    //EPWM_setChopperFirstPulseWidth(EPWM1_BASE, 10);
    EPWM_enableChopper(EPWM1_BASE);

    initEPWM(EPWM2_BASE);
    EPWM_setChopperDutyCycle(EPWM2_BASE, 0);
    EPWM_setChopperFreq(EPWM2_BASE, 3);
    //EPWM_setChopperFirstPulseWidth(EPWM1_BASE, 10);
    EPWM_enableChopper(EPWM2_BASE);

    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);
#endif
    // Enable global interrupts. and real time interrupt
    EINT;
    ERTM;

    // Set test data - for debug
    int i;
    for(i=0; i<16; i++) {
        sData[i] = i;
    }



    for(;;) {
        //sendDataSCI(SCIA_BASE, sData, SCI_FIFO_TX16);
        //DEVICE_DELAY_US(500000);
        //GPIO_writePin(22U, 0);
        //GPIO_writePin(13U, 1);
        //GPIO_controlStepper(56U, 180, 100000);
        //GPIO_controlStepper(40U, 30, 500);
        //GPIO_writePin(22U, 1);
        //GPIO_writePin(13U, 0);
        //GPIO_controlStepper(56U, 360*10, 500);
        //GPIO_controlStepper(40U, 30, 500);
        //GPIO_controlStepper(22U, 360, 5000);
//        rcvCmdData(SCIA_BASE, rData, SCI_FIFO_RX16);
//        parseMsgSCI(rData, cmd);
    }
}


#if USE_TX_INTERRUPT
/*******************************************************************
* sciaTxISR - Disable the TXRDY interrupt and print message asking
*             for a character.
*******************************************************************/
__interrupt void sciaTxISR(void) {
    // Disable the TXRDY interrupt.
//    SCI_disableInterrupt(SCIA_BASE, SCI_INT_TXRDY);
//    SCI_writeCharArray(SCIA_BASE, 0, 22);
//
//    // Ackowledge the PIE interrupt.
//    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
    asm ("  NOP");
}

/*******************************************************************
* sciaTxISR - Disable the TXRDY interrupt and print message asking
*             for a character.
*******************************************************************/
__interrupt void scibTxISR(void) {
    // Disable the TXRDY interrupt.
//    SCI_disableInterrupt(SCIA_BASE, SCI_INT_TXRDY);
//    SCI_writeCharArray(SCIA_BASE, 0, 22);
//
//    // Ackowledge the PIE interrupt.
//    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
    asm ("  NOP");
}
#endif


//sciaRxISR - Read the character from the RXBUF.
__interrupt void sciaRxISR(void) {
    uint16_t i, flag = 0;
    uint16_t parser[16] = { 0,};
    unsigned char Ack[2];

    // Enable the TXRDY interrupt again.
    //SCI_enableInterrupt(SCIA_BASE, SCI_INT_TXRDY);

    // Read a character from the RXBUF.
     for (i=0;i<RxCopyCount;i++) {
        receivedChar[i] = SCI_readCharNonBlocking(SCIA_BASE);
    }

    reParse:
    switch (receivedChar[0]) {
    case 0: //return ERR
        flag = 1;
        break;
    case 1: //unit8_t -> uint16_t
        flag = 2;
        parseMsgSCI(receivedChar, parser);
        memcpy(receivedChar, parser, sizeof(parser));
        goto reParse;
    case 2: //GPS
        flag = 3;
        NOP;
        break;
    case 3: //STEPPER
        flag = 4;
        ctrlStepper(receivedChar);
        break;
    case 4: //no need to parse
        flag = 5;
        NOP;
        break;
    default:
        break;
    }

    //return Ack via SCI
    Ack[0] = 1;
    Ack[1] = flag;
    SCI_writeCharArray(SCIA_BASE, (uint16_t*)Ack, sizeof(Ack));

    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_RXFF);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);

    RxReadyFlag = 1;
}

//scibRxISR - Read the character from the RXBUF.
__interrupt void scibRxISR(void) {
#if 0
    uint16_t i;
    unsigned char Ack[2];

    // Enable the TXRDY interrupt again.
    //SCI_enableInterrupt(SCIA_BASE, SCI_INT_TXRDY);

    // Read a character from the RXBUF.
    for (i=0;i<RxCopyCount;i++) {
        receivedChar[i] = SCI_readCharNonBlocking(SCIB_BASE);
    }

    //return Ack via SCI
    //if(receivedChar[0] == 1) {
        Ack[0] = 1;
        Ack[1] = 8;
    //}
    SCI_writeCharArray(SCIA_BASE, (uint16_t*)Ack, sizeof(Ack));

    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);

    RxReadyFlag = 1;
#else
    asm ("  NOP");
#endif
}

void GPIO_controlStepper(uint32_t pin, uint16_t angle, uint32_t speed) {
    uint32_t i;
    for(i = 0; i < (uint32_t)(angle / 1.8) ; i++) { // 1.8 degree per step
        GPIO_writePin(pin, 1);
        DEVICE_DELAY_US(500); //speed
        GPIO_writePin(pin, 0);
        DEVICE_DELAY_US(speed); //speed
    }
}

void ctrlStepper(uint16_t* cmd) {
    //ID, SEL, DIR, ANGLE, SPEED
    uint32_t pincfg[2] = { 0, };
    if(cmd[1] == 1) {
        pincfg[0] = 13U;
        pincfg[1] = 40U;
    }
    else if(cmd[1] = 2) {
        pincfg[0] == 56U;
        pincfg[1] == 22U;
    }

    GPIO_writePin(pincfg[0], cmd[2]);
    GPIO_controlStepper(pincfg[1], cmd[3], 500);
}

void I2C_init() {
    I2C_disableModule(I2CA_BASE);
    I2C_initMaster(I2CA_BASE, DEVICE_SYSCLK_FREQ, 400000, I2C_DUTYCYCLE_50);
    I2C_setConfig(I2CA_BASE, I2C_MASTER_SEND_MODE);
    I2C_setSlaveAddress(I2CA_BASE, 60);
    I2C_disableLoopback(I2CA_BASE);
    I2C_setBitCount(I2CA_BASE, I2C_BITCOUNT_8);
    I2C_setDataCount(I2CA_BASE, 2);
    I2C_setAddressMode(I2CA_BASE, I2C_ADDR_MODE_7BITS);
    I2C_setEmulationMode(I2CA_BASE, I2C_EMULATION_STOP_SCL_LOW);
    I2C_enableModule(I2CA_BASE);
}

void getDataI2C(uint32_t base, uint16_t Data) {
    uint16_t i, tmp[9];
    I2C_putData(base, 0x00);
    I2C_sendStartCondition(base);
    for(i = 0; i < 16; i++) {
        tmp[i] = I2C_getData(base);
    }
    memcpy(Data, tmp, sizeof(tmp));
}

#if 0
uint16_t* parseGPS(char* nmea) {
    char* data[16];
    uint16_t flag = 0;
    uint16_t i;
    char GPGLL[2][11];

    for(i=0; i<strlen(nmea); i++) {
        if(nmea[i] == 0x0A) {
            if(nmea[i+4] == 0x4C && nmea[i+5] == 0x4C) {
                flag = i;
            }
        }
        if(flag != 0) {
            break;
        }
    }

    for(i=flag+6; i<flag + 1024; i++) {
        uint16_t idx = 0;
        uint16_t idx2 = 0;
        if(nmea[i] == ',') {
            idx++;
            idx2 = 0;
        }

        if(idx == 3) {
            break;
        }

        if(nmea[i] != ',') {
            GPGLL[idx-1][idx2] = nmea[i];
            idx2++;
        }
    }

    data[0] = 2;
    data[1] = 0x4E;
    for(i=0;i<=4;i++) {
        data[i+3] = GPGLL[0][i];
    }
    data[6] = 0x2C;
    data[7] = 0x45;
    for(i=0;i<=4;i++) {
        data[i+8] = GPGLL[0][i];
    }

    return (uint16_t*) *data;
}
#endif
