/*
 * lwns_addr_manage.c
 * lwns��ַ�������浽dataflash��
 *
 *  Created on: Oct 15, 2021
 *      Author: WCH
 */
#include "lwns_addr_manage.h"
#include "lwns_adapter_csma_mac.h"
#include "lwns_adapter_blemesh_mac.h"
#include "lwns_adapter_no_mac.h"

#define DEBUG_PRINT_IN_THIS_FILE 1
#if DEBUG_PRINT_IN_THIS_FILE
#define PRINTF(...) PRINT(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif


__attribute__((aligned(4)))  const char lwns_addr_index[]={"lwad"};

void lwns_addr_init(lwns_addr_t *to){
    size_t  len;
    ef_get_env_blob(lwns_addr_index, NULL, 0, &len);
    if(len == LWNS_ADDR_SIZE){
        PRINTF("use addr in flash\n");
        ef_get_env_blob(lwns_addr_index, to, LWNS_ADDR_SIZE, NULL);
    } else {
        PRINTF("use default addr\n");
#if LWNS_ADDR_USE_BLE_MAC
        GetMACAddress(to->u8); //��ȡ����оƬmac��ַ����ΪLWNS��ĵ�ַ
#else
        //���ж���ĵ�ַ
        uint8 MacAddr[6] = {0,0,0,0,0,1};
        tmos_memcpy(to->u8, MacAddr, LWNS_ADDR_SIZE);
#endif
    }
    PRINTF("lwns addr: %02x %02x %02x %02x %02x %02x\n",
            to->u8[0], to->u8[1], to->u8[2], to->u8[3],
            to->u8[4], to->u8[5]);//��ӡ��lwns��ַ
}

EfErrCode lwns_addr_save_to_flash(lwns_addr_t *from)
{
    EfErrCode err;
    err = ef_set_env_blob(lwns_addr_index, from, LWNS_ADDR_SIZE);
    if(err != EF_NO_ERR){
        PRINTF("set lwns addr err\n");
    } else {
        PRINTF("set lwns addr ok\n");
    }
    return err;
}
