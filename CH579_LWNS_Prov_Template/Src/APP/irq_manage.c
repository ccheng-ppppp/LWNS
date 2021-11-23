/*
 * irq_manage.c
 *  �жϹ���
 *  Created on: Sep 17, 2021
 *      Author: WCH
 */
#include "irq_manage.h"

static uint32_t irq_status = 0;

/*********************************************************************
 * @fn      irq_disable_all
 *
 * @brief   �ر������жϣ������ж�ֵ.
 *
 * @param   None.
 *
 * @return  None.
 */
void irq_disable_all(void) {
    SYS_DisableAllIrq(&irq_status);
}

/*********************************************************************
 * @fn      irq_enable
 *
 * @brief   �ָ������ж�.
 *
 * @param   None.
 *
 * @return  None.
 */
void irq_enable(void) {
    SYS_RecoverIrq(irq_status);
}
