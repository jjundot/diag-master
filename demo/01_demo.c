#include <stdio.h>

#include "demo.h"
#include "dm_common.h"
#include "dm_api.h"

int main(int argc, char *argv[])
{
    /* ================================================================= */
    /* ----------------------------------------------------------------- */
    /* 创建UDS客户端API */
    diag_master_api *om_api = dm_api_create();
    if (om_api) {
        demo_printf("UDS client api create success. \n");
    }    
    /* ----------------------------------------------------------------- */
    /* ================================================================= */

    return 0;
}

