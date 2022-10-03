/*
 * protocols.h
 *
 *  Created on: 2022. 9. 5.
 *      Author: Trollonion03
 */

#ifndef INC_PROTOCOLS_H_
#define INC_PROTOCOLS_H_


extern uint16_t RxReadyFlag;
extern uint16_t RxCopyCount;
extern uint16_t receivedChar[16];


void sendDataSCI(uint32_t SelSCI, uint16_t * TrsData, SCI_TxFIFOLevel size);
void rcvCmdData(uint32_t SelSCI, uint16_t * RcvData, SCI_RxFIFOLevel size);
void parseMsgSCI(uint16_t *DataFrame, uint16_t* CmdData);
void makePacketSCI(uint16_t* dataFrame, uint16_t* packet_data, uint16_t ID, uint16_t len, uint16_t target);

#endif /* INC_PROTOCOLS_H_ */
