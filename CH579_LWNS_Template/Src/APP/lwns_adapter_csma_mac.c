/*
 *  lwns_adapter_csma_mac.c
 *  lwns�����������䲻ͬ��оƬ�Ϳ⣺
 *  ���ļ��ṩ��һ��ģ��csma�ļ򵥵ķ���ײmac��Э��
 *  Created on: Jul 14, 2021
 *      Author: WCH
 */
#include "config.h"
#include "CH57x_common.h"
#include "lwns_adapter_csma_mac.h"
#include "lwns_sec.h"

#define DEBUG_PRINT_IN_THIS_FILE 1
#if DEBUG_PRINT_IN_THIS_FILE
#define PRINTF(...) PRINT(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif

#if LWNS_USE_CSMA_MAC//�Ƿ�ʹ��ģ��csma��macЭ�飬ע��ֻ��ʹ��һ��mac��Э�顣

//for lwns_packet_buffer save
#define QBUF_MANUAL_NUM   4
__attribute__((aligned(4)))  static lwns_qbuf_list_t qbuf_memp[QBUF_MANUAL_NUM];

//for lwns_route_entry manage
#define ROUTE_ENTRY_MANUAL_NUM   32
#if ROUTE_ENTRY_MANUAL_NUM
__attribute__((aligned(4)))  static lwns_route_entry_data_t route_entry_memp[ROUTE_ENTRY_MANUAL_NUM];
#endif

//for neighbor manage
#define NEIGHBOR_MANUAL_NUM   LWNS_NEIGHBOR_MAX_NUM
__attribute__((aligned(4)))  static lwns_neighbor_list_t neighbor_memp[NEIGHBOR_MANUAL_NUM];


static void ble_new_neighbor_callback(lwns_addr_t *n); //�������ھӻص�����
static BOOL ble_phy_output(u8 * dataptr, uint8_t len); //���ͽӿں���
static void RF_2G4StatusCallBack(uint8 sta, uint8 crc, uint8 *rxBuf);

static uint8 lwns_adapter_taskid;
static uint16 lwns_adapter_ProcessEvent(uint8 task_id, uint16 events);
static uint8 lwns_phyoutput_taskid;
static uint16 lwns_phyoutput_ProcessEvent(uint8 task_id, uint16 events);

//lwns���õĺ����ӿڣ���ָ�봫�ݸ�lwns���ڲ�ʹ��
static lwns_fuc_interface_t ble_lwns_fuc_interface = {
        .lwns_phy_output  = ble_phy_output,
        .lwns_rand        = tmos_rand,
        .lwns_memcpy      = tmos_memcpy,
        .lwns_memcmp      = tmos_memcmp,
        .lwns_memset      = tmos_memset,
        .new_neighbor_callback  = ble_new_neighbor_callback,
};

static u8 ble_phy_manage_state, ble_phy_send_cnt = 0, ble_phy_wait_cnt = 0; //ble phy״̬�������ʹ����������ȴ���������
static struct csma_mac_phy_manage_struct* csma_phy_manage_list_head = NULL; //mac�������б�ָ��
static struct csma_mac_phy_manage_struct csma_phy_manage_list[LWNS_MAC_SEND_PACKET_MAX_NUM]; //mac�������б��������


static void RF_2G4StatusCallBack(uint8 sta, uint8 crc, uint8 *rxBuf) {
    switch (sta) {
    case RX_MODE_RX_DATA: {
        if (crc == 1) {
            PRINTF("crc error\n");
        } else if (crc == 2) {
            PRINTF("match type error\n");
        } else {
            u8 *pMsg;
#if LWNS_ENCRYPT_ENABLE//�Ƿ�������Ϣ���ܣ�����aes128��ΪӲ��ʵ��
            if (((rxBuf[1] % 16) == 1) && (rxBuf[1] >= 17) && (rxBuf[1] > rxBuf[2])) {//���������������16���ֽڣ�������ʵ���ݳ���һ�ֽ�
                //����У��ͨ��������rxBuf[1] - 1��Ϊ16�ı���
                pMsg = tmos_msg_allocate(rxBuf[1]);//�����ڴ�ռ䣬��ʵ���ݳ��Ȳ���Ҫ����
                if (pMsg != NULL) {
                    lwns_msg_decrypt(rxBuf + 3, pMsg + 1, rxBuf[1] - 1);//��������
                    if((rxBuf[2] ^ pMsg[rxBuf[2]]) == pMsg[rxBuf[2]+1]){
                        pMsg[0] = rxBuf[2];//У��ͨ�����洢��ʵ���ݳ���
                        PRINTF("send rx msg\n");//���ͽ��յ������ݵ����ս�����
                        tmos_msg_send(lwns_adapter_taskid, pMsg);
                    } else {
                        PRINTF("verify rx msg err\n");//У��ʧ��
                        tmos_msg_deallocate(pMsg);
                    }
                } else {
                    PRINTF("send rx msg failed\n");//�����ڴ�ʧ�ܣ��޷����ͽ��յ�������
                }
            } else {
                PRINTF("bad len\n");//�����Ȳ���
            }
#else
            if (rxBuf[1] >= LWNS_PHY_OUTPUT_MIN_SIZE) { //���ݳ��ȷ��ϣ��Żᷢ����Э��ջ�ڲ�����
                pMsg = tmos_msg_allocate(rxBuf[1] + 1);
                if (pMsg != NULL) {
                    PRINTF("send rx msg\n");//���ͽ��յ������ݵ�����������
                    tmos_memcpy(pMsg, rxBuf + 1, rxBuf[1] + 1);
                    tmos_msg_send(lwns_adapter_taskid, pMsg);
                } else {
                    PRINTF("rx msg failed\n");//�����ڴ�ʧ�ܣ��޷����ͽ��յ�������
                }
            } else {
                PRINTF("bad len\n");//�����Ȳ���
            }
#endif
            //�����յ�һ�����ݰ�ʱ������ʱ����ڵĻ�δ��ʼ�ķ�������ֹͣ���ȴ���һʱ��η��ͣ��Ѿ���ʼ�ķ���������ͣ��ģ��csma/ca�����з���ײ��ؼ�⡣
            if (ble_phy_manage_state == BLE_PHY_MANAGE_STATE_WAIT_SEND) { //�ȴ�����״̬���յ����ݰ�
                PRINTF("send delay\n");
                ble_phy_manage_state = BLE_PHY_MANAGE_STATE_RECEIVED; //�ȴ������������յ������ݰ�״̬
                tmos_stop_task(lwns_phyoutput_taskid, LWNS_PHY_OUTPUT_EVT); //��������Ȩ��ʧ�ܣ������Լ��ķ�������
                tmos_clear_event(lwns_phyoutput_taskid, LWNS_PHY_OUTPUT_EVT);//�����Լ��ķ�������
            }
        }
        tmos_set_event(lwns_adapter_taskid, LWNS_PHY_RX_OPEN_EVT);//���´򿪽���
        break;
    }
    case TX_MODE_TX_FINISH:
    case TX_MODE_TX_FAIL:
        tmos_stop_task(lwns_phyoutput_taskid, LWNS_PHY_OUTPUT_FINISH_EVT);//ֹͣ��ʱ����
        tmos_set_event(lwns_phyoutput_taskid, LWNS_PHY_OUTPUT_FINISH_EVT);//���뷢����ɴ���
        break;
    default:
        break;
    }
}

void RF_Init(void) {
    uint8 state;
    rfConfig_t rfConfig;
    tmos_memset( &rfConfig, 0, sizeof(rfConfig_t) );
    rfConfig.TxAccessAddress = 0x17267162; // ��ֹʹ��0x55555555�Լ�0xAAAAAAAA ( ���鲻����24��λ��ת���Ҳ�����������6��0��1 )����ȷ������Ӧ�����accessaddress�����ַԼ��23�ڸ�
		rfConfig.RxAccessAddress = 0x17267162;
    rfConfig.TxCRCInit = 0x555555;
		rfConfig.RxCRCInit = 0x555555;
    rfConfig.Channel = 8;
    rfConfig.LLEMode = LLE_MODE_BASIC; //|LLE_MODE_EX_CHANNEL; // ʹ�� LLE_MODE_EX_CHANNEL ��ʾ ѡ�� rfConfig.Frequency ��Ϊͨ��Ƶ��
    rfConfig.rfStatusCB = RF_2G4StatusCallBack;
    state = RF_Config(&rfConfig);
    PRINTF("rf 2.4g init: %x\n", state);
}

void lwns_init(void) {
    uint8_t s;
    lwns_config_t cfg;
    tmos_memset( &cfg, 0, sizeof(lwns_config_t) );
    cfg.lwns_lib_name = (u8*) VER_LWNS_FILE; //��֤���������ƣ���ֹ�汾����
    cfg.qbuf_num = QBUF_MANUAL_NUM; //������䣬����1���ڴ浥λ�������������ʹ�õĶ˿�����Ӧģ��ʹ�õ�qbuf��λ�����塣
    cfg.qbuf_ptr = qbuf_memp; //mesh���ʹ��3��qbuf��λ��(uni/multi)netflood���ʹ��2��������ģ�鶼ʹ��1����
    cfg.routetable_num = ROUTE_ENTRY_MANUAL_NUM; //�����Ҫʹ��mesh���������·�ɱ��ڴ�ռ䡣��Ȼmesh��ʼ������ɹ���
#if ROUTE_ENTRY_MANUAL_NUM
    cfg.routetable_ptr = route_entry_memp;
#else
    cfg.routetable_ptr = NULL;
#endif
    cfg.neighbor_num = NEIGHBOR_MANUAL_NUM; //�ھӱ��������������
    cfg.neighbor_list_ptr = neighbor_memp; //�ھӱ��ڴ�ռ�
    cfg.neighbor_mod = LWNS_NEIGHBOR_AUTO_ADD_STATE_RECALL_ADDALL; //�ھӱ��ʼ��Ĭ�Ϲ���ģʽΪ�������а�����������ھӲ��ҹ����ظ�����ģʽ
#if LWNS_ADDR_USE_BLE_MAC
    GetMACAddress(cfg.addr.u8); //����Ӳ����mac��ַ
#else
//���ж���ĵ�ַ
            uint8 MacAddr[6] = {0,0,0,0,0,1};
            tmos_memcpy(cfg.addr.u8, MacAddr, LWNS_ADDR_SIZE);
#endif
    s = lwns_lib_init(&ble_lwns_fuc_interface, &cfg); //lwns��ײ��ʼ��
    if (s) {
        PRINTF("%s init err:%d\n", VER_LWNS_FILE, s);
    } else {
        PRINTF("%s init ok\n", VER_LWNS_FILE);
    }
    lwns_adapter_taskid = TMOS_ProcessEventRegister(lwns_adapter_ProcessEvent);
    lwns_phyoutput_taskid = TMOS_ProcessEventRegister(lwns_phyoutput_ProcessEvent);
    tmos_start_task(lwns_phyoutput_taskid, LWNS_PHY_PERIOD_EVT, MS1_TO_SYSTEM_TIME(LWNS_MAC_PERIOD_MS));
    tmos_memset(csma_phy_manage_list, 0, sizeof(csma_phy_manage_list));//������͹���ṹ��
    ble_phy_manage_state = BLE_PHY_MANAGE_STATE_FREE; //���phy״̬
    RF_Shut();
    RF_Rx(NULL, 0, USER_RF_RX_TX_TYPE, USER_RF_RX_TX_TYPE); //��RF���գ������Ҫ�͹��Ĺ����������ط��򿪡�
}
static void ble_new_neighbor_callback(lwns_addr_t *n) {
    PRINTF("new neighbor: %02x %02x %02x %02x %02x %02x\n", n->u8[0], n->u8[1],
            n->u8[2], n->u8[3], n->u8[4], n->u8[5]);
}
/*---------------------------------------------------------------------------*/
static BOOL ble_phy_output(u8 * dataptr, uint8_t len) {
    u8 *pMsg, i;
    struct csma_mac_phy_manage_struct *p;
    for (i = 0; i < LWNS_MAC_SEND_PACKET_MAX_NUM; i++) {
        if (csma_phy_manage_list[i].data == NULL) {
            break; //Ѱ�ҵ���һ���յĽṹ�����ʹ�á�
        } else {
            if (i == (LWNS_MAC_SEND_PACKET_MAX_NUM - 1)) {
                PRINTF("send failed!\n"); //�б����ˣ�����ʧ�ܣ�ֱ�ӷ��ء�
                return FALSE;
            }
        }
    }
#if LWNS_ENCRYPT_ENABLE
    pMsg = tmos_msg_allocate((((len + 1 + 15) & 0xf0) + 1 + 1)); //У��λ1λ���Ϻ��ٽ���16�ֽڶ��룬�洢���ͳ���+1����ʵ���ݳ���+1
#else
    pMsg = tmos_msg_allocate(len + 1); //�����ڴ�ռ�洢��Ϣ���洢���ͳ���+1
#endif
    if (pMsg != NULL) {//�ɹ�����
        p = csma_phy_manage_list_head;
        if (p != NULL) {
            while (p->next != NULL) {//Ѱ�ҷ���������յ�
                p = p->next;
            }
        }
#if LWNS_ENCRYPT_ENABLE
        //lwns buffer�ڲ�Ԥ�������ֽڣ��û���ֱ��ʹ��dataptr[len]���и�ֵ���ֽ�����
        dataptr[len] = dataptr[len - 1] ^ len;//У���ֽڽ�ȡ���һ���ֽںͳ��Ƚ���������㣬���ֽ���ͬport��һ���ģ�������Ӱ�졣��У��Ƚ��˷�ʱ�䣬���Բ�����
        pMsg[1] = len;//��ʵ���ݳ���ռһ�ֽڣ������ܣ�������������һ��У��
        pMsg[0] = lwns_msg_encrypt(dataptr, pMsg + 2, len + 1) + 1;//��ȡ���ݼ��ܺ�ĳ��ȣ�Ҳ������Ҫ���ͳ�ȥ���ֽ�������ʵ���ݳ��Ȳ�����
#else
        pMsg[0] = len;
        tmos_memcpy(pMsg + 1, dataptr, len);
#endif
        if (csma_phy_manage_list_head != NULL) {
            p->next = &csma_phy_manage_list[i];        //�������β���
        } else {
            csma_phy_manage_list_head = &csma_phy_manage_list[i];        //����Ϊ�գ���ڵ���Ϊͷ���
        }
        csma_phy_manage_list[i].data = pMsg;        //����Ϣ
        csma_phy_manage_list[i].next = NULL;
        return TRUE;
    } else {
        PRINTF("send failed!\n");//�޷����뵽�ڴ棬���޷�����
    }
    return FALSE;
}

static uint16 lwns_adapter_ProcessEvent(uint8 task_id, uint16 events) {
    if (events & LWNS_PHY_RX_OPEN_EVT) {
        RF_Shut();
        RF_Rx(NULL, 0, USER_RF_RX_TX_TYPE, USER_RF_RX_TX_TYPE); //���´򿪽���
        return (events ^ LWNS_PHY_RX_OPEN_EVT);
    }
    if (events & SYS_EVENT_MSG) {
        uint8 *pMsg;
        if ((pMsg = tmos_msg_receive(lwns_adapter_taskid)) != NULL) {
            // Release the TMOS message,tmos_msg_allocate
            lwns_input(pMsg + 1, pMsg[0]); //�����ݴ���Э��ջ������
            tmos_msg_deallocate(pMsg); //���ͷ��ڴ棬�����ݴ���ǰ�ͷţ���ֹ���ݴ�������Ҫ�������ݣ����ڴ治����
            lwns_dataHandler(); //����Э��ջ�������ݺ���
        }
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }
    // Discard unknown events
    return 0;
}

static uint16 lwns_phyoutput_ProcessEvent(uint8 task_id, uint16 events) {
    if (events & LWNS_PHY_PERIOD_EVT) {
        lwns_htimer_update(); //htimer�ĸ�����Ҫ��mac��phy�����һ�𣬱���һ�¡�
        if ((csma_phy_manage_list_head != NULL)) { //����Ҫ���͵İ�
            if((ble_phy_manage_state == BLE_PHY_MANAGE_STATE_FREE) || (ble_phy_manage_state == BLE_PHY_MANAGE_STATE_RECEIVED)){//��ǰ���ڷ��͹�����
                if (ble_phy_manage_state == BLE_PHY_MANAGE_STATE_RECEIVED) { //��ǰ���ڷ�����ײ���ӳٷ���
                    ble_phy_wait_cnt++; //��¼�����ӳٴ���
                } else {//BLE_PHY_MANAGE_STATE_FREE
                    ble_phy_send_cnt = 0;//�������
                    ble_phy_wait_cnt = 0;//�������
                }
                ble_phy_manage_state = BLE_PHY_MANAGE_STATE_WAIT_SEND; //����Ϊ�ȴ�����״̬
                if (ble_phy_wait_cnt >= LWNS_MAC_SEND_DELAY_MAX_TIMES) { //���ͱ�ȡ���������ӳٹ��࣬��������ȴ������̿�ʼ����
                    tmos_set_event(lwns_phyoutput_taskid, LWNS_PHY_OUTPUT_EVT);
                    PRINTF("too many delay\n");
                } else {
                    u8 rand_delay;
                    rand_delay = tmos_rand() % LWNS_MAC_SEND_DELAY_MAX_625US + BLE_PHY_ONE_PACKET_MAX_625US; //����ӳ٣���ֹ��ͻ������ӳٵȴ��������յ������ݰ����´��ٷ���
                    tmos_start_task(lwns_phyoutput_taskid, LWNS_PHY_OUTPUT_EVT, rand_delay);
                    PRINTF("rand send:%d\n", rand_delay);
                }
            }
        }
        tmos_start_task(lwns_phyoutput_taskid, LWNS_PHY_PERIOD_EVT, MS1_TO_SYSTEM_TIME(LWNS_MAC_PERIOD_MS)); //ÿ�����ں�����һ����������Ҫ���ͣ����Ҹ���htimer��
        return (events ^ LWNS_PHY_PERIOD_EVT);
    }
    if (events & LWNS_PHY_OUTPUT_EVT) {//�������񣬾������ͳɹ�
        if (ble_phy_manage_state == BLE_PHY_MANAGE_STATE_WAIT_SEND){
            ble_phy_manage_state = BLE_PHY_MANAGE_STATE_SENDING; //��Ϊ������״̬��������������ϣ���Ҫ�ȴ�һ�½��շ�����׼������
            tmos_clear_event(lwns_adapter_taskid, LWNS_PHY_RX_OPEN_EVT);//ֹͣ�����Ѿ���λ�ġ����ܻ�򿪽��յ�����
        }
        RF_Shut();
        RF_Tx((u8 *) (csma_phy_manage_list_head->data + 1),
                                csma_phy_manage_list_head->data[0], USER_RF_RX_TX_TYPE,
                                USER_RF_RX_TX_TYPE);
        tmos_start_task(lwns_phyoutput_taskid, LWNS_PHY_OUTPUT_FINISH_EVT, MS1_TO_SYSTEM_TIME(LWNS_PHY_OUTPUT_TIMEOUT_MS));//��ʼ���ͳ�ʱ����
        return (events ^ LWNS_PHY_OUTPUT_EVT);
    }
    if (events & LWNS_PHY_OUTPUT_FINISH_EVT) {//�����������
        ble_phy_send_cnt++;//���ͼ���
        if (ble_phy_send_cnt < LWNS_MAC_TRANSMIT_TIMES) {//����û����
            tmos_set_event(lwns_phyoutput_taskid, LWNS_PHY_OUTPUT_EVT);//���ʹ���û��������������
        } else {//�������̽���
            ble_phy_manage_state = BLE_PHY_MANAGE_STATE_FREE;     //���״̬
            RF_Shut();
            RF_Rx(NULL, 0, USER_RF_RX_TX_TYPE, USER_RF_RX_TX_TYPE); //���´򿪽���
            tmos_msg_deallocate(csma_phy_manage_list_head->data); //�ͷ��ڴ�
            csma_phy_manage_list_head->data = NULL; //�ָ�Ĭ�ϲ���
            csma_phy_manage_list_head = csma_phy_manage_list_head->next; //����pop��ȥ������Ԫ��
        }
        return (events ^ LWNS_PHY_OUTPUT_FINISH_EVT);
    }
    if (events & SYS_EVENT_MSG) {
        uint8 *pMsg;
        if ((pMsg = tmos_msg_receive(lwns_phyoutput_taskid)) != NULL) {
            // Release the TMOS message,tmos_msg_allocate
            tmos_msg_deallocate(pMsg); //�ͷ��ڴ�
        }
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }
    return 0;
}

#endif  /* LWNS_USE_CSMA_MAC */

