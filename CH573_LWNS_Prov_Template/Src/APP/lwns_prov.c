/*
 * lwns_prov.c
 *  lwnsͨ��2.4g�������̣�Ҳ����ʹ��������usb�����ڵ�������������
   *  ��������Ҫһ���豸�䵱�����豸���������豸��Ҫ�źŴ�����������ģʽ������֮ǰ��Ҫ��mac�㷢���б�ȫ����գ����������豸ֻ������������
   * ���豸���һ���㲥ͨ�����ָ�����δ����ʱ����Կ��rf���������й㲥���ݵ��շ���
   * ���豸���յ�δ�����豸�Ĺ㲥���ݺ󣬽��µ���Կ��rf�����㲥���ͻ�ȥ��
   * ������������̡�
 *  Created on: Sep 28, 2021
 *      Author: WCH
 */
#include "lwns_prov.h"
#include "lwns_adapter_csma_mac.h"
#include "lwns_adapter_blemesh_mac.h"
#include "lwns_adapter_no_mac.h"
#include "lwns_broadcast_example.h"

#define DEBUG_PRINT_IN_THIS_FILE 1
#if DEBUG_PRINT_IN_THIS_FILE
#define PRINTF(...) PRINT(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif

uint8_t prov_task_id;
uint16_t lwns_prov_ProcessEvent(uint8_t task_id, uint16_t events);

__attribute__((aligned(4)))  const char lwns_prov_index[]={"prov"};

/**
 * lwnsĬ�ϵ���������
 */
static const rf_config_params_t lwns_rf_params_default = {
        .Channel[0] = 8,
        .Channel[1] = 18,
        .Channel[2] = 28,
        .channelNum = 3,
        .CRCInit = 0x555555,
        .accessAddress = 0x17267162,
};

//Ĭ�ϵ�������Կ
static const uint8_t lwns_sec_key_default[16]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};

#if PROV_MASTER

/**
 * lwns���������������
 */
static const rf_config_params_t lwns_rf_params_prov = {
        .Channel[0] = 9,
        .Channel[1] = 19,
        .Channel[2] = 29,
        .channelNum = 3,
        .CRCInit = 0x555555,
        .accessAddress = 0x71621726,
};

//�������������Կ
static const unsigned char lwns_sec_key_prov[16]={10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160};

#endif

/*********************************************************************
 * @fn      prov_bc_recv
 *
 * @brief   �����㲥���ջص�����
 *
 * @param   ptr     -   ���ν��յ������������Ĺ㲥���ƽṹ��ָ��.
 * @param   sender  -   ���ν��յ������ݵķ����ߵ�ַָ��.
 *
 * @return  None.
 */
static void prov_bc_recv(lwns_controller_ptr ptr,
        const lwns_addr_t* sender) {
#if PROV_MASTER
		void *prov_uc;
    //�û�������Ҫ������ѡ��sender��������
    uint8_t prov_buffer[sizeof(rf_config_params_t) + sizeof(lwns_sec_key_prov)];//���͵��������û��������������Ҫ��У�������
    PRINTF("rec prov request from %02x,%02x,%02x,%02x,%02x,%02x\n",sender->v8[0],sender->v8[1],sender->v8[2],sender->v8[3],sender->v8[4],sender->v8[5]);
    prov_uc = lwns_controller_lookup(PROV_UC_PORT);//���ҿ��ƽṹ��ָ��
    if(prov_uc != NULL) {
        //�û���������������ò�������У��
        tmos_memcpy(prov_buffer, &lwns_rf_params_prov, sizeof(rf_config_params_t));
        tmos_memcpy(prov_buffer + sizeof(rf_config_params_t), lwns_sec_key_prov, sizeof(lwns_sec_key_prov));
        lwns_buffer_load_data(prov_buffer, sizeof(rf_config_params_t) + sizeof(lwns_sec_key_prov));
        lwns_unicast_send(prov_uc, sender);
        PRINTF("send prov params\n");
    }
#else
    PRINTF("I am slave,ignore packet\n");
#endif
}

/**
 * lwns�����㲥�ص������ṹ�壬ע��ص�����
 */
static const struct lwns_broadcast_callbacks prov_bc_callbacks = {
        prov_bc_recv, NULL };//�����㲥�ص������ṹ�壬ע��ص�����

/*********************************************************************
 * @fn      prov_uc_recv
 *
 * @brief   �����������ջص�����
 *
 * @param   ptr     -   ���ν��յ������������ĵ������ƽṹ��ָ��.
 * @param   sender  -   ���ν��յ������ݵķ����ߵ�ַָ��.
 *
 * @return  None.
 */
static void prov_uc_recv(lwns_controller_ptr ptr, const lwns_addr_t *from){
#if PROV_MASTER
    PRINTF("I am master,ignore packet\n");
#else
    uint8_t *data,len;
    len = lwns_buffer_datalen(); //��ȡ��ǰ���������յ������ݳ���
    if (len == sizeof(rf_config_params_t) + sizeof(lwns_sec_key_default)) {
        PRINTF("rec prov params from %02x,%02x,%02x,%02x,%02x,%02x\n",from->v8[0],from->v8[1],from->v8[2],from->v8[3],from->v8[4],from->v8[5]);
        data = lwns_buffer_dataptr();
        tmos_memcpy(&lwns_rf_params,data,sizeof(rf_config_params_t));
        rf_config_params_save_to_flash();//����rf����
        RF_Init();//���³�ʼ��rf����
        lwns_temp_set_key(data + sizeof(rf_config_params_t));
        lwns_save_key_to_flash();//����key
        lwns_set_provisioned();//����������
        tmos_stop_task(prov_task_id, PROV_BROADCAST_TX_PERIOD_EVT);
        tmos_clear_event(prov_task_id, PROV_BROADCAST_TX_PERIOD_EVT);
        prov_process_deinit();
        PRINTF("Start lwns user process\n");
        lwns_broadcast_process_init();

    } else {
        PRINTF("data len err\n");
    }
#endif
}

/**
 * lwns���������ص������ṹ�壬ע��ص�����
 */
static const struct lwns_unicast_callbacks prov_uc_callbacks = {
        prov_uc_recv,NULL
};

/*********************************************************************
 * @fn      prov_process_init
 *
 * @brief   lwns����״̬��ʼ������flash�ж�ȡ����Կ����Ϣ
 *
 * @param   None.
 *
 * @return  None.
 */
void prov_process_init(void) {
    void *prov_bc,*prov_uc;

    PRINTF("prov init\n");
#if PROV_MASTER
    prov_bc = tmos_msg_allocate(sizeof(lwns_broadcast_controller));//�����ڴ棬���ܲ�����һֱ�����������Ծ�����ռ��ȫ�ֱ���
    if(prov_bc == NULL){
        PRINTF("failed\n");//��ʼ���������ƽṹ��ʧ��
        return;
    }
    prov_uc = tmos_msg_allocate(sizeof(lwns_unicast_controller));//�����ڴ�
    if(prov_uc == NULL){
        tmos_msg_deallocate(prov_bc);//�ͷ�ͨ�����ƽṹ��ռ���ڴ�
        PRINTF("failed\n");//��ʼ���������ƽṹ��ʧ��
        return;
    }
    lwns_broadcast_init(prov_bc, PROV_BC_PORT, &prov_bc_callbacks); //��ͨ����Ϊ1�Ĺ㲥ͨ���������������ݵ��շ�
    lwns_unicast_init(prov_uc, PROV_UC_PORT, &prov_uc_callbacks); //��ͨ����Ϊ2�ĵ���ͨ���������������ݵ��շ�
    PRINTF("I am master,don't need to send prov request\n");
#else
    if(lwns_provisioned() == 0){
        prov_bc = tmos_msg_allocate(sizeof(lwns_broadcast_controller));//�����ڴ棬���ܲ�����һֱ�����������Ծ�����ռ��ȫ�ֱ���
        if(prov_bc == NULL){
            PRINTF("failed\n");//��ʼ���������ƽṹ��ʧ��
            return;
        }
        prov_uc = tmos_msg_allocate(sizeof(lwns_unicast_controller));//�����ڴ�
        if(prov_uc == NULL){
            tmos_msg_deallocate(prov_bc);//�ͷ�ͨ�����ƽṹ��ռ���ڴ�
            PRINTF("failed\n");//��ʼ���������ƽṹ��ʧ��
            return;
        }
        lwns_broadcast_init(prov_bc, PROV_BC_PORT, &prov_bc_callbacks); //��ͨ����Ϊ1�Ĺ㲥ͨ���������������ݵ��շ�
        lwns_unicast_init(prov_uc, PROV_UC_PORT, &prov_uc_callbacks); //��ͨ����Ϊ2�ĵ���ͨ���������������ݵ��շ�
        prov_task_id = TMOS_ProcessEventRegister(lwns_prov_ProcessEvent);//�ӻ�ע�����ڹ㲥����
        PRINTF("prov taskid:%d\n",prov_task_id);
        tmos_start_task(prov_task_id, PROV_BROADCAST_TX_PERIOD_EVT,
                MS1_TO_SYSTEM_TIME(1000));//��ʼ�����Թ㲥��Ѱ������
    } else {
        PRINTF("I am provisioned,don't need to send prov request\n");
        PRINTF("Start lwns user process\n");
        lwns_broadcast_process_init();
    }
#endif
}


/*********************************************************************
 * @fn      prov_process_deinit
 *
 * @brief   ����lwns��������
 *
 * @param   None.
 *
 * @return  None.
 */
void prov_process_deinit(void){
    void *prov_bc,*prov_uc;
    prov_bc = lwns_controller_lookup(PROV_BC_PORT);//����ͨ�����ƽṹ��ָ��
    if(prov_bc!=NULL) {
        lwns_broadcast_close(prov_bc);//�رչ㲥ͨ��
        tmos_msg_deallocate(prov_bc);//�ͷ�ͨ�����ƽṹ��ռ���ڴ�
    } else {
        PRINTF("find bc failed\n");
    }
    prov_uc = lwns_controller_lookup(PROV_UC_PORT);
    if(prov_uc != NULL) {
        lwns_unicast_close(prov_uc);//�رյ���ͨ��
        tmos_msg_deallocate(prov_uc);//�ͷ�ͨ�����ƽṹ��ռ���ڴ�
    } else {
        PRINTF("find uc failed\n");
    }
}

/*********************************************************************
 * @fn      lwns_prov_ProcessEvent
 *
 * @brief   lwns provison Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed.
 */
uint16_t lwns_prov_ProcessEvent(uint8_t task_id, uint16_t events) {
    if (events & PROV_BROADCAST_TX_PERIOD_EVT) {
        uint8_t *prov_bc;
        lwns_buffer_load_data(NULL, 0);//������Ҫ���͵����ݵ�������
        prov_bc = lwns_controller_lookup(PROV_BC_PORT);
        if(prov_bc != NULL) {
            PRINTF("send prov request\n");
            lwns_broadcast_send(prov_bc);//�㲥��������
            tmos_start_task(task_id, PROV_BROADCAST_TX_PERIOD_EVT,
                    MS1_TO_SYSTEM_TIME(1000));//�����Է���
        }
        return events ^ PROV_BROADCAST_TX_PERIOD_EVT;
    }
    if (events & SYS_EVENT_MSG) {
        uint8_t *pMsg;

        if ((pMsg = tmos_msg_receive(task_id)) != NULL) {
            // Release the TMOS message
            tmos_msg_deallocate(pMsg);
        }
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }
    return 0;
}

/*********************************************************************
 * @fn      lwns_provisioned
 *
 * @brief   lwns��ȡ�豸����״̬
 *
 * @param   None.
 *
 * @return  lwns�豸����״̬.
 */
uint8_t lwns_provisioned(void)
{
    size_t  len;
    uint8_t value;
    ef_get_env_blob(lwns_prov_index, NULL, 0, &len);
    if(len == 1){
        ef_get_env_blob(lwns_prov_index, &value, 1, NULL);
        return value;
    }
    return 0;
}

/*********************************************************************
 * @fn      lwns_set_provisioned
 *
 * @brief   lwns����״̬������
 *
 * @param   None.
 *
 * @return  defined in EfErrCode.
 */
EfErrCode lwns_set_provisioned(void)
{
    EfErrCode err;
    uint8_t state = 1;
    err = ef_set_env_blob(lwns_prov_index, &state, 1);
    if(err != EF_NO_ERR){
        PRINTF("set prov err\n");
    } else {
        PRINTF("set prov ok\n");
    }
    return err;
}

/*********************************************************************
 * @fn      lwns_provisioned_reset
 *
 * @brief   lwns����״̬δ����
 *
 * @param   None.
 *
 * @return  defined in EfErrCode.
 */
EfErrCode lwns_provisioned_reset(void)
{
    EfErrCode err;
    tmos_memcpy(&lwns_rf_params,&lwns_rf_params_default,sizeof(rf_config_params_t));//�ָ�Ĭ�ϵ�rf����
    rf_config_params_save_to_flash();//����rf����
    lwns_temp_set_key((unsigned char *)lwns_sec_key_default);//�ָ�Ĭ�ϵ���Կ����
    lwns_save_key_to_flash();//����key
    err = ef_del_env(lwns_prov_index);
    PRINTF("prov reset:%d\n",err);
    return err;
}

/*
 * lwnsͨ���ֻ�������������
   *  ��������Ҫһ���豸�����������ܣ��ֻ�ͨ��rwprofile���豸������������
   * ������������̡�
 */

#if BLE_PROV

/*********************************************************************
 * @fn      Rec_BLE_PROV_DataDeal
 *
 * @brief   lwns��������ͨ�����պ���
 *
 * @param   p_data      -   ���ν��յ�������ָ��.
 * @param   w_len       -   ���ν��յ������ݳ���.
 * @param   r_data      -   ���λظ�������ָ��.
 * @param   w_len       -   ���λظ������ݳ���ָ��.
 *
 * @return  None.
 */
void Rec_BLE_PROV_DataDeal(unsigned char *p_data, unsigned char w_len ,unsigned char *r_data, unsigned char* r_len)
{
    r_data[0] = p_data[0];
    switch(p_data[0]){
    case LWNS_PROV_CMD_TEST:{
        PRINTF("test ok\n");
        tmos_memcpy(r_data, p_data, w_len);
        *r_len = w_len;
        break;
    }
#if PROV_MASTER
    //����������
#else
    case LWNS_PROV_CMD_GET_PROV_STATE:{
        r_data[1] = lwns_provisioned();
        *r_len = 2;
        break;
    }
    case LWNS_PROV_CMD_GET_RF_CONFIG_PARAMS:{
        tmos_memcpy(r_data + 1, &lwns_rf_params, sizeof(lwns_rf_params));
        *r_len = 1 + sizeof(lwns_rf_params);
        break;
    }
    case LWNS_PROV_CMD_GET_RF_KEY:{
        lwns_get_key(r_data + 1);
        *r_len = 17;
        break;
    }
    case LWNS_PROV_CMD_SET_RF_CONFIG_PARAMS:{
        tmos_memcpy(&lwns_rf_params, p_data + 1, sizeof(lwns_rf_params));
        r_data[1] = 0;
        *r_len = 2;
        break;
    }
    case LWNS_PROV_CMD_SET_RF_KEY:{
        lwns_temp_set_key(p_data + 1);
        r_data[1] = 0;
        *r_len = 2;
        break;
    }
    case LWNS_PROV_CMD_PROV_ENABLE:{
        EfErrCode err;
        *r_len = 2;
        err = rf_config_params_save_to_flash();//����rf����
        if(err){
            r_data[1] = err;
            break;
        }
        RF_Init();//���³�ʼ��rf����
        err = lwns_save_key_to_flash();//����key
        if(err){
            r_data[1] = err;
            break;
        }
        err = lwns_set_provisioned();//����������
        if(err){
            r_data[1] = err;
            break;
        }
        r_data[1] = 0;
        break;
    }
    case LWNS_PROV_CMD_PROV_RESET:{
        r_data[1] = lwns_provisioned_reset();
        *r_len = 2;
        break;
    }
#endif
    default:
        r_data[1] = 0xfe;
        *r_len = 2;
        break;
    }
}

#endif

