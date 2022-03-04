/*----------------------------------------------
 * 
 * CAN_12 : eget definerat CAN-kom av grupp12
 * 
 * --------------------------------------------*/
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_can.h"
#include "stm32f4xx_rcc.h"
#include "can12.h"

CanRxMsg RxMessage;
CanTxMsg TxMessage;

void can1_init(VoidFunction interrupt) {
    GPIO_InitTypeDef GPIO_InitStructure;
    CAN_FilterInitTypeDef  CAN_FilterInitStructure;

    GPIO_StructInit( &GPIO_InitStructure );

    GPIO_DeInit( GPIOB );
    CAN_DeInit( CAN1 );
	
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_CAN1, ENABLE );
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOB, ENABLE);

    /* Connect CAN1 pins to AF */
    /* PB9 - CAN1 TX */
    /* PB8 - CAN1 RX  */
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_CAN1);  	
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource8, GPIO_AF_CAN1);  

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);	

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;	
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;	
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);	

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;	
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;	
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    
    /* CAN filter init */
    CAN_FilterInitStructure.CAN_FilterNumber = CAN_Filter_FIFO0;
    CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask; // IdMask and not IdList because intergrated message in ID field
    CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
    CAN_FilterInitStructure.CAN_FilterIdHigh = nodeID<<5;
    CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh = (nodeMask<<5); 
    CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 0;
    CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
    CAN_FilterInit(&CAN_FilterInitStructure);
    
    CAN_InitTypeDef CAN_InitStructure;
    CAN_InitStructure.CAN_TTCM = DISABLE; // time-triggered communication mode = DISABLED
    CAN_InitStructure.CAN_ABOM = DISABLE; // automatic bus-off management mode = DISABLED
    CAN_InitStructure.CAN_AWUM = DISABLE; // automatic wake-up mode = DISABLED
    CAN_InitStructure.CAN_NART = DISABLE; // non-automatic retransmission mode = DISABLED
    CAN_InitStructure.CAN_RFLM = DISABLE; // receive FIFO locked mode = DISABLED
    CAN_InitStructure.CAN_TXFP = DISABLE; // transmit FIFO priority = DISABLED
    CAN_InitStructure.CAN_Mode = CAN_Mode_Normal; // normal CAN mode
    //
    // 42 MHz clock on APB1
    // Prescaler = 7 => time quanta tq = 1/6 us
    // Bit time = tq*(SJW + BS1 + BS2)
    // See figure 346 in F407 - Reference Manual.pdf
    // 
    CAN_InitStructure.CAN_SJW = CAN_SJW_1tq; // synchronization jump width = 1
    CAN_InitStructure.CAN_BS1 = CAN_BS1_3tq; 
    CAN_InitStructure.CAN_BS2 = CAN_BS2_4tq; 
    CAN_InitStructure.CAN_Prescaler = 7; // baudrate 750kbps

    if (CAN_Init(CAN1, &CAN_InitStructure) == CAN_InitStatus_Failed){
		#ifdef DEBUG_MODE
		DUMP("CAN #1 Init failed!");
		#endif
	} else{
		#ifdef DEBUG_MODE
		DUMP("CAN #1 Init successful!");
		#endif
	}
        

    *((void (**)(void) ) CAN1_IRQ_VECTOR ) = interrupt;
    
    //__set_BASEPRI(__ENABLED_PRIORITY << (8 - __NVIC_PRIO_BITS));
    NVIC_SetPriority( CAN1_RX0_IRQn, __IRQ_PRIORITY);
    NVIC_EnableIRQ( CAN1_RX0_IRQn);
    CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);
}

int can_send(CANMsg *msg){
    unsigned char index;
    CAN_TypeDef* canport = CAN1;
    CanTxMsg TxMessage;
    uint8_t TransmitMailbox = 0;
    //set the transmit ID, standard identifiers are used, combine IDs
    TxMessage.StdId = (msg->urgent<<10) | (msg->direction<<9) | (msg->msgId<<5) | msg->nodeId;
	TxMessage.RTR = CAN_RTR_Data;
    TxMessage.IDE = CAN_Id_Standard;
    if (msg->length > 8) 
        msg->length = 8; 
    TxMessage.DLC = msg->length; // set number of bytes to send
	
	
    for (index = 0; index < msg->length; index++) {
        TxMessage.Data[index] = msg->buff[index]; //copy data to buffer
    }

    TransmitMailbox = CAN_Transmit(canport, &TxMessage);

    if (TransmitMailbox == CAN_TxStatus_NoMailBox) {
        DUMP("CAN TxBuf full!\n\r");
        return 0;
    }
    while (CAN_TransmitStatus(canport, TransmitMailbox) == CAN_TxStatus_Pending) ;
    
    if (CAN_TransmitStatus(canport, TransmitMailbox) == CAN_TxStatus_Failed) {
        DUMP("CAN Tx fail!\n\r");
        return 0;
    }    
    return 1;
}


int can_receive(CANMsg *msg){
    unsigned char index;
    CanRxMsg RxMessage;
    if (CAN_GetFlagStatus( CAN1, CAN_FLAG_FMP0) != SET)  // Data received in FIFO0
        return 0;
	
    CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);
	msg->urgent = (RxMessage.StdId >> 10) & 0x01;
	msg->direction = (RxMessage.StdId >> 9) & 0x01;
    msg->msgId  = (RxMessage.StdId >> 5) & 0x0F;
    msg->nodeId = RxMessage.StdId & 0x1F;
    msg->length = (RxMessage.DLC & 0x0F);
	
    for (index = 0; index < msg->length; index++) {
        // Get received data
        msg->buff[index] = RxMessage.Data[index];
    }
    return 1;
}

