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
    
    /* ================================================================= */
    /* ----------------------------------------------------------------- */
    udsc_general_config gconfig;
    memset(&gconfig, 0, sizeof(gconfig));
    /* M��������Ǳ�����õ� uds�ͻ���ͨ������ */
    dm_api_udsc_general_config(om_api, udscid, &gconfig);
    
    /* M��������Ǳ�����õ� ����OTA MASTER����������Ĳ�ͨ��doIP����doCAN���� */
    dm_api_udsc_request_transfer_callback_set(om_api, udscid, dm_uds_request_transfer_callback, 0/* �û�����ָ�� */);
    
    /* M��������Ǳ�����õ� ����UDS�ͻ��˿�ʼִ��������� */
    dm_api_udsc_start(om_api, udscid);

    /* ������ȡOTA MASTER������Ϣ */
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
            /* M��������Ǳ�����õĽ���OTA MASTER���̼�ͨ������ */
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
    /* ���������ͨ��doCAN����doIP����send msg to CAN/IP */
    /* doIP_send_diag_request(msg, msg_len) */
    /* doCAN_send_diag_request(msg, msg_len) */
}

