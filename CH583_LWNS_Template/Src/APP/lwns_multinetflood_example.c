/*
 * lwns_multinetflood_example.c
   *    ʹ����Ҫȥapp_main.c��ȡ�������ӳ�ʼ��������ע��
   *   �鲥���緺�鴫�����ӣ������ݷ������ĵĵ�ַ�����ĵĵ�ַΪ2�ֽ�u16���͡�
 * multinetflood
 *  Created on: Jul 19, 2021
 *      Author: WCH
 */

#include "lwns_multinetflood_example.h"
#include "CH58x_common.h"
#include "config.h"

#define DEBUG_PRINT_IN_THIS_FILE 1
#if DEBUG_PRINT_IN_THIS_FILE
#define PRINTF(...) PRINT(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif


static u8 subaddrs_index = 0;//���Ͷ��ĵ�ַ���
#define SUBADDR_NUM     3//���ĵ�ַ����
static uint16 subaddrs[SUBADDR_NUM]={1,2,3};//���ĵ�ַ����

static uint8 TX_DATA[LWNS_DATA_SIZE] = {0};//��󳤶������շ�����
static uint8 RX_DATA[LWNS_DATA_SIZE] = {0};//��󳤶������շ�����
static uint16 lwns_multinetflood_ProcessEvent(uint8 task_id, uint16 events);
static void multinetflood_recv(lwns_controller_ptr ptr,u16 subaddr,const lwns_addr_t *sender, uint8_t hops);//�鲥���緺����ջص�����
static void multinetflood_sent(lwns_controller_ptr ptr);//�鲥���緺�鷢����ɻص�����

static lwns_multinetflood_controller multinetflood;//�����鲥���緺����ƽṹ��

static uint8 multinetflood_taskID;//�鲥���緺���������id

static void multinetflood_recv(lwns_controller_ptr ptr,u16 subaddr,const lwns_addr_t *sender, uint8_t hops){
    uint8 len;
    len = lwns_buffer_datalen(); //��ȡ��ǰ���������յ������ݳ���
    lwns_buffer_save_data(RX_DATA); //�������ݵ��û���������
    PRINTF("multinetflood %d rec from %02x %02x %02x %02x %02x %02x\n",
            get_lwns_object_port(ptr),
            sender->u8[0], sender->u8[1], sender->u8[2], sender->u8[3],
            sender->u8[4], sender->u8[5]);//fromΪ���յ������ݵķ��ͷ���ַ
    PRINTF("subaddr:%d,data:",subaddr);
    for (uint8 i = 0; i < len; i++) {
        PRINTF("%02x ", RX_DATA[i]);//��ӡ������
    }
    PRINTF("\n");
}

static void multinetflood_sent(lwns_controller_ptr ptr) {
    PRINTF("multinetflood %d sent\n",get_lwns_object_port(ptr));
}

static const struct lwns_multinetflood_callbacks multinetflood_callbacks =
{multinetflood_recv,multinetflood_sent};//ע���鲥���緺��ص�����


void lwns_multinetflood_process_init(void) {
    multinetflood_taskID = TMOS_ProcessEventRegister(lwns_multinetflood_ProcessEvent);
    for(uint8 i = 0; i < LWNS_DATA_SIZE ;i++){
        TX_DATA[i]=i;
    }
    lwns_multinetflood_init(&multinetflood,
                137,//��һ���˿ں�Ϊ137���鲥���緺��ṹ��
                HTIMER_SECOND_NUM,//���ȴ�ת��ʱ��
                1,//�ڵȴ��ڼ䣬���յ�����ͬ�������ݰ���ȡ�������ݰ��ķ���
                3,//���ת���㼶
                FALSE,//�ڵȴ�ת�������У��յ����µ���Ҫת�������ݰ��������ݰ������̷��ͳ�ȥ���Ƕ�����FALSEΪ���̷��ͣ�TRUEΪ������
                20,//����ָ���������ֵ������һ����࣬�������ű��ڴ��ڱ�������ݰ����С��ֵ���ڴ�ֵ�������Ϊ������ϻָ����������ո����ݰ���
                //ͬʱ����ֵҲ�������ж�Ϊ�����ݰ��Ĳ�ֵ��������ͬһ���ڵ�������ݰ�����Ų����Ա��ڴ��еĴ���࣬���ȴ�ֵ����
                //���磬�ڴ��б����Ϊ10�������ݰ����Ϊ50����ֵΪ40�����ڴ�ʱ���õ�20�����Խ����ᱻ��ΪΪ�µ����ݰ�����������
                //ֻ�����Ϊ29����ֵΪ19��С�ڸ�ֵ���Żᱻ���ա�
                TRUE,//�����Ƿ�ת��Ŀ��Ǳ��������ݰ�������������mesh�Ƿ�����relay���ܡ�
                subaddrs,//���ĵĵ�ַ����ָ��
                SUBADDR_NUM,//���ĵ�ַ����
                &multinetflood_callbacks
                );//����0�����ʧ�ܡ�����1�򿪳ɹ���
#if 1
    tmos_start_task(multinetflood_taskID, MULTINETFLOOD_EXAMPLE_TX_PERIOD_EVT,
            MS1_TO_SYSTEM_TIME(1000));
#endif
}

uint16 lwns_multinetflood_ProcessEvent(uint8 task_id, uint16 events) {
    if (events & MULTINETFLOOD_EXAMPLE_TX_PERIOD_EVT) {
       uint8 temp;
       temp = TX_DATA[0];
       for (uint8 i = 0; i < 9; i++) {
           TX_DATA[i] = TX_DATA[i + 1];//��λ�������ݣ��Ա�۲�Ч��
       }
       TX_DATA[9] = temp;
       lwns_buffer_load_data(TX_DATA, sizeof(TX_DATA));//������Ҫ���͵����ݵ�������
       if(subaddrs_index >= SUBADDR_NUM){
          subaddrs_index = 0;
       }
       lwns_multinetflood_send(&multinetflood,subaddrs[subaddrs_index]);//�鲥���緺�鷢�����ݵ����ĵ�ַ
       subaddrs_index++;

       tmos_start_task(multinetflood_taskID, MULTINETFLOOD_EXAMPLE_TX_PERIOD_EVT,
               MS1_TO_SYSTEM_TIME(1000));//�����Է���
       return events ^ MULTINETFLOOD_EXAMPLE_TX_PERIOD_EVT;
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

