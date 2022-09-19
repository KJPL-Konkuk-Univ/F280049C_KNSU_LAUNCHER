/*
 * protocols.h
 *
 *  Created on: 2022. 9. 5.
 *      Author: Trollonion03
 */

#ifndef INC_PROTOCOLS_H_
#define INC_PROTOCOLS_H_



void sendDataSCI(uint32_t SelSCI, uint16_t * TrsData, SCI_TxFIFOLevel size);
void rcvCmdData(uint32_t SelSCI, uint16_t * RcvData, SCI_RxFIFOLevel size);
void parseMsgSCI(uint16_t *DataFrame, uint16_t* CmdData);

#endif /* INC_PROTOCOLS_H_ */
