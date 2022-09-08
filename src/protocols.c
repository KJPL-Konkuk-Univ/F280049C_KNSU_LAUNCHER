/*********************************************
 * protocols.c
 *
 *  Created on: 2022. 9. 5.
 *      Author: Trollonion03
 */


#include "driverlib.h"
#include "device.h"

uint16_t RxReadyFlag = 0;
uint16_t RxCopyCount = 0;
extern uint16_t receivedChar[16];

void SendDataSCI(uint32_t SelSCI, uint16_t * TrsData, SCI_TxFIFOLevel size) {
    uint16_t i, rACK[2];


    for (i=0;i<size;i++) {
        SCI_writeCharBlockingNonFIFO(SelSCI, TrsData[i]);
    }

    //ADD check ACK
//    RcvCmdData(SCIA_BASE, rACK, SCI_FIFO_RX2);
//    if(rACK[1] != 8) {
//        ESTOP0;
//    }
}

void RcvCmdData(uint32_t SelSCI, uint16_t * RcvData, SCI_RxFIFOLevel size) {
    RxReadyFlag = 0;
    RxCopyCount = size;

    SCI_resetChannels(SelSCI);
    SCI_clearInterruptStatus(SelSCI, SCI_INT_TXRDY | SCI_INT_RXRDY_BRKDT);
    SCI_enableFIFO(SelSCI);
    SCI_enableModule(SelSCI);
    SCI_performSoftwareReset(SelSCI);

    SCI_setFIFOInterruptLevel(SCIA_BASE, SCI_FIFO_TX0, size);
    SCI_enableInterrupt(SCIA_BASE, SCI_INT_TXRDY | SCI_INT_RXFF);

    while(RxReadyFlag == 0);


    memcpy(RcvData, receivedChar, size*2);
}
