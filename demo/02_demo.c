#include <stdio.h>

#include "demo.h"
#include "dm_common.h"
#include "dm_api.h"

int main(int argc, char *argv[])
{
    /* 创建UDS客户端API */
    diag_master_api *om_api = dm_api_create();
    if (om_api) {
        demo_printf("UDS client api create success. \n");
    }

    /* ================================================================= */
    /* ----------------------------------------------------------------- */
    /* UDS客户端API创建成功后复位一下UDS客户端，防止UDS客户端API异常 */
    dm_api_master_reset(om_api);

    dm_udsc_config config;
    /* 
       命令有效时间，接收到ipc命令后会比对命令的发出时间和当前时间 
       如果时间超过这个时间，命令将视为无效丢弃。
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

