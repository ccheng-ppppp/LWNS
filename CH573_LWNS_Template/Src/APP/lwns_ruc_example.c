/*
 * lwns_ruc_example.c
   *   �ɿ�������������
 *  reliable unicast
   *    ʹ����Ҫȥapp_main.c��ȡ�������ӳ�ʼ��������ע��
 *  Created on: Jul 19, 2021
 *      Author: WCH
 */
#include "CH57x_common.h"
#include "config.h"
#include "lwns_ruc_example.h"

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

static lwns_ruc_controller ruc;//�����ɿ��������ƽṹ��

static uint8 TX_DATA[10] =
        { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };
static uint8 RX_DATA[10];

static uint8 ruc_taskID;
uint16 lwns_ruc_ProcessEvent(uint8 task_id, uint16 events);


void lwns_ruc_process_init(void);

static void recv_ruc(lwns_controller_ptr ptr,
        const lwns_addr_t* sender);

static void sent_ruc(lwns_controller_ptr ptr,
        const lwns_addr_t* to, uint8_t retransmissions);
static void timedout_ruc(lwns_controller_ptr ptr,
        const lwns_addr_t* to);




static void recv_ruc(lwns_controller_ptr ptr,
        const lwns_addr_t* sender) {
    //ruc�н��յ����͸��Լ������ݺ󣬲Ż���øûص�
    uint8 len;
    len = lwns_buffer_datalen(); //��ȡ��ǰ���������յ������ݳ���
    if (len == 10) {
        lwns_buffer_save_data(RX_DATA); //�������ݵ��û���������
        PRINTF("ruc %d rec %02x %02x %02x %02x %02x %02x\r\n",get_lwns_object_port(ptr), sender->u8[0],
                sender->u8[1], sender->u8[2], sender->u8[3], sender->u8[4], sender->u8[5]);
        PRINTF("data:");
        for (uint8 i = 0; i < len; i++) {
            PRINTF("%02x ", RX_DATA[i]);
        }
        PRINTF("\n");
    } else {
        PRINTF("data len err\n");
    }
}


static void sent_ruc(lwns_controller_ptr ptr,
        const lwns_addr_t* to, uint8_t retransmissions) {
    //ruc�з��ͳɹ��������յ�Ŀ��ڵ��ack�ظ��󣬲Ż���øûص�
    PRINTF("ruc %d sent %d\r\n",get_lwns_object_port(ptr),retransmissions);
    tmos_start_task(ruc_taskID, RUC_EXAMPLE_TX_PERIOD_EVT,
                    MS1_TO_SYSTEM_TIME(1000));//��������ʱ�䣬���Ͳ��յ��ظ���1���Ӻ��ٷ���
}
static void timedout_ruc(lwns_controller_ptr ptr,
        const lwns_addr_t* to) {
    //ruc�У����ط�������������ط������󣬻���øûص���
    PRINTF("ruc %d timedout\n",get_lwns_object_port(ptr));
    tmos_start_task(ruc_taskID, RUC_EXAMPLE_TX_PERIOD_EVT,
                    MS1_TO_SYSTEM_TIME(1000));
}




static const struct lwns_ruc_callbacks ruc_callbacks = {
        recv_ruc, sent_ruc, timedout_ruc };//�����ɿ������ص��ṹ��

void lwns_ruc_process_init(void) {
    lwns_ruc_init(&ruc,
            144,//��һ���˿ں�Ϊ144�Ŀɿ�����
            HTIMER_SECOND_NUM,//�ȴ�ackʱ������û�յ��ͻ��ط�
            &ruc_callbacks);//����0�����ʧ�ܡ�����1�򿪳ɹ���
    ruc_taskID = TMOS_ProcessEventRegister(lwns_ruc_ProcessEvent);
    tmos_start_task(ruc_taskID, RUC_EXAMPLE_TX_PERIOD_EVT,
                MS1_TO_SYSTEM_TIME(1000));
}


uint16 lwns_ruc_ProcessEvent(uint8 task_id, uint16 events) {
    if (events & RUC_EXAMPLE_TX_PERIOD_EVT) {
        uint8 temp;
        temp = TX_DATA[0];
        for (uint8 i = 0; i < 9; i++) {
            TX_DATA[i] = TX_DATA[i + 1];//��λ�������ݣ��Ա�۲�Ч��
        }
        TX_DATA[9] = temp;
        lwns_buffer_load_data(TX_DATA, sizeof(TX_DATA));//������Ҫ���͵����ݵ�������
        lwns_ruc_send(&ruc,
                &dst_addr,//�ɿ�����Ŀ���ַ
                4//����ط�����
                );//�ɿ��������ͺ��������Ͳ�����Ŀ���ַ������ط�������Ĭ��һ�����ط�һ��
        return events ^ RUC_EXAMPLE_TX_PERIOD_EVT;
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
