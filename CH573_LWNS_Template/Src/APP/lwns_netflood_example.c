/*
 * lwns_netflood_example.c
   *   ���緺��
   *    ʹ����Ҫȥapp_main.c��ȡ�������ӳ�ʼ��������ע��
 *  Created on: Jul 20, 2021
 *      Author: WCH
 */
#include "CH57x_common.h"
#include "lwns_netflood_example.h"
#include "config.h"

#define DEBUG_PRINT_IN_THIS_FILE 1
#if DEBUG_PRINT_IN_THIS_FILE
#define PRINTF(...) PRINT(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif


static uint8 TX_DATA[LWNS_DATA_SIZE] = {0};//��󳤶������շ�����
static uint8 RX_DATA[LWNS_DATA_SIZE] = {0};//��󳤶������շ�����
static uint16 lwns_netflood_ProcessEvent(uint8 task_id, uint16 events);
static int netflood_recv(lwns_controller_ptr ptr,
        const lwns_addr_t *from,
        const lwns_addr_t *originator, uint8_t hops);
static void netflood_sent(lwns_controller_ptr ptr);
static void netflood_dropped(lwns_controller_ptr ptr);

static const struct lwns_netflood_callbacks callbacks = { netflood_recv,
        netflood_sent, netflood_dropped };

static uint8 netflood_taskID;

void lwns_netflood_process_init(void);

static lwns_netflood_controller netflood;//���緺����ƽṹ��

static int netflood_recv(lwns_controller_ptr ptr,
        const lwns_addr_t *from,
        const lwns_addr_t *originator, uint8_t hops) {
    uint8 len;
    len = lwns_buffer_datalen(); //��ȡ��ǰ���������յ������ݳ���
    PRINTF("netflood %d rec %02x %02x %02x %02x %02x %02x,hops=%d\r\n",get_lwns_object_port(ptr),
            from->u8[0], from->u8[1], from->u8[2], from->u8[3], from->u8[4],
            from->u8[5], hops);//��ӡת���ߣ���Ϊ�յ��ı���ת��������˭ת���ġ�
    PRINTF("netflood orec %02x %02x %02x %02x %02x %02x,hops=%d\r\n",
            originator->u8[0], originator->u8[1], originator->u8[2],
            originator->u8[3], originator->u8[4], originator->u8[5],
            hops);//��ӡ����Ϣ�����ߣ���Ϊ���𱾴����緺��Ľڵ��ַ��
    lwns_buffer_save_data(RX_DATA); //�������ݵ��û���������
    PRINTF("data:");
    for (uint8 i = 0; i < len; i++) {
        PRINTF("%02x ", RX_DATA[i]);
    }
    PRINTF("\n");
    return 1;//����1���򱾽ڵ㽫��������netflood��ת������
    //return 0;//����0���򱾽ڵ㲻�ټ���netflood��ֱ����ֹ
}
static void netflood_sent(lwns_controller_ptr ptr) {
    PRINTF("netflood %d sent\n",get_lwns_object_port(ptr));
}
static void netflood_dropped(lwns_controller_ptr ptr) {
    PRINTF("netflood %d dropped\n",get_lwns_object_port(ptr));
}

void lwns_netflood_process_init(void) {
    netflood_taskID = TMOS_ProcessEventRegister(lwns_netflood_ProcessEvent);
    for(uint8 i = 0; i < LWNS_DATA_SIZE ;i++){
        TX_DATA[i]=i;
    }
    lwns_netflood_init(&netflood,
            137,//��һ���˿ں�Ϊ137�ķ���ṹ��
            HTIMER_SECOND_NUM*1,//�ȴ�ת��ʱ��
            1,//�ڵȴ��ڼ䣬���յ�����ͬ�������ݰ���ȡ�������ݰ��ķ���
            3,//���ת���㼶
            FALSE,//�ڵȴ�ת�������У��յ����µ���Ҫת�������ݰ��������ݰ������̷��ͳ�ȥ���Ƕ�����FALSEΪ���̷��ͣ�TRUEΪ������
            50,//����ָ���������ֵ������һ����࣬�������ű��ڴ��ڱ�������ݰ����С��ֵ���ڴ�ֵ�������Ϊ������ϻָ����������ո����ݰ���
            //ͬʱ����ֵҲ�������ж�Ϊ�����ݰ��Ĳ�ֵ��������ͬһ���ڵ�������ݰ�����Ų����Ա��ڴ��еĴ���࣬���ȴ�ֵ����
            //���磬�ڴ��б����Ϊ10�������ݰ����Ϊ60����ֵΪ50�����ڵ��ڴ�ʱ���õ�50�����Խ����ᱻ��ΪΪ�µ����ݰ�����������
            //ֻ�����Ϊ59����ֵΪ49��С�ڸ�ֵ���Żᱻ���ա�
            &callbacks);//����0�����ʧ�ܡ�����1�򿪳ɹ���
#if 1
    tmos_start_task(netflood_taskID, NETFLOOD_EXAMPLE_TX_PERIOD_EVT,
            MS1_TO_SYSTEM_TIME(1000));
#endif
}

static uint16 lwns_netflood_ProcessEvent(uint8 task_id, uint16 events) {
    if (events & NETFLOOD_EXAMPLE_TX_PERIOD_EVT) {
        PRINTF("send\n");
        lwns_buffer_load_data(TX_DATA, sizeof(TX_DATA)); //������Ҫ���͵����ݵ�������
        lwns_netflood_send(&netflood); //�������緺�����ݰ�
        tmos_start_task(netflood_taskID, NETFLOOD_EXAMPLE_TX_PERIOD_EVT,
                MS1_TO_SYSTEM_TIME(3000)); //10s����һ��
        return (events ^ NETFLOOD_EXAMPLE_TX_PERIOD_EVT);
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
