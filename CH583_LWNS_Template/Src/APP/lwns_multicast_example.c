/*
 * lwns_multicast_example.c
   *    ʹ����Ҫȥapp_main.c��ȡ�������ӳ�ʼ��������ע��
   *   �鲥�������ӣ������ݷ��͵�һ�����ڵ��豸
   *   single-hop multicast
 *  Created on: Jul 19, 2021
 *      Author: WCH
 */

#include "lwns_multicast_example.h"
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

static uint8 TX_DATA[10] =
        { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };
static uint8 RX_DATA[10];
static uint16 lwns_multicast_ProcessEvent(uint8 task_id, uint16 events);
static void multicast_recv(lwns_controller_ptr c, u16 subaddr, const lwns_addr_t *sender);//�鲥���ջص�����
static void multicast_sent(lwns_controller_ptr ptr);//�鲥������ɻص�����

static lwns_multicast_controller multicast;//�����鲥���ƽṹ��

static uint8 multicast_taskID;//�����鲥��������id

static void multicast_recv(lwns_controller_ptr ptr, u16 subaddr,  const lwns_addr_t *sender){
    uint8 len;
    len = lwns_buffer_datalen(); //��ȡ��ǰ���������յ������ݳ���
    if (len == 10) {
        lwns_buffer_save_data(RX_DATA); //�������ݵ��û���������
        PRINTF("multicast %d rec from %02x %02x %02x %02x %02x %02x\n",
                get_lwns_object_port(ptr),
                sender->u8[0], sender->u8[1], sender->u8[2], sender->u8[3],
                sender->u8[4], sender->u8[5]);//fromΪ���յ������ݵķ��ͷ���ַ
        PRINTF("subaddr:%d data:",subaddr);
        for (uint8 i = 0; i < len; i++) {
            PRINTF("%02x ", RX_DATA[i]);
        }
        PRINTF("\n");
    } else {
        PRINTF("data len err\n");
    }
}

static void multicast_sent(lwns_controller_ptr ptr) {
    PRINTF("multicast %d sent\n",get_lwns_object_port(ptr));
}

static const struct lwns_multicast_callbacks multicast_callbacks =
{multicast_recv,multicast_sent};//ע��ص�����


void lwns_multicast_process_init(void) {
    multicast_taskID = TMOS_ProcessEventRegister(lwns_multicast_ProcessEvent);
    lwns_multicast_init(&multicast,
            136,//��һ���˿ں�Ϊ136���鲥
            subaddrs,//���ĵ�ַ����ָ��
            SUBADDR_NUM,//���ĵ�ַ����
            &multicast_callbacks
            );//����0�����ʧ�ܡ�����1�򿪳ɹ���
    tmos_start_task(multicast_taskID, MULTICAST_EXAMPLE_TX_PERIOD_EVT,
            MS1_TO_SYSTEM_TIME(1000));
}

uint16 lwns_multicast_ProcessEvent(uint8 task_id, uint16 events) {
    if (events & MULTICAST_EXAMPLE_TX_PERIOD_EVT) {//�������ڲ�ͬ���鲥��ַ�Ϸ����鲥��Ϣ
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
        lwns_multicast_send(&multicast,subaddrs[subaddrs_index]);//�鲥�������ݸ�ָ���ڵ�
        subaddrs_index++;
        tmos_start_task(multicast_taskID, MULTICAST_EXAMPLE_TX_PERIOD_EVT,
                MS1_TO_SYSTEM_TIME(1000));//�����Է���
        return events ^ MULTICAST_EXAMPLE_TX_PERIOD_EVT;
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

