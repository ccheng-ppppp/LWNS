/*
 * lwns_adapter_blemesh_mac.h
 *
 *  Created on: Jul 19, 2021
 *      Author: WCH
 */

#ifndef _LWNS_ADAPTER_BLEMESH_MAC_H_
#define _LWNS_ADAPTER_BLEMESH_MAC_H_

#ifdef __cplusplus
extern "C"
{
#endif


#define LWNS_USE_BLEMESH_MAC            0   //�Ƿ�ʹ��ģ��blemesh��macЭ�飬ע��ֻ��ʹ��һ��mac��Э�顣

#if LWNS_USE_BLEMESH_MAC

#include "WCH_LWNS_LIB.h"

struct blemesh_mac_phy_manage_struct{
    struct blemesh_mac_phy_manage_struct* next;
    u8* data;
};//ģ��blemesh mac�㷢�͹���ṹ��

#define LWNS_ENCRYPT_ENABLE               1   //�Ƿ�ʹ�ܼ���

#define LWNS_ADDR_USE_BLE_MAC             1  //�Ƿ�ʹ������Ӳ����mac��ַ��Ϊ���ַ

#define LWNS_NEIGHBOR_MAX_NUM             8   //����ھ�����

#define LWNS_MAC_TRANSMIT_TIMES           2   //һ�η��ͣ�����Ӳ�����ͼ���

#define LWNS_MAC_PERIOD_MS                10  //mac�������ڣ������л�

#define LWNS_MAC_SEND_DELAY_MAX_MS        10  //����mesh����10ms���ڵ��������

#define LWNS_MAC_SEND_PACKET_MAX_NUM      8   //�����������֧�ּ������ݰ��ȴ�����

#define BLE_PHY_ONE_PACKET_MAX_625US      5

#define LLE_MODE_ORIGINAL_RX                        (0x80)  //�������LLEMODEʱ���ϴ˺꣬����յ�һ�ֽ�Ϊԭʼ���ݣ�ԭ��ΪRSSI��

extern void RF_Init( void );

void lwns_init(void);

#define LWNS_HTIMER_PERIOD_MS             20 //Ϊ(1000/HTIMER_SECOND_NUM)

//RF_TX��RF_RX���õ����ͣ������޸ģ����Ƽ���
#define USER_RF_RX_TX_TYPE 0xff

//receive process evt
#define LWNS_PHY_RX_OPEN_EVT              1
#define LWNS_PHY_RX_CHANGE_CHANNEL_EVT    2
//send process evt
#define LWNS_HTIMER_PERIOD_EVT            1
#define LWNS_PHY_OUTPUT_PREPARE_EVT       2
#define LWNS_PHY_OUTPUT_FINISH_EVT        4

#define LWNS_PHY_OUTPUT_TIMEOUT_MS        5

#ifdef __cplusplus
}
#endif

#endif  /* LWNS_USE_BLEMESH_MAC */

#endif /* _LWNS_ADAPTER_BLEMESH_MAC_H_ */
