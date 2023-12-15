#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>  
#include <ws2tcpip.h>
#endif /* _WIN32 */
#include <sys/types.h>
#ifdef __linux__
#include <sys/select.h>
#include <sys/socket.h>
#endif /* __linux__ */

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
    while (1) {
        config.cmd_valid_time = 100; /* unit ms */
        int udscid = dm_api_udsc_create(om_api, 0);
        if (udscid >= 0) {
           demo_printf("UDS client create success => %d \n", udscid);
        }
        
        doipc_config_t dcfg;

        dcfg.ver = 2;
        dcfg.sa_addr = 0x1122;
        dcfg.src_ip = 1111111111;
        dcfg.src_port = 13400;    
        dcfg.dst_ip = 2222222222;
        dcfg.dst_port = 13400;
        dm_api_doipc_create(om_api, udscid, &dcfg);        
        sleep(1);        
        dm_api_doipc_create(om_api, udscid, &dcfg);        
        sleep(1);
        dm_api_udsc_destory(om_api, udscid);
        sleep(1);
    }
    
    return 0;
}
