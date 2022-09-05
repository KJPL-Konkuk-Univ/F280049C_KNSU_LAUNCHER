/*********************************************
 * protocols.c
 *
 *  Created on: 2022. 9. 5.
 *      Author: Trollonion03
 */


#include "driverlib.h"
#include "device.h"

void SendDataSCI(uint32_t SelSCI, uint16_t * TrsData, uint16_t size) {
    uint16_t i;
    for (i=0;i<size;i++) {
        SCI_writeCharBlockingNonFIFO(SelSCI, TrsData[i]);
    }
}
