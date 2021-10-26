/*
 * lwns_sec.c
   *  ��Ϣ����
 *  Created on: Sep 17, 2021
 *      Author: WCH
 */

#include "lwns_sec.h"
#include "config.h"

static unsigned char lwns_sec_key[16]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};//�û����и���Ϊ�Լ�����Կ�����߸�Ϊ���Դ�������ȡ�����洢��eeprom��

//������Ϣ����src��ʼ��mlen���ֽ����ݣ����ܵ�toָ����ڴ�ռ䡣
int lwns_msg_encrypt(u8 *src,u8 *to,u8 mlen){
    unsigned short i = 0;
    unsigned char esrc[16] = {0};
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

//������Ϣ����src��ʼ��mlen���ֽ����ݣ����ܵ�toָ����ڴ�ռ䡣
int lwns_msg_decrypt(u8 *src,u8 *to,u8 mlen){//����mlen����Ϊ16�ı���
    unsigned short i = 0;
    while(1){
        LL_Decrypt(lwns_sec_key, src + i, to + i);
        i+=16;
        if(i >= mlen){
            break;
        }
    }
    return i;
}
