/*
 * lwns_prov.h
 *
 *  Created on: Sep 28, 2021
 *      Author: WCH
 */

#ifndef _LWNS_PROV_H_
#define _LWNS_PROV_H_

#include "lwns_config.h"
#include "rf_config_params.h"
#include "lwns_sec.h"

#define PROV_BC_PORT 1  //�����㲥ͨ���˿�
#define PROV_UC_PORT 2  //��������ͨ���˿�

#define PROV_MASTER 0 //�Ƿ�Ϊ�����豸��Ϊ0��Ϊ�ӻ��豸
#define BLE_PROV    1 //�Ƿ�ʹ��ʹ����������



#define PROV_BROADCAST_TX_PERIOD_EVT                 1<<(0)

void prov_process_init(void);

void prov_process_deinit(void);

uint8 lwns_provisioned(void);

EfErrCode lwns_set_provisioned(void);

EfErrCode lwns_provisioned_reset(void);

typedef enum {
    LWNS_PROV_CMD_TEST = 0,               //ͨ�Ų��ԣ���ʲô�ظ�ʲô
    LWNS_PROV_CMD_GET_PROV_STATE,         //��ȡ����״̬
    LWNS_PROV_CMD_GET_RF_CONFIG_PARAMS,   //��ȡ��ǰrf����
    LWNS_PROV_CMD_GET_RF_KEY,             //��ȡ��ǰrf������Կ
    LWNS_PROV_CMD_SET_RF_CONFIG_PARAMS,   //����rf��������
    LWNS_PROV_CMD_SET_RF_KEY,             //����rf����������Կ
    LWNS_PROV_CMD_PROV_ENABLE,            //��������״̬���ŻὫ֮ǰ���õ��������������豸
    LWNS_PROV_CMD_PROV_RESET,
    LWNS_PROV_CMD_MAX,
} LWNS_PROV_CMD_t;


extern void Rec_BLE_PROV_DataDeal(unsigned char *p_data, unsigned char w_len ,unsigned char *r_data, unsigned char* r_len);

#endif /* _LWNS_PROV_H_ */
