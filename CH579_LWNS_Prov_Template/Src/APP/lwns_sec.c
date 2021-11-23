/*
 * lwns_sec.c
   *  ��Ϣ����
 *  Created on: Sep 17, 2021
 *      Author: WCH
 */

#include "lwns_sec.h"
#include "config.h"

#define DEBUG_PRINT_IN_THIS_FILE 1
#if DEBUG_PRINT_IN_THIS_FILE
#define PRINTF(...) PRINT(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif


//�û��ڴ��޸ģ�ΪĬ�ϵ�lwns_sec_key��������һ�����г���flash�в�����д˲���������Ĭ�ϲ��ô˲������м��ܽ���
//ͨ���������û����µ�lwns_sec_key����д��flash�У��������������
static uint8_t lwns_sec_key[16]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};//�û����и���Ϊ�Լ�����Կ�����߸�Ϊ���Դ�������ȡ�����洢��flash��

__attribute__((aligned(4)))  const char lwns_rf_key_index[]={"skey"};

/*********************************************************************
 * @fn      lwns_sec_init
 *
 * @brief   lwns��Ϣ������Կ��ʼ������flash�ж�ȡ����Կ
 *
 * @param   None.
 *
 * @return  None.
 */
void lwns_sec_init(void){
    size_t  len;
    ef_get_env_blob(lwns_rf_key_index, NULL, 0, &len);
    if(len == 16){
        ef_get_env_blob(lwns_rf_key_index, lwns_sec_key, 16, NULL);
    } else {
        PRINTF("use default rf_key\n");
    }
    PRINTF("rf sec key:%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\n"
            ,lwns_sec_key[0],lwns_sec_key[1],lwns_sec_key[2],lwns_sec_key[3]
            ,lwns_sec_key[4],lwns_sec_key[5],lwns_sec_key[6],lwns_sec_key[7]
            ,lwns_sec_key[8],lwns_sec_key[9],lwns_sec_key[10],lwns_sec_key[11]
            ,lwns_sec_key[12],lwns_sec_key[13],lwns_sec_key[14],lwns_sec_key[15]);
}

/*********************************************************************
 * @fn      lwns_msg_encrypt
 *
 * @brief   lwns��Ϣ����
 *
 * @param   src     -   �����ܵ����ݻ���ͷָ��.
 * @param   to      -   ���洢�������ݵ����ݻ�����ͷָ��.
 * @param   mlen    -   �����ܵ����ݳ���.
 *
 * @return  ���ܺ�����ݳ���.
 */
int lwns_msg_encrypt(uint8_t *src,uint8_t *to,uint8_t mlen){
    uint16_t i = 0;
    uint8_t esrc[16] = {0};
    while(1){
        if((mlen - i) < 16){
            tmos_memcpy(esrc, src + i, (mlen - i));//���䵽16�ֽڣ�����Ϊ0
            LL_Encrypt(lwns_sec_key, esrc , to + i);
        } else {
            LL_Encrypt(lwns_sec_key, src + i , to + i);
        }
        i+=16;
        if(i >= mlen){
            break;
        }
    }
    return i;//���ؼ��ܺ����ݳ���
}

/*********************************************************************
 * @fn      lwns_msg_decrypt
 *
 * @brief   lwns��Ϣ����
 *
 * @param   src     -   �����ܵ����ݻ���ͷָ��.
 * @param   to      -   ���洢�������ݵ����ݻ�����ͷָ��.
 * @param   mlen    -   �����ܵ����ݳ���.
 *
 * @return  ���ܺ�����ݳ���.
 */
int lwns_msg_decrypt(uint8_t *src,uint8_t *to,uint8_t mlen){//����mlen����Ϊ16�ı���
    uint16_t i = 0;
    while(1){
        LL_Decrypt(lwns_sec_key, src + i, to + i);
        i+=16;
        if(i >= mlen){
            break;
        }
    }
    return i;
}

/*********************************************************************
 * @fn      lwns_temp_set_key
 *
 * @brief   lwns������ʱ��Կ��������flash
 *
 * @param   keyValue     -   �����õ���Կֵ����ͷָ��.
 *
 * @return  None.
 */
void lwns_temp_set_key(uint8_t* keyValue)
{
    tmos_memcpy(lwns_sec_key, keyValue, 16);
}

/*********************************************************************
 * @fn      lwns_get_key
 *
 * @brief   lwns��ȡ��Կ
 *
 * @param   to     -   ��Կ�����浽�Ļ�����ͷָ��.
 *
 * @return  None.
 */
void lwns_get_key(uint8_t* to){
    tmos_memcpy(to, lwns_sec_key, 16);
}

/*********************************************************************
 * @fn      lwns_save_key_to_flash
 *
 * @brief   lwns����ʱ��Կ���浽flash
 *
 * @param   to     -   ��Կ�����浽�Ļ�����ͷָ��.
 *
 * @return  defined in EfErrCode.
 */
EfErrCode lwns_save_key_to_flash(void)
{
    EfErrCode err;
    err = ef_set_env_blob(lwns_rf_key_index, lwns_sec_key, 16);
    if(err != EF_NO_ERR){
        PRINTF("set rf key err\n");
    } else {
        PRINTF("set rf key ok\n");
    }
    return err;
}
