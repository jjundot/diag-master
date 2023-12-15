#include <stdio.h>

#include "demo.h"
#include "dm_common.h"
#include "dm_api.h"

int main(int argc, char *argv[])
{
    /* ����UDS�ͻ���API */
    diag_master_api *om_api = dm_api_create();
    if (om_api) {
        demo_printf("UDS client api create success. \n");
    }

    /* ================================================================= */
    /* ----------------------------------------------------------------- */
    /* UDS�ͻ���API�����ɹ���λһ��UDS�ͻ��ˣ���ֹUDS�ͻ���API�쳣 */
    dm_api_master_reset(om_api);

    dm_udsc_config config;
    /* 
       ������Чʱ�䣬���յ�ipc������ȶ�����ķ���ʱ��͵�ǰʱ�� 
       ���ʱ�䳬�����ʱ�䣬�����Ϊ��Ч������
    */
    config.cmd_valid_time = 100; /* unit ms */
    int udscid = dm_api_udsc_create(om_api, &config);
    if (udscid >= 0) {
       demo_printf("UDS client create success => %d \n", udscid);
    }
    /* ----------------------------------------------------------------- */
    /* ================================================================= */
    
    return 0;
}

