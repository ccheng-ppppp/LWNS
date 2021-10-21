/*
 * lwns_uninetflood_example.c
   *    ʹ����Ҫȥapp_main.c��ȡ�������ӳ�ʼ��������ע��
   *   �������緺�鴫�����ӣ����������緺�鷢����ָ���ڵ㡣
 * uninetflood
 *  Created on: Jul 19, 2021
 *      Author: WCH
 */

#include "lwns_uninetflood_example.h"
#include "CH57x_common.h"
#include "config.h"

#define DEBUG_PRINT_IN_THIS_FILE 1
#if DEBUG_PRINT_IN_THIS_FILE
#define PRINTF(...) PRINT(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif

#if 1
static lwns_addr_t dst_addr = { { 0xa3, 0xdf, 0x38, 0xe4, 0xc2, 0x84 } };//Ŀ��ڵ��ַ������ʱ������ݵ�·��оƬMAC��ַ��ͬ�����޸ġ��޸�Ϊ���շ���MAC��ַ������ʹ���Լ���MAC��ַ
#else
static lwns_addr_t dst_addr = { { 0xd9, 0x37, 0x3c, 0xe4, 0xc2, 0x84 } };
#endif

static uint8 TX_DATA[LWNS_DATA_SIZE] = {0};//��󳤶������շ�����
static uint8 RX_DATA[LWNS_DATA_SIZE] = {0};//��󳤶������շ�����
static uint16 lwns_uninetflood_ProcessEvent(uint8 task_id, uint16 events);
static void uninetflood_recv(lwns_controller_ptr ptr,const lwns_addr_t *sender, uint8_t hops);//�������緺����ջص�����
static void uninetflood_sent(lwns_controller_ptr ptr);//�������緺�鷢����ɻص�����

static lwns_uninetflood_controller uninetflood;//�������緺����ƽṹ��

static uint8 uninetflood_taskID;//�������緺���������id

static void uninetflood_recv(lwns_controller_ptr ptr,const lwns_addr_t *sender, uint8_t hops){
    uint8 len;
    len = lwns_buffer_datalen(); //��ȡ��ǰ���������յ������ݳ���
    lwns_buffer_save_data(RX_DATA); //�������ݵ��û���������
    PRINTF("uninetflood %d rec from %02x %02x %02x %02x %02x %02x\n",
            get_lwns_object_port(ptr),
            sender->u8[0], sender->u8[1], sender->u8[2], sender->u8[3],
            sender->u8[4], sender->u8[5]);//fromΪ���յ������ݵķ��ͷ���ַ
    PRINTF("data:");
    for (uint8 i = 0; i < len; i++) {
        PRINTF("%02x ", RX_DATA[i]);//��ӡ������
    }
    PRINTF("\n");
}

static void uninetflood_sent(lwns_controller_ptr ptr) {
    PRINTF("uninetflood %d sent\n",get_lwns_object_port(ptr));
}

static const struct lwns_uninetflood_callbacks uninetflood_callbacks =
{uninetflood_recv,uninetflood_sent};//ע�ᵥ�����緺��ص�����


void lwns_uninetflood_process_init(void) {
    uninetflood_taskID = TMOS_ProcessEventRegister(lwns_uninetflood_ProcessEvent);
    for(uint8 i = 0; i < LWNS_DATA_SIZE ;i++){
        TX_DATA[i]=i;
    }
    lwns_uninetflood_init(&uninetflood,
                137,//��һ���˿ں�Ϊ137�ĵ������緺��ṹ��
                HTIMER_SECOND_NUM*2,//���ȴ�ת��ʱ��
                1,//���յ��������ݰ���ȡ�����ͣ�Ҫ�ۺϿ��Ǻ��ط��������ص�����
                3,//���ת���㼶
                FALSE,//�ڵȴ�ת�������У��յ����µ���Ҫת�������ݰ��������ݰ������̷��ͳ�ȥ���Ƕ�����FALSEΪ���̷��ͣ�TRUEΪ������
                20,//����ָ���������ֵ������һ����࣬�������ű��ڴ��ڱ�������ݰ����С��ֵ���ڴ�ֵ�������Ϊ������ϻָ����������ո����ݰ���
                //ͬʱ����ֵҲ�������ж�Ϊ�����ݰ��Ĳ�ֵ��������ͬһ���ڵ�������ݰ�����Ų����Ա��ڴ��еĴ���࣬���ȴ�ֵ����
                //���磬�ڴ��б����Ϊ10�������ݰ����Ϊ50����ֵΪ40�����ڴ�ʱ���õ�20�����Խ����ᱻ��ΪΪ�µ����ݰ�����������
                //ֻ�����Ϊ29����ֵΪ19��С�ڸ�ֵ���Żᱻ���ա�
                TRUE,//�����Ƿ�ת��Ŀ��Ǳ��������ݰ�������������mesh�Ƿ�����relay���ܡ�
                &uninetflood_callbacks
                );//����0�����ʧ�ܡ�����1�򿪳ɹ���
    tmos_start_task(uninetflood_taskID, UNINETFLOOD_EXAMPLE_TX_PERIOD_EVT,
            MS1_TO_SYSTEM_TIME(1000));
}

uint16 lwns_uninetflood_ProcessEvent(uint8 task_id, uint16 events) {
    if (events & UNINETFLOOD_EXAMPLE_TX_PERIOD_EVT) {
       uint8 temp;
       temp = TX_DATA[0];
       for (uint8 i = 0; i < 9; i++) {
           TX_DATA[i] = TX_DATA[i + 1];//��λ�������ݣ��Ա�۲�Ч��
       }
       TX_DATA[9] = temp;
       lwns_buffer_load_data(TX_DATA, sizeof(TX_DATA));//������Ҫ���͵����ݵ�������
       lwns_uninetflood_send(&uninetflood,&dst_addr);//�������緺�鷢�����ݸ�Ŀ���ַ
       tmos_start_task(uninetflood_taskID, UNINETFLOOD_EXAMPLE_TX_PERIOD_EVT,
               MS1_TO_SYSTEM_TIME(1000));//�����Է���
       return events ^ UNINETFLOOD_EXAMPLE_TX_PERIOD_EVT;
   }

    if (events & SYS_EVENT_MSG) {
        uint8 *pMsg;
        if ((pMsg = tmos_msg_receive(task_id)) != NULL) {
            // Release the TMOS message
            tmos_msg_deallocate(pMsg);
        }
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }

    return 0;
}

