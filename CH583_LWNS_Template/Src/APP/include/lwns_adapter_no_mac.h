/*
 * lwns_adapter_no_mac.h
 *
 *  Created on: Jul 19, 2021
 *      Author: WCH
 */

#ifndef _LWNS_ADAPTER_NO_MAC_H_
#define _LWNS_ADAPTER_NO_MAC_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define LWNS_USE_NO_MAC  0 //�Ƿ�ʹ�ܴ�͸��macЭ�飬�ʺϲ����ڲ���������������磬�������ʴӻ�����������硣

#if LWNS_USE_NO_MAC

#include "WCH_LWNS_LIB.h"

typedef enum {
    BLE_PHY_MANAGE_STATE_FREE = 0,
    BLE_PHY_MANAGE_STATE_SENDING,
} BLE_PHY_MANAGE_STATE_t;

#define LWNS_ENCRYPT_ENABLE               1   //�Ƿ�ʹ�ܼ���

#define LWNS_ADDR_USE_BLE_MAC             1  //�Ƿ�ʹ������Ӳ����mac��ַ��Ϊ���ַ

#define LWNS_NEIGHBOR_MAX_NUM             1   //����ھ�������null mac��ʹ���ھӱ�

#define LLE_MODE_ORIGINAL_RX                        (0x80)  //�������LLEMODEʱ���ϴ˺꣬����յ�һ�ֽ�Ϊԭʼ���ݣ�ԭ��ΪRSSI��

extern void RF_Init( void );

void lwns_init(void);

#define LWNS_HTIMER_PERIOD_MS             20//Ϊ(1000/HTIMER_SECOND_NUM)


//RF_TX��RF_RX���õ����ͣ������޸ģ����Ƽ���
#define USER_RF_RX_TX_TYPE 0xff


//receive process evt
#define LWNS_PHY_RX_OPEN_EVT          1
//send process evt
#define LWNS_HTIMER_PERIOD_EVT        1
#define LWNS_PHY_OUTPUT_FINISH_EVT    2

#define LWNS_PHY_OUTPUT_TIMEOUT_MS        5



#endif  /* LWNS_USE_NO_MAC */

#ifdef __cplusplus
}
#endif

#endif /* _LWNS_ADAPTER_NO_MAC_H_ */
