#include <stdio.h>
#include <string.h>

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

    /* UDS�ͻ���API�����ɹ���λһ��UDS�ͻ��ˣ���ֹUDS�ͻ���API�쳣 */
    dm_api_master_reset(om_api);

    /* UDS�ͻ������ýṹ�� */
    dm_udsc_config config;
    /* 
       ������Чʱ�䣬���յ�ipc������ȶ�����ķ���ʱ��͵�ǰʱ�� 
       ���ʱ�䳬�����ʱ�䣬���Ϊ��Ч����
    */
    config.cmd_valid_time = 100; /* unit ms */
    int udscid = dm_api_udsc_create(om_api, &config);
    if (udscid >= 0) {
       demo_printf("UDS client create success => %d \n", udscid);
    }

    /* ================================================================= */
    /* ----------------------------------------------------------------- */
    /* UDS�������ýṹ�� */
    service_config service;

    memset(&service, 0, sizeof(service));
    /* ��Ϸ����еĿɱ����ݣ�UDS �ͻ��˽����� sid sub did������Զ�����UDS�������� */
    service.variableByte = ByteArrayNew();
    /* Ԥ�������Ӧ���ݣ������жϵ�ǰ��Ϸ���ִ���Ƿ����Ԥ�� */
    service.expectResponByte = ByteArrayNew();
    /* ��Ӧ����ƥ�����ݣ������жϵ�ǰ��Ϸ����Ƿ���Ҫ�ظ�ִ�� */
    service.finishByte = ByteArrayNew();
    /* �������Դ��ַ */
    service.sa = 0x11223344;
    /* �������Ŀ�ĵ�ַ */
    service.ta = 0x55667788;
    /* ��Ϸ���ID */
    service.sid = 0x22;
    /* DID $22 $2e $31�����ʹ�� */
    service.did = 0x1122;
    /* ��ʱ�೤ʱ���ִ�������Ϸ���unit ms */
    service.delay = 500;
    /* ������Ӧ��ʱʱ��unit ms */
    service.timeout = 100;
    /* ���һ��������� */
    if (dm_api_udsc_service_config(om_api, udscid, &service) == 0) {
        demo_printf("UDS service config success. \n");
    }
    /* UDS��������ӽ������ͷ��ڴ� */
    ByteArrayDelete(service.variableByte);
    ByteArrayDelete(service.expectResponByte);
    ByteArrayDelete(service.finishByte);
    /* ----------------------------------------------------------------- */
    /* ================================================================= */
    
    return 0;
}

