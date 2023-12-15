#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "demo.h"
#include "dm_common.h"
#include "dm_api.h"

void dm_uds_request_transfer_callback(void *arg, unsigned short udscid, const unsigned char *msg, unsigned int msg_len, unsigned int sa, unsigned int ta, unsigned int tatype);

int main(int argc, char *argv[])
{
    /* 创建UDS客户端API */
    diag_master_api *om_api = dm_api_create();
    if (om_api) {
        demo_printf("UDS client api create success. \n");
    }

    /* UDS客户端API创建成功后复位一下UDS客户端，防止UDS客户端API异常 */
    dm_api_master_reset(om_api);

    /* UDS客户端配置结构体 */
    dm_udsc_config config;
    /* 
       命令有效时间，接收到ipc命令后会比对命令的发出时间和当前时间 
       如果时间超过这个时间，命令将为无效丢弃
    */
    config.cmd_valid_time = 100; /* unit ms */
    int udscid = dm_api_udsc_create(om_api, &config);
    if (udscid >= 0) {
       demo_printf("UDS client create success => %d \n", udscid);
    }

    /* UDS服务配置结构体 */
    service_config service;

    memset(&service, 0, sizeof(service));
    /* 诊断服务中的可变数据，UDS 客户端将根据 sid sub did和这个自动构建UDS请求数据 */
    service.variableByte = ByteArrayNew();
    /* 预期诊断响应数据，用于判断当前诊断服务执行是否符合预期 */
    service.expectResponByte = ByteArrayNew();
    /* 响应结束匹配数据，用于判断当前诊断服务是否需要重复执行 */
    service.finishByte = ByteArrayNew();
    /* 诊断请求源地址 */
    service.sa = 0x11223344;
    /* 诊断请求目的地址 */
    service.ta = 0x55667788;
    /* 诊断服务ID */
    service.sid = 0x22;
    /* DID $22 $2e $31服务会使用 */
    service.did = 0x1122;
    /* 延时多长时间后执行这个诊断服务unit ms */
    service.delay = 500;
    /* 服务响应超时时间unit ms */
    service.timeout = 100;
    /* 添加一个诊断请求 */
    if (dm_api_udsc_service_config(om_api, udscid, &service) == 0) {
        demo_printf("UDS service config success. \n");
    }
    /* UDS服务项添加结束，释放内存 */
    ByteArrayDelete(service.variableByte);
    ByteArrayDelete(service.expectResponByte);
    ByteArrayDelete(service.finishByte);
    
    /* ================================================================= */
    /* ----------------------------------------------------------------- */
    udsc_general_config gconfig;
    memset(&gconfig, 0, sizeof(gconfig));
    /* M这个函数是必须调用的 uds客户端通用配置 */
    dm_api_udsc_general_config(om_api, udscid, &gconfig);
    
    /* M这个函数是必须调用的 接收OTA MASTER的诊断请求报文并通过doIP或者doCAN发送 */
    dm_api_udsc_request_transfer_callback_set(om_api, udscid, dm_uds_request_transfer_callback, 0/* 用户数据指针 */);
    
    /* M这个函数是必须调用的 启动UDS客户端开始执行诊断任务 */
    dm_api_udsc_start(om_api, udscid);

    /* 监听获取OTA MASTER进程消息 */
    while (1) {
        fd_set readfds;
        struct timeval timeout;
        int sret = 0;
        int sockfd = dm_api_sockfd(om_api);
    
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        if ((sret = select(sockfd + 1, &readfds, NULL, NULL, &timeout)) > 0 &&\
            FD_ISSET(sockfd, &readfds)) {
            /* M这个函数是必须调用的接收OTA MASTER进程间通信数据 */
            dm_api_request_event_loop(om_api);
        }
    }
    /* ----------------------------------------------------------------- */
    /* ================================================================= */
    
    return 0;
}

void dm_uds_request_transfer_callback(void *arg, unsigned short udscid, const unsigned char *msg, unsigned int msg_len, unsigned int sa, unsigned int ta, unsigned int tatype)
{
    demo_printf("UDS SA: 0x%08X TA: 0x%08X \n", sa, ta);
    demo_hex_printf("UDS Service Request", msg, msg_len);
    /* 将诊断请求通过doCAN或者doIP发送send msg to CAN/IP */
    /* doIP_send_diag_request(msg, msg_len) */
    /* doCAN_send_diag_request(msg, msg_len) */
}

