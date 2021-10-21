/*
 * lwns_broadcast_example.c
   *   �㲥��������
   *    ʹ����Ҫȥapp_main.c��ȡ�������ӳ�ʼ��������ע��
 *  Created on: Jul 19, 2021
 *      Author: WCH
 */

#include "lwns_broadcast_example.h"
#include "CH57x_common.h"
#include "config.h"

#define DEBUG_PRINT_IN_THIS_FILE 1
#if DEBUG_PRINT_IN_THIS_FILE
#define PRINTF(...) PRINT(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif

//�û���Ҫ���͵����ݻ�����
static uint8 TX_DATA[10] =
        { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };

static uint8 RX_DATA[10];//�û��������ݵĻ�����
static uint16 lwns_broadcast_ProcessEvent(uint8 task_id, uint16 events);
static void broadcast_recv(lwns_controller_ptr ptr,
        const lwns_addr_t* sender);//���ջص���������
static void broadcast_sent(lwns_controller_ptr ptr);//���ͻص���������

static lwns_broadcast_controller broadcast;//�㲥���ƽṹ��

static uint8 broadcast_taskID;

static void broadcast_recv(lwns_controller_ptr ptr,
        const lwns_addr_t* sender) {
    uint8 len;
    len = lwns_buffer_datalen(); //��ȡ��ǰ���������յ������ݳ���
    if (len == 10) {
        lwns_buffer_save_data(RX_DATA); //�������ݵ��û���������
        PRINTF("broadcast %d rec from %02x %02x %02x %02x %02x %02x\n",get_lwns_object_port(ptr),
                sender->u8[0], sender->u8[1], sender->u8[2], sender->u8[3],
                sender->u8[4], sender->u8[5]);//��ӡ��������Ϣ�ķ����ߵ�ַ
        PRINTF("data:");
        for (uint8 i = 0; i < len; i++) {
            PRINTF("%02x ", RX_DATA[i]);//��ӡ����
        }
        PRINTF("\n");

    } else {
        PRINTF("data len err\n");
    }
}

static void broadcast_sent(lwns_controller_ptr ptr) {
    PRINTF("broadcast %d sent\n",get_lwns_object_port(ptr));//��ӡ���������Ϣ
}

static const struct lwns_broadcast_callbacks broadcast_call = {
        broadcast_recv, broadcast_sent };//�����㲥�ص������ṹ�壬ע��ص�����

void lwns_broadcast_process_init(void) {
    broadcast_taskID = TMOS_ProcessEventRegister(lwns_broadcast_ProcessEvent);
    lwns_broadcast_init(&broadcast, 136, &broadcast_call); //��һ���˿ں�Ϊ136�Ĺ㲥�˿ڣ�ע��ص�����
    tmos_start_task(broadcast_taskID, BROADCAST_EXAMPLE_TX_PERIOD_EVT,
            MS1_TO_SYSTEM_TIME(1000));//��ʼ�����Թ㲥����
}

uint16 lwns_broadcast_ProcessEvent(uint8 task_id, uint16 events) {
    if (events & BROADCAST_EXAMPLE_TX_PERIOD_EVT) {
        uint8 temp;
        temp = TX_DATA[0];
        for (uint8 i = 0; i < 9; i++) {
            TX_DATA[i] = TX_DATA[i + 1];//��λ�������ݣ��Ա�۲�Ч��
        }
        TX_DATA[9] = temp;
        lwns_buffer_load_data(TX_DATA, sizeof(TX_DATA));//������Ҫ���͵����ݵ�������
        lwns_broadcast_send(&broadcast);//�㲥��������
        tmos_start_task(broadcast_taskID, BROADCAST_EXAMPLE_TX_PERIOD_EVT,
                MS1_TO_SYSTEM_TIME(1000));//�����Է���

        return events ^ BROADCAST_EXAMPLE_TX_PERIOD_EVT;
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

