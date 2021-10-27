/*
 * lwns_rucft_example.c
   *   �ɿ������鴫������
 *   reliable unicast file transfer
   *    ʹ����Ҫȥapp_main.c��ȡ�������ӳ�ʼ��������ע��
 *  Created on: Jul 19, 2021
 *      Author: WCH
 */
#include "lwns_rucft_example.h"
#include "CH58x_common.h"
#include "config.h"

#define DEBUG_PRINT_IN_THIS_FILE 1
#if DEBUG_PRINT_IN_THIS_FILE
#define PRINTF(...) PRINT(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif

#if 1
static lwns_addr_t dst_addr = { { 0x66, 0xdf, 0x38, 0xe4, 0xc2, 0x84 } }; //Ŀ��ڵ��ַ������ʱ������ݵ�·��оƬMAC��ַ��ͬ�����޸ġ��޸�Ϊ���շ���MAC��ַ������ʹ���Լ���MAC��ַ
#else
static lwns_addr_t dst_addr = { { 0xd9, 0x37, 0x3c, 0xe4, 0xc2, 0x84 } };
#endif

static uint8 rucft_taskID;

static lwns_rucft_controller rucft; //����rucft���ƽṹ��

#define FILESIZE 4000
static char strsend[FILESIZE]; //���ͻ�����
static char *strp;
static void write_file(lwns_controller_ptr ptr, const lwns_addr_t *sender,
        int offset, int flag, char *data, int datalen);
static int read_file(lwns_controller_ptr ptr, int offset, char *to,
        int maxsize);
static void timedout_rucft(lwns_controller_ptr ptr);

const static struct lwns_rucft_callbacks rucft_callbacks = { write_file,
        read_file, timedout_rucft };

uint16 lwns_rucft_ProcessEvent(uint8 task_id, uint16 events);
static void write_file(lwns_controller_ptr ptr, const lwns_addr_t *sender,
        int offset, int flag, char *data, int datalen) {
    //senderΪ���ͷ��ĵ�ַ
    //�����Ҫ���ղ�ͬ���ļ�����Ҫ�ڴ˺��������ýӿ�
    if (datalen > 0) {     //�����������data��ȡ���ݴ�ӡ
        PRINTF("r:%c\n", *data);
    }
    if (flag == LWNS_RUCFT_FLAG_END) {
        PRINTF("re\n");
        //�����ļ���������һ����
    } else if (flag == LWNS_RUCFT_FLAG_NONE) {
        PRINTF("ru\n");
        //�����ļ����������İ�
    } else if (flag == LWNS_RUCFT_FLAG_NEWFILE) {
        PRINTF("rn\n");
        //�����ļ�����ĵ�һ����
    }
}

static int read_file(lwns_controller_ptr ptr, int offset, char *to,
        int maxsize) {
    //toΪ��Ҫ�������ݹ�ȥ��ָ��
    //�����Ҫ���Ͳ�ͬ���ļ�����Ҫ�ڴ˺��������ýӿ�
    int size = maxsize;
    if (offset >= FILESIZE) {
        //�ϴ��Ѿ�����,���������ȷ��
        PRINTF("Send done\n");
        tmos_start_task(rucft_taskID, RUCFT_EXAMPLE_TX_PERIOD_EVT,
                MS1_TO_SYSTEM_TIME(5000)); //5���Ӻ�������Ͳ���
        return 0;
    } else if (offset + maxsize >= FILESIZE) {
        size = FILESIZE - offset;

    }
    //�ѱ�����Ҫ���͵�����ѹ��������
    tmos_memcpy(to, strp + offset, size);
    return size;
}
static void timedout_rucft(lwns_controller_ptr ptr) {
    //rucft�У����ͷ����ط�������������ط������󣬻���øûص���
    //���շ���ʱû���յ���һ����Ҳ�����
    PRINTF("rucft %d timedout\r\n", get_lwns_object_port(ptr));
}
void lwns_rucft_process_init(void) {
    lwns_addr_t MacAddr;
    rucft_taskID = TMOS_ProcessEventRegister(lwns_rucft_ProcessEvent);
    lwns_rucft_init(&rucft, 137, //�˿ں�
            HTIMER_SECOND_NUM / 10, //�ȴ�Ŀ��ڵ�ackʱ��
            5, //����ط���������ruc�е�ruc_send���ط���������һ��
            &rucft_callbacks//�ص�����
            ); //����0�����ʧ�ܡ�����1�򿪳ɹ���
    int i;
    for (i = 0; i < FILESIZE; i++) {    //LWNS_RUCFT_DATASIZE��LWNSNK_RUCFT_DATASIZE��b���ȵȣ���ʼ����Ҫ���͵�����
        strsend[i] = 'a' + i / LWNS_RUCFT_DATASIZE;
    }
    strp = strsend;
    GetMACAddress(MacAddr.u8);
    if (lwns_addr_cmp(&MacAddr, &dst_addr)) {

    } else {
        tmos_start_task(rucft_taskID, RUCFT_EXAMPLE_TX_PERIOD_EVT,
                MS1_TO_SYSTEM_TIME(1000));
    }
}
uint16 lwns_rucft_ProcessEvent(uint8 task_id, uint16 events) {
    if (events & RUCFT_EXAMPLE_TX_PERIOD_EVT) {
        PRINTF("send\n");
        lwns_rucft_send(&rucft, &dst_addr); //��ʼ������Ŀ��ڵ㣬�û����÷���ʱҪ���úûص������е����ݰ���ȡ
        return events ^ RUCFT_EXAMPLE_TX_PERIOD_EVT;
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
