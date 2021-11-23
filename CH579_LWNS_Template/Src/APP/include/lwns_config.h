/*
 * lwns_config.h
 *
 *  Created on: Nov 1, 2021
 *      Author: WCH
 */

#ifndef _LWNS_CONFIG_H_
#define _LWNS_CONFIG_H_

#include "config.h"
#include "CH57x_common.h"
#include "WCH_LWNS_LIB.h"

#define LWNS_ADDR_USE_BLE_MAC           1       //�Ƿ�ʹ������Ӳ����mac��ַ��ΪĬ��lwns��ַ

#define LWNS_ENCRYPT_ENABLE             1       //�Ƿ�ʹ�ܼ���

#define QBUF_MANUAL_NUM                 4       //qbuf������������

#define ROUTE_ENTRY_MANUAL_NUM          32      //·����Ŀ��������

#define LWNS_NEIGHBOR_MAX_NUM           8       //����ھ�����

#endif /* _LWNS_CONFIG_H_ */
