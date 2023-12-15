#include <stdio.h>

#include "demo.h"
#include "dm_common.h"
#include "dm_api.h"

int main(int argc, char *argv[])
{
    dm_udsc_config config;
    service_config srv_config;
    udsc_general_config gen_config;
    int udscid = -1;
    
    //dm_debug_enable(1);

    memset(&srv_config, 0, sizeof(srv_config));
    memset(&gen_config, 0, sizeof(gen_config));
    diag_master_api *omapi = dm_api_create();
    if (omapi) {
        demo_printf("diag_master_api * dm_api_create() - PASS \n");
    }
    else {
        demo_printf("diag_master_api * dm_api_create() - FAIL \n");
    }
    diag_master_api *omapi1 = dm_api_create();
    if (omapi1 == 0) {
        demo_printf("diag_master_api * dm_api_create(1) - FAIL \n");
    }
    diag_master_api *omapi2 = dm_api_create();
    if (omapi2 == 0) {
        demo_printf("diag_master_api * dm_api_create(2) - FAIL \n");
    }
    diag_master_api *omapi3 = dm_api_create();    
    if (omapi2 == 0) {
        demo_printf("diag_master_api * dm_api_create(3) - FAIL \n");
    }
    diag_master_api *omapi4 = dm_api_create();
    if (omapi4 == 0) {
        demo_printf("diag_master_api * dm_api_create(4) - FAIL \n");
    }

    if (dm_api_sockfd(omapi) > 0) {
        demo_printf("int dm_api_sockfd(%p) - PASS \n", omapi);
    }
    else {
        demo_printf("int dm_api_sockfd(%p) - FAIL \n", omapi);
    }

    if (dm_api_master_reset(omapi) == 0) {
        demo_printf("int dm_api_master_reset(%p) - PASS \n", omapi);
    }
    else {
        demo_printf("int dm_api_master_reset(%p) - FAIL \n", omapi);
    }

    udscid = dm_api_udsc_create(omapi, &config);
    if (udscid >= 0) {
        demo_printf("int dm_api_udsc_create(%p, %p) - PASS \n", omapi, &config);
    }
    else {
        demo_printf("int dm_api_udsc_create(%p, %p) - FAIL \n", omapi, &config);
    }

    if (dm_api_udsc_service_config(omapi, udscid, &srv_config) == 0) {
        demo_printf("int dm_api_udsc_service_config(%p, %d, %p) - PASS \n", omapi, udscid, &srv_config);
    }
    else {
        demo_printf("int dm_api_udsc_service_config(%p, %d, %p) - FAIL \n", omapi, udscid, &srv_config);
    }

    if (dm_api_udsc_general_config(omapi, udscid, &gen_config) == 0) {
        demo_printf("int dm_api_udsc_general_config(%p, %d, %p) - PASS \n", omapi, udscid, &gen_config);
    }
    else {
        demo_printf("int dm_api_udsc_general_config(%p, %d, %p) - FAIL \n", omapi, udscid, &gen_config);
    }

    if (dm_api_udsc_start(omapi, udscid) == 0) {
        demo_printf("int dm_api_udsc_start(%p, %d) - PASS \n", omapi, udscid);
    }
    else {
        demo_printf("int dm_api_udsc_start(%p, %d) - FAIL \n", omapi, udscid);
    }

    if (dm_api_udsc_sa_key(omapi, udscid, 0x0a, "\x11\x22\x33\x44", 4) == 0) {
        demo_printf("int dm_api_udsc_sa_key(%p, %d, %02x, byte array, %d) - PASS \n", omapi, udscid, 0x0a, 4);
    }
    else {
        demo_printf("int dm_api_udsc_sa_key(%p, %d, %02x, byte array, %d) - FAIL \n", omapi, udscid, 0x0a, 4);
    }

    if (dm_api_udsc_stop(omapi, udscid) == 0) {
        demo_printf("int dm_api_udsc_stop(%p, %d) - PASS \n", omapi, udscid);
    }
    else {
        demo_printf("int dm_api_udsc_stop(%p, %d) - FAIL \n", omapi, udscid);
    }

    if (dm_api_udsc_destory(omapi, udscid) == 0) {
        demo_printf("int dm_api_udsc_destory(%p, %d) - PASS \n", omapi, udscid);
    }
    else {
        demo_printf("int dm_api_udsc_destory(%p, %d) - FAIL \n", omapi, udscid);
    }

    /* 无效UDS客户端ID测试 */
    if (dm_api_udsc_destory(omapi, 0xffff) == 0) {
        demo_printf("int dm_api_udsc_destory(%p, %d) - FAIL \n", omapi, 0xffff);
    }
    else {        
        demo_printf("int dm_api_udsc_destory(%p, %d) - PASS \n", omapi, 0xffff);
    }
    if (dm_api_udsc_service_config(omapi, 0xffff, &srv_config) == 0) {
        demo_printf("int dm_api_udsc_service_config(%p, %d, %p) - FAIL \n", omapi, 0xffff, &srv_config);
    }
    else {
        demo_printf("int dm_api_udsc_service_config(%p, %d, %p) - PASS \n", omapi, 0xffff, &srv_config);
    }

    if (dm_api_udsc_general_config(omapi, 0xffff, &gen_config) == 0) {
        demo_printf("int dm_api_udsc_general_config(%p, %d, %p) - FAIL \n", omapi, 0xffff, &gen_config);
    }
    else {
        demo_printf("int dm_api_udsc_general_config(%p, %d, %p) - PASS \n", omapi, 0xffff, &gen_config);
    }

    if (dm_api_udsc_start(omapi, 0xffff) == 0) {
        demo_printf("int dm_api_udsc_start(%p, %d) - FAIL \n", omapi, 0xffff);
    }
    else {
        demo_printf("int dm_api_udsc_start(%p, %d) - PASS \n", omapi, 0xffff);
    }

    if (dm_api_udsc_sa_key(omapi, 0xffff, 0x0a, "\x11\x22\x33\x44", 4) == 0) {
        demo_printf("int dm_api_udsc_sa_key(%p, %d, %02x, byte array, %d) - FAIL \n", omapi, 0xffff, 0x0a, 4);
    }
    else {
        demo_printf("int dm_api_udsc_sa_key(%p, %d, %02x, byte array, %d) - PASS \n", omapi, 0xffff, 0x0a, 4);
    }

    if (dm_api_udsc_stop(omapi, 0xffff) == 0) {
        demo_printf("int dm_api_udsc_stop(%p, %d) - FAIL \n", omapi, 0xffff);
    }
    else {
        demo_printf("int dm_api_udsc_stop(%p, %d) - PASS \n", omapi, 0xffff);
    }

    if (dm_api_udsc_destory(omapi, 0xffff) == 0) {
        demo_printf("int dm_api_udsc_destory(%p, %d) - FAIL \n", omapi, 0xffff);
    }
    else {
        demo_printf("int dm_api_udsc_destory(%p, %d) - PASS \n", omapi, 0xffff);
    }

    for ( ; ; ) {
        ;
        diag_master_api *omapi = dm_api_create();
        if (omapi) {
            demo_printf("FREE diag master api \n");
            dm_api_destory(omapi);
        }
    }
}

