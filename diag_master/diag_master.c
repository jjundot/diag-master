#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#ifdef __linux__
#include <sys/select.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif /* __linux__ */

#include "config.h"
#ifdef __HAVE_LIBUV__
#include "uv.h"
#endif /* __HAVE_LIBUV__ */
#ifdef __HAVE_LIBEV__
#include "ev.h"
#ifdef _WIN32
#include <winsock2.h>  
#include <ws2tcpip.h>
#endif /* _WIN32 */
#endif /* __HAVE_LIBEV__ */

#include "dm_udsc_types.h"
#include "dm_common.h"
#include "dm_uds_client.h"
#include "dm_doip_client.h" 
#include "diag_master.h"

#ifdef _WIN32
#define DMS_LOCK   { 
#define DMS_UNLOCK   }
#else /* _WIN32 */
static pthread_mutex_t dms_mutex_lock = PTHREAD_MUTEX_INITIALIZER;
#define DMS_LOCK   { pthread_mutex_lock(&dms_mutex_lock)
#define DMS_UNLOCK   pthread_mutex_unlock(&dms_mutex_lock);}
#endif /* _WIN32 */

static uds_client *diag_master_udsc(diag_master_t *diag_master, INT16U id)
{
    if (!(id < diag_master->udsc_cnt) ||\
        diag_master->udscs[id] == NULL || \
        diag_master->udscs[id]->isidle == true) {
        return NULL;
    }
        
    return diag_master->udscs[id];
}

#ifdef __HAVE_LIBEV__
static void diag_master_ev_timer_keepalive_handler(struct ev_loop *loop, ev_timer *w, int revents);
#endif /* __HAVE_LIBEV__ */

#ifdef __HAVE_LIBUV__
static void diag_master_ev_timer_keepalive_handler(uv_timer_t* w);
#endif /* __HAVE_LIBUV__ */

static void diag_master_keepalive_refresh(diag_master_t *diag_master)
{
    diag_master->keepalive_cnt = 0;
#ifdef __HAVE_LIBUV__
    uv_timer_stop(&diag_master->keepalive_watcher);
    uv_timer_start(&diag_master->keepalive_watcher, diag_master_ev_timer_keepalive_handler, \
        diag_master->keepalive_interval, diag_master->keepalive_interval);
#endif /* __HAVE_LIBUV__ */
#ifdef __HAVE_LIBEV__
    ev_timer_stop(diag_master->loop, &diag_master->keepalive_watcher);
    ev_timer_init(&diag_master->keepalive_watcher, diag_master_ev_timer_keepalive_handler, \
        diag_master->keepalive_interval * 0.001, diag_master->keepalive_interval * 0.001);
    ev_timer_start(diag_master->loop, &diag_master->keepalive_watcher);
#endif /* __HAVE_LIBEV__ */
}

static int diag_master_sendto_api(diag_master_t *diag_master, unsigned char *buff, unsigned int size)
{
#ifdef __HAVE_LIBUV__
    uv_udp_send_t req;
    struct sockaddr_in addr;

    uv_buf_t buf = uv_buf_init(buff, size);

    uv_ip4_addr(diag_master->ip_str, diag_master->api_port, &addr);
    uv_udp_send(&diag_master->udp_send, &diag_master->iwrite_watcher, &buf, 1, (const struct sockaddr*)&addr, NULL);

    return size;
#endif /* __HAVE_LIBUV__ */

#ifdef __HAVE_LIBEV__
#ifdef __linux__
    struct sockaddr_un un;
    ssize_t bytesSent = -1;
    
    if (diag_master->sockfd < 0) return -1;
    
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    snprintf(un.sun_path, sizeof(un.sun_path), diag_master->dmapi_path, strlen(diag_master->dmapi_path));    
    bytesSent = sendto(diag_master->sockfd, buff, size, 0,\
                        (struct sockaddr *)&un, sizeof(struct sockaddr_un));
    // log_d("diag master api unix path => %s \n", diag_master->dmapi_path);
    // log_hex_d("send", buff, bytesSent);
    return bytesSent;
#endif /* __linux__ */
#ifdef _WIN32
    int bytes = -1;

    SOCKADDR_IN remoteAddress;
    remoteAddress.sin_family = AF_INET;
    remoteAddress.sin_port = htons(diag_master->api_port);
    remoteAddress.sin_addr.S_un.S_addr = htonl(diag_master->api_ip);
    bytes = sendto(diag_master->sockfd, buff, size, 0, \
        (SOCKADDR*)&remoteAddress, sizeof(remoteAddress));

    return bytes;
#endif /* _WIN32 */
#endif /* __HAVE_LIBEV__ */
}

/* 停止diag master左右的事件循环，这将导致diag master事件循环退出 */
static void diag_master_ev_stop(diag_master_t *diag_master)
{ 
#ifdef __HAVE_LIBEV__
    ev_io_stop(diag_master->loop, &diag_master->iread_watcher);
    ev_io_stop(diag_master->loop, &diag_master->iwrite_watcher);    
    ev_timer_stop(diag_master->loop, &diag_master->keepalive_watcher);
#endif /* __HAVE_LIBEV__ */
#ifdef __HAVE_LIBUV__
    uv_udp_recv_stop(&diag_master->iread_watcher);
    uv_timer_stop(&diag_master->keepalive_watcher);
    uv_stop(diag_master->loop);
#endif /* __HAVE_LIBUV__ */
}

void diag_master_service_sa_seed_generate(void *arg/* 用户数据指针 */, INT16U id, INT8U level, INT8U *seed, INT16U seed_size)
{
    diag_master_service_sa_seed_request(arg, id, level, seed, seed_size);
}

INT32 diag_master_service_request_sent_callback(INT16U id, void *arg, const INT8U *data, INT32U size, INT32U sa, INT32U ta, INT32U tatype)
{
    diag_master_t *diag_master = arg;
    doip_client_t *doipc = NULL;

    /* 优先使用DOIP客户端 */
    if ((doipc = dm_udsc_doip_channel(diag_master_udsc(diag_master, id)))) {
        size = dm_doipc_diagnostic_request(doipc, sa, ta, data, size, 0);
    }
    else {
        diag_master_service_request(diag_master, id, data, size, sa, ta, tatype);
    }

    return size;
}

void diag_master_services_finish_callback(uds_client *udsc, udsc_finish_stat stat, void *arg, const INT8U *ind, INT32U indl, const INT8U *resp, INT32U respl)
{
    diag_master_t *diag_master = arg;
    INT32U recode = DM_CAMMAND_ERR_NO;

    if (stat == UDSC_UNEXPECT_RESPONSE_FINISH) {
        /* 非预期响应 */
        recode = DM_CAMMAND_ERR_UDS_RESPONSE_UNEXPECT;
    }
    else if (stat == UDSC_TIMEOUT_RESPONSE_FINISH) {
        /* 响应超时 */
        recode = DM_CAMMAND_ERR_UDS_RESPONSE_TIMEOUT;
    }
    if (ind) {
        log_hex_d("Services finish Request", ind, indl);
    }
    if (resp) {
        log_hex_d("Services finish Response", resp, respl);
    }

    diag_master_all_service_result(diag_master, udsc->id, recode, ind, indl, resp, respl);
}

static void diag_master_cammand_service_response_handler(diag_master_t *diag_master, INT32U recode, INT16U id)
{
    INT32U to_cmd = DM_CAMMAND_SERVICE_RESPONSE_REPLY;
    INT32U to_recode = DM_CAMMAND_ERR_NO;
    INT16U to_id = id;
    INT32U tv_sec = 0; INT32U tv_usec = 0;
    INT8U *byte_addr = NULL;
    INT8U *payload = NULL;
    service_item *sitem = NULL;
    INT32U A_SA = 0, A_TA = 0, payload_length = 0, TA_TYPE = 0;
    uds_client *udsc = diag_master_udsc(diag_master, id);

    log_d("Reply Command: 0x%02X(%s) Recode: %d(%s) udsc ID: %d \n", \
        to_cmd, dm_ipc_command_str(to_cmd), to_recode, dm_command_rcode_str(to_recode), to_id);
    dm_common_encode(diag_master->txbuf, sizeof(diag_master->txbuf), to_cmd, to_recode, to_id, tv_sec, tv_usec);
    diag_master_sendto_api(diag_master, diag_master->txbuf, DM_IPC_COMMON_MSG_SIZE);   

    byte_addr = diag_master->rxbuf + DM_IPC_COMMON_MSG_SIZE;
    A_SA = *(INT32U *)(byte_addr + SERVICE_RESPONSE_A_SA_OFFSET);
    A_TA = *(INT32U *)(byte_addr + SERVICE_RESPONSE_A_TA_OFFSET);
    payload_length = *(INT32U *)(byte_addr + SERVICE_RESPONSE_PAYLOAD_LEN_OFFSET);
    TA_TYPE = *(INT32U *)(byte_addr + SERVICE_RESPONSE_TA_TYPE_OFFSET);
    log_d("Mtype: %d A_SA: 0x%08X A_TA: 0x%08X TA_TYPE: %d pl: %d \n", 0, A_SA, A_TA, TA_TYPE, payload_length);
    payload = byte_addr + SERVICE_RESPONSE_PAYLOAD_OFFSET;
    log_hex_d("payload: ", payload, payload_length);        
    /* 判断一下是否需要把响应结果发送给diag master API处理 */
    sitem = dm_udsc_curr_service_item(udsc);
    if (sitem && sitem->rr_callid > 0) {
        if ((payload[0] == 0x7f && payload[1] != sitem->sid) && \
            (payload[0] != (sitem->sid | UDS_REPLY_MASK))) {
            /* 非当前诊断服务响应数据，忽略处理 */
        }
        else {
            diag_master_service_request_result(diag_master, id, payload, payload_length, sitem->rr_callid);
        }
    }
    /* 交由UDS客户端处理诊断响应             */
    dm_udsc_service_response_finish(udsc, SERVICE_RESPONSE_NORMAL, A_SA, A_TA, payload, payload_length);
}

static void diag_master_cammand_udsc_create_handler(diag_master_t *diag_master, INT32U recode, INT16U id)
{
    int udsc_index = 0;
    INT32U to_cmd = DM_CAMMAND_UDSC_CREATE_REPLY;
    INT32U to_recode = DM_CAMMAND_ERR_NO;
    INT16U to_id = 0;
    uds_client *udsc = NULL;    
    INT32U tv_sec = 0; INT32U tv_usec = 0;
    INT32U cmd_valid_time = 0; /* 命令有效时间,收发端超时的命令造成的异常 */

    cmd_valid_time = *(INT32U *)(diag_master->rxbuf + DM_IPC_UDSC_CREATE_CMD_VALID_TIME_OFFSET);
    /* 优先找到一个空闲的UDS客户端 */
    for (udsc_index = 0; \
         udsc_index < diag_master->udsc_cnt;\
         udsc_index++) {
        if (diag_master->udscs[udsc_index] && \
            diag_master->udscs[udsc_index]->isidle == true) {
            udsc = diag_master->udscs[udsc_index];
            break;
        }
    }

    /* 没有找到空闲的UDS客户端，创建一个新的UDS客户端使用 */
    if (udsc == NULL) {    
        udsc = dm_udsc_create();
        if (udsc) {            
            dm_udsc_ev_loop_set(udsc, diag_master_ev_loop(diag_master));
            for (udsc_index = diag_master->udsc_cnt; \
                 udsc_index < DM_UDSC_CAPACITY_MAX; \
                 udsc_index++) {
                if (diag_master->udscs[udsc_index] == NULL) {
                    udsc->id = udsc_index;
                    diag_master->udscs[udsc_index] = udsc;
                    diag_master->udsc_cnt++;
                    break;
                }
            }
        }
    }
    
    if (udsc != NULL) {
        udsc->cmd_valid_time = cmd_valid_time > IPC_CAMMAND_VALID_TIME ? cmd_valid_time : IPC_CAMMAND_VALID_TIME;
        to_id = udsc->id; /* 将创建的UDS客户端ID返回给diag master API使用 */
        udsc->isidle = false; /* 设置UDS客户端已经被使用 */                
        dm_udsc_request_sent_callback_set(udsc, diag_master_service_request_sent_callback, diag_master);
        dm_udsc_services_finish_callback_set(udsc, diag_master_services_finish_callback, diag_master);
    }
    else {
        log_d("udsc create error \n");
        to_recode = DM_CAMMAND_ERR_UDSC_CREATE;
    }

    if (to_recode != DM_CAMMAND_ERR_NO && udsc) {
        /* 创建UDS客户端失败 */
        log_d("udsc create failed \n");        
    }

    log_d("Reply Command: 0x%02X(%s) Recode: %d(%s) udsc ID: %d \n", \
        to_cmd, dm_ipc_command_str(to_cmd), to_recode, dm_command_rcode_str(to_recode), to_id);
    dm_common_encode(diag_master->txbuf, sizeof(diag_master->txbuf), to_cmd, to_recode, to_id, tv_sec, tv_usec);
    diag_master_sendto_api(diag_master, diag_master->txbuf, DM_IPC_COMMON_MSG_SIZE);

    return ;
}

static void diag_master_cammand_udsc_destory_handler(diag_master_t *diag_master, INT32U recode, INT16U id)
{
    INT32U to_cmd = DM_CAMMAND_UDSC_DESTORY_REPLY;
    INT32U to_recode = DM_CAMMAND_ERR_NO;
    INT16U to_id = id;
    INT32U tv_sec = 0; INT32U tv_usec = 0;
    doip_client_t *doipc = NULL;

    /* 释放绑定在UDS客户端上的DOIP客户端 */
    if ((doipc = dm_udsc_doip_channel(diag_master_udsc(diag_master, id)))) {
        dm_udsc_doip_channel_unbind(diag_master_udsc(diag_master, id));
        dm_doipc_destory(doipc);
    }

    /* 并不释放UDS客户端，只是把UDS客户端设置为空闲，减少反复的malloc和free */
    dm_udsc_reset(diag_master_udsc(diag_master, id));
    
    log_d("Reply Command: 0x%02X(%s) Recode: %d(%s) udsc ID: %d \n", \
        to_cmd, dm_ipc_command_str(to_cmd), to_recode, dm_command_rcode_str(to_recode), to_id);
    dm_common_encode(diag_master->txbuf, sizeof(diag_master->txbuf), to_cmd, to_recode, to_id, tv_sec, tv_usec);
    diag_master_sendto_api(diag_master, diag_master->txbuf, DM_IPC_COMMON_MSG_SIZE);

    return ;
}

static void diag_master_cammand_udsc_start_handler(diag_master_t *diag_master, INT32U recode, INT16U id)
{
    INT32U to_cmd = DM_CAMMAND_START_SCRIPT_REPLY;
    INT32U to_recode = DM_CAMMAND_ERR_NO;
    INT16U to_id = id;
    INT32U tv_sec = 0; INT32U tv_usec = 0;
    
    dm_udsc_services_start(diag_master_udsc(diag_master, id));
       
    log_d("Reply Command: 0x%02X(%s) Recode: %d(%s) udsc ID: %d \n", \
        to_cmd, dm_ipc_command_str(to_cmd), to_recode, dm_command_rcode_str(to_recode), to_id);
    dm_common_encode(diag_master->txbuf, sizeof(diag_master->txbuf), to_cmd, to_recode, to_id, tv_sec, tv_usec);
    diag_master_sendto_api(diag_master, diag_master->txbuf, DM_IPC_COMMON_MSG_SIZE);

    return ;
}

static void diag_master_cammand_udsc_stop_handler(diag_master_t *diag_master, INT32U recode, INT16U id)
{
    INT32U to_cmd = DM_CAMMAND_STOP_SCRIPT_REPLY;
    INT32U to_recode = DM_CAMMAND_ERR_NO;
    INT16U to_id = id;
    INT32U tv_sec = 0; INT32U tv_usec = 0;
    
    dm_udsc_services_stop(diag_master_udsc(diag_master, id));

    log_d("Reply Command: 0x%02X(%s) Recode: %d(%s) udsc ID: %d \n", \
        to_cmd, dm_ipc_command_str(to_cmd), to_recode, dm_command_rcode_str(to_recode), to_id);
    dm_common_encode(diag_master->txbuf, sizeof(diag_master->txbuf), to_cmd, to_recode, to_id, tv_sec, tv_usec);
    diag_master_sendto_api(diag_master, diag_master->txbuf, DM_IPC_COMMON_MSG_SIZE);

    return ;
}

static void diag_master_cammand_master_reset_handler(diag_master_t *diag_master, INT32U recode, INT16U id)
{
    INT32U to_cmd = DM_CAMMAND_DIAG_MASTER_RESET_REPLY;
    INT32U to_recode = DM_CAMMAND_ERR_NO;
    INT16U to_id = id;
    uds_client *udsc = NULL;
    INT32U tv_sec = 0; INT32U tv_usec = 0;
    INT16U udsc_index = 0;
    
    for (udsc_index = 0; udsc_index < diag_master->udsc_cnt; udsc_index++) {
        udsc = diag_master->udscs[udsc_index];
        if (udsc) {
            dm_udsc_services_stop(udsc);
            dm_udsc_reset(udsc);
        }
    }

    log_d("Reply Command: 0x%02X(%s) Recode: %d(%s) udsc ID: %d \n", \
        to_cmd, dm_ipc_command_str(to_cmd), to_recode, dm_command_rcode_str(to_recode), to_id);
    dm_common_encode(diag_master->txbuf, sizeof(diag_master->txbuf), to_cmd, to_recode, to_id, tv_sec, tv_usec);
    diag_master_sendto_api(diag_master, diag_master->txbuf, DM_IPC_COMMON_MSG_SIZE);

    return ;
}

static void diag_master_cammand_service_config_handler(diag_master_t *diag_master, INT32U recode, INT16U id)
{
    INT32U to_cmd = DM_CAMMAND_CONFIG_SCRIPT_REPLY;
    INT32U to_recode = DM_CAMMAND_ERR_NO;
    INT16U to_id = id;
    INT32U tv_sec = 0; INT32U tv_usec = 0;    
    service_config sconfig;
    service_item *si = NULL;
    INT8U *byte_addr = NULL;
    
    uds_client *udsc = diag_master_udsc(diag_master, id);    
    byte_addr = diag_master->rxbuf + DM_IPC_COMMON_MSG_SIZE;
    log_d("Service config: %s \n", byte_addr); 

    memset(&sconfig, 0, sizeof(sconfig));
    sconfig.variableByte = ByteArrayNew();
    sconfig.expectResponByte = ByteArrayNew();
    sconfig.finishByte = ByteArrayNew(); 
    ByteArrayClear(sconfig.variableByte);
    ByteArrayClear(sconfig.expectResponByte);
    ByteArrayClear(sconfig.finishByte);
    dm_service_config_decode(byte_addr, strlen((const char *)byte_addr), &sconfig);

    if ((si = dm_udsc_service_item_add(udsc, sconfig.desc))) {    
        dm_udsc_service_enable_set(si, true);  
        dm_udsc_service_sid_set(si, sconfig.sid);
        dm_udsc_service_sub_set(si, sconfig.sub);
        dm_udsc_service_did_set(si, sconfig.did);
        dm_udsc_service_delay_set(si, sconfig.delay);
        dm_udsc_service_timeout_set(si, sconfig.timeout);

        ByteArrayAppendArray(si->variable_byte, sconfig.variableByte);
        ByteArrayAppendArray(si->expect_byte, sconfig.expectResponByte);        
        ByteArrayAppendArray(si->finish_byte, sconfig.finishByte);
        si->issuppress = sconfig.issuppress;
        si->ta = sconfig.ta;
        si->sa = sconfig.sa;
        si->tatype = sconfig.tatype;
        si->issuppress = sconfig.issuppress;
        si->finish_rule = sconfig.finish_rule;
        si->response_rule = sconfig.expectRespon_rule;
        si->finish_num_max = sconfig.finish_num_max;
        si->rr_callid = sconfig.rr_callid;
        si->td_36.maxNumberOfBlockLength = sconfig.maxNumberOfBlockLength;
        if (si->sid == UDS_SERVICES_RD) {
            si->rd_34.dataFormatIdentifier = sconfig.service_34_rd.dataFormatIdentifier;
            si->rd_34.addressAndLengthFormatIdentifier = sconfig.service_34_rd.addressAndLengthFormatIdentifier;
            si->rd_34.memoryAddress = sconfig.service_34_rd.memoryAddress;            
            si->rd_34.memorySize = sconfig.service_34_rd.memorySize;            
            int flen = strlen(sconfig.local_path);
            si->td_36.local_path = malloc(flen + 1);
            if (si->td_36.local_path) {
#ifdef _WIN32
                strcpy_s(si->td_36.local_path, flen + 1, sconfig.local_path);
#else /* #ifdef _WIN32 */
                strcpy(si->td_36.local_path, sconfig.local_path);
#endif /* #ifdef _WIN32 */
                si->td_36.local_path[flen] = 0;
            }
        }
        else if (si->sid == UDS_SERVICES_RFT) {
            si->rft_38.modeOfOperation = sconfig.service_38_rft.modeOfOperation;
            si->rft_38.filePathAndNameLength = sconfig.service_38_rft.filePathAndNameLength;
            snprintf(si->rft_38.filePathAndName, sizeof(si->rft_38.filePathAndName), "%s", sconfig.service_38_rft.filePathAndName);
            si->rft_38.dataFormatIdentifier = sconfig.service_38_rft.dataFormatIdentifier;            
            si->rft_38.fileSizeParameterLength = sconfig.service_38_rft.fileSizeParameterLength;
            si->rft_38.fileSizeUnCompressed = sconfig.service_38_rft.fileSizeUnCompressed;
            si->rft_38.fileSizeCompressed = sconfig.service_38_rft.fileSizeCompressed;
            int flen = strlen(sconfig.local_path);
            si->td_36.local_path = malloc(flen + 1);
            if (si->td_36.local_path) {
#ifdef _WIN32
                strcpy_s(si->td_36.local_path, flen + 1, sconfig.local_path);
#else /* #ifdef _WIN32 */
                strcpy(si->td_36.local_path, sconfig.local_path);
#endif /* #ifdef _WIN32 */
                si->td_36.local_path[flen] = 0;
            }
        }
        else if (si->sid == UDS_SERVICES_SA) {
            dm_udsc_service_saseed_callback_set(udsc, diag_master_service_sa_seed_generate, diag_master);
        }
        
        dm_udsc_service_request_build(si);
    }
    else {
        to_recode = DM_CAMMAND_ERR_UDS_SERVICE_ADD;
        log_d("fatal error UDS service add failed \n");
    }
    
    ByteArrayDelete(sconfig.variableByte);
    ByteArrayDelete(sconfig.expectResponByte);
    ByteArrayDelete(sconfig.finishByte);
    log_d("Reply Command: 0x%02X(%s) Recode: %d(%s) udsc ID: %d \n", \
        to_cmd, dm_ipc_command_str(to_cmd), to_recode, dm_command_rcode_str(to_recode), to_id);
    dm_common_encode(diag_master->txbuf, sizeof(diag_master->txbuf), to_cmd, to_recode, to_id, tv_sec, tv_usec);
    diag_master_sendto_api(diag_master, diag_master->txbuf, DM_IPC_COMMON_MSG_SIZE);

    return ;
}

static void diag_master_cammand_general_config_handler(diag_master_t *diag_master, INT32U recode, INT16U id)
{
    INT32U to_cmd = DM_CAMMAND_OTA_GENERAL_CONFIG_REPLY;
    INT32U to_recode = DM_CAMMAND_ERR_NO;
    INT16U to_id = id;
    INT32U tv_sec = 0; INT32U tv_usec = 0;    
    udsc_general_config gconfig;
    INT8U *byte_addr = NULL;
    
    uds_client *udsc = diag_master_udsc(diag_master, id);
    byte_addr = diag_master->rxbuf + DM_IPC_COMMON_MSG_SIZE;
    log_d("Service config: %s \n", byte_addr); 

    memset(&gconfig, 0, sizeof(gconfig));
    dm_general_config_decode(byte_addr, strlen((const char *)byte_addr), &gconfig); 
    udsc->isFailAbort = gconfig.isFailAbort;
    udsc->isTpRefresh = gconfig.isTpRefresh;
    udsc->tpInterval = gconfig.tpInterval;
    udsc->tpEnable = gconfig.tpEnable;
    udsc->tpta = gconfig.tpta; /* 诊断目的地址 */
    udsc->tpsa = gconfig.tpsa; /* 诊断源地址 */  

    log_d("Reply Command: 0x%02X(%s) Recode: %d(%s) udsc ID: %d \n", \
        to_cmd, dm_ipc_command_str(to_cmd), to_recode, dm_command_rcode_str(to_recode), to_id);
    dm_common_encode(diag_master->txbuf, sizeof(diag_master->txbuf), to_cmd, to_recode, to_id, tv_sec, tv_usec);
    diag_master_sendto_api(diag_master, diag_master->txbuf, DM_IPC_COMMON_MSG_SIZE);

    return ;
}

static void diag_master_cammand_sa_key_handler(diag_master_t *diag_master, INT32U recode, INT16U id)
{
    uds_client *udsc = NULL;
    INT32U to_cmd = DM_CAMMAND_SERVICE_SA_KEY_REPLY;
    INT32U to_recode = DM_CAMMAND_ERR_NO;
    INT16U to_id = id;
    INT32U tv_sec = 0; INT32U tv_usec = 0;
    INT16U key_size = 0;
    INT8U *key = 0;
    INT8U level = 0;
    
    log_d("Reply Command: 0x%02X(%s) Recode: %d(%s) udsc ID: %d \n", \
        to_cmd, dm_ipc_command_str(to_cmd), to_recode, dm_command_rcode_str(to_recode), to_id);
    dm_common_encode(diag_master->txbuf, sizeof(diag_master->txbuf), to_cmd, to_recode, to_id, tv_sec, tv_usec);
    diag_master_sendto_api(diag_master, diag_master->txbuf, DM_IPC_COMMON_MSG_SIZE);

    level = *(INT8U *)(diag_master->rxbuf + SERVICE_SA_KEY_LEVEL_OFFSET);
    key_size = *(INT16U *)(diag_master->rxbuf + SERVICE_SA_KEY_SIZE_OFFSET);
    key = diag_master->rxbuf + SERVICE_SA_KEY_OFFSET; 
    log_d("Security access level %02x \n", level);
    log_hex_d("UDS Request key", key, key_size);
    udsc = diag_master_udsc(diag_master, id);
    if (udsc->common_var.key_byte) {
        ByteArrayAppendNChar(udsc->common_var.key_byte, key, key_size);
    }
    
    return ;
}

static void diag_master_cammand_doipc_create_handler(diag_master_t *diag_master, INT32U recode, INT16U id)
{
    INT32U to_cmd = DM_CAMMAND_DOIPC_CREATE_REPLY;
    INT32U to_recode = DM_CAMMAND_ERR_NO;
    INT16U to_id = id;
    INT32U tv_sec = 0; INT32U tv_usec = 0;
    doipc_config_t config;
    doip_client_t *doipc = NULL; int doipc_index = 0;
    uds_client *udsc = NULL;

    udsc = diag_master_udsc(diag_master, id);
    memset(&config, 0, sizeof(config));
    config.ver = *(INT8U *)(diag_master->rxbuf + DOIPC_CREATE_VERSION_OFFSET);    
    config.sa_addr = *(INT16U *)(diag_master->rxbuf + DOIPC_CREATE_SOURCE_ADDR_OFFSET);
    config.src_ip = *(INT32U *)(diag_master->rxbuf + DOIPC_CREATE_CLIENT_IP_OFFSET);
    config.src_port = *(INT16U *)(diag_master->rxbuf + DOIPC_CREATE_CLIENT_PORT_OFFSET);
    config.dst_ip = *(INT32U *)(diag_master->rxbuf + DOIPC_CREATE_SERVER_IP_OFFSET);
    config.dst_port = *(INT16U *)(diag_master->rxbuf + DOIPC_CREATE_SERVER_PORT_OFFSET);

    log_d("doip version:     %d\n", config.ver);
    log_d("doip source addr: %04x \n", config.sa_addr);
    log_d("doip src ip:      %d \n", config.src_ip);
    log_d("doip src port:    %d \n", config.src_port);
    log_d("doip dst ip:      %d \n", config.dst_ip);
    log_d("doip dst port:    %d \n", config.dst_port);
    /*  */
    doipc = dm_udsc_doip_channel(udsc);
    if (doipc == NULL) {
        /* 创建一个新的DOIP客户端 */
        doipc = dm_doipc_create(diag_master_ev_loop(diag_master));
    }
    /* doip客户端创建成功 */
    if (doipc) {
        memcpy(diag_master->txbuf + DOIPC_CREATE_DOIP_CLIENT_ID_OFFSET, \
            &doipc_index, DOIPC_CREATE_DOIP_CLIENT_ID_SIZE);
        /* UDS客户端和DOIP客户端绑定 */
        dm_udsc_doip_channel_bind(udsc, doipc);
        /* 直接连接 doip server */
        dm_doipc_connect_active_server(doipc, &config);
    }
    else {
        to_recode = DM_CAMMAND_ERR_DOIPC_CREATE_FAILED;
    }
    log_d("Reply Command: 0x%02X(%s) Recode: %d(%s) udsc ID: %d \n", \
    to_cmd, dm_ipc_command_str(to_cmd), to_recode, dm_command_rcode_str(to_recode), to_id);
    dm_common_encode(diag_master->txbuf, sizeof(diag_master->txbuf), to_cmd, to_recode, to_id, tv_sec, tv_usec);
    diag_master_sendto_api(diag_master, diag_master->txbuf, DOIPC_CREATE_REPLY_SIZE);

    return ;
}

int diag_master_keepalive_request(diag_master_t *diag_master)
{
    INT32U request_cmd = DM_CAMMAND_OMAPI_KEEPALIVE_REQUEST;
    INT32U recode = DM_CAMMAND_ERR_NO;
    INT32U tv_sec = 0; INT32U tv_usec = 0;
  
    dm_common_encode(diag_master->txbuf, sizeof(diag_master->txbuf), request_cmd, recode, 0, tv_sec, tv_usec);
    diag_master_sendto_api(diag_master, diag_master->txbuf, DM_IPC_COMMON_MSG_SIZE);
    
    return 0;
}

#ifdef __HAVE_LIBUV__
static void diag_master_ev_timer_keepalive_handler(uv_timer_t* w)
#endif /* __HAVE_LIBUV__ */
#ifdef __HAVE_LIBEV__
static void diag_master_ev_timer_keepalive_handler(struct ev_loop *loop, ev_timer *w, int revents)
#endif /* __HAVE_LIBEV__ */
{
#ifdef __HAVE_LIBUV__
    diag_master_t *diag_master = (diag_master*)((char*)w - offsetof(diag_master, keepalive_watcher));
#endif /* __HAVE_LIBUV__ */
#ifdef __HAVE_LIBEV__
    diag_master_t *diag_master = ev_userdata(loop);
#endif /* __HAVE_LIBEV__ */
    diag_master->keepalive_cnt++;
    diag_master_keepalive_request(diag_master);
    if (diag_master->keepalive_cnt > 10) {
        log_e("The diag master api is offline. The diag master exits \n");
        diag_master_ev_stop(diag_master);     
    }
}

#ifdef __HAVE_LIBUV__
static void diag_master_ev_io_om_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    diag_master_t *diag_master = (diag_master_t *)((char*)handle - offsetof(diag_master, iread_watcher));
    buf->base = diag_master->rxbuf;
    buf->len = sizeof(diag_master->rxbuf);
}

static void diag_master_ev_io_om_ipc_read(uv_udp_t *w, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags)
#endif /* #ifdef __HAVE_LIBUV__ */
#ifdef __HAVE_LIBEV__
static void diag_master_ev_io_om_ipc_read(struct ev_loop* loop, ev_io* w, int e)
#endif /* __HAVE_LIBEV__ */
{
#ifdef __HAVE_LIBUV__
    diag_master_t *diag_master = (diag_master*)((char*)w - offsetof(diag_master, iread_watcher));
#endif /* __HAVE_LIBUV__ */
#ifdef __HAVE_LIBEV__
    diag_master_t *diag_master = ev_userdata(loop);
#endif /* __HAVE_LIBEV__ */
    INT32U dm_cmd = 0; INT32U recode = 0; INT16U id = 0;
    INT32U tv_sec = 0; INT32U tv_usec = 0;
    int recvByte = -1;

#ifdef __HAVE_LIBUV__
    recvByte = nread;
    log_hex_d("RECV", buf->base, recvByte);
#endif /* __HAVE_LIBUV__ */
#ifdef __HAVE_LIBEV__
    recvByte = dm_recvfrom(diag_master->sockfd, diag_master->rxbuf, sizeof(diag_master->rxbuf), 0);
#endif /* __HAVE_LIBEV__ */
    if (recvByte > 0) {
        /* 刷新心跳定时器 */
        diag_master_keepalive_refresh(diag_master);
        dm_common_decode(diag_master->rxbuf, sizeof(diag_master->rxbuf), &dm_cmd, &recode, &id, &tv_sec, &tv_usec);
        if (dm_cmd != DM_CAMMAND_OMAPI_KEEPALIVE_REPLY) {
            log_d("Request Command: 0x%02X(%s) Recode: %d(%s) udsc ID: %d tv_sec: %d tv_usec: %d \n", \
                dm_cmd, dm_ipc_command_str(dm_cmd), recode, dm_command_rcode_str(recode), id, tv_sec, tv_usec);
        }
        if (DM_CMD_REPLY_MASK & dm_cmd) {
            /* 只处理请求消息，应答消息直接忽略 */
            if (dm_cmd == DM_CAMMAND_OMAPI_KEEPALIVE_REPLY) {
                /* 接收到心跳响应，刷写心跳计数器 */
                diag_master->keepalive_cnt = 0;
            }
            return ;
        }
        if (dm_cmd != DM_CAMMAND_UDSC_CREATE_REQUEST && \
            dm_cmd != DM_CAMMAND_DIAG_MASTER_RESET_REQUEST) {
            /* 需要用到uds客户端ID的命令事先判断ID是否有效 */
            uds_client *udsc = diag_master_udsc(diag_master, id);
            if (udsc == NULL) {
                log_w("The udsc id is invalid => %d \n", id);
                recode = DM_CAMMAND_ERR_UDSC_INVALID;
                goto REPLY;
            }
            if (udsc->cmd_valid_time > 0 && \
                dm_tv_currtdms(tv_sec, tv_usec) > udsc->cmd_valid_time) {
                log_w("When the command expires Ignore the command. \n");
                return ;    
            }
        }
        switch (dm_cmd) {
            case DM_CAMMAND_SERVICE_RESPONSE_REQUEST:
                diag_master_cammand_service_response_handler(diag_master, recode, id);
                break;
            case DM_CAMMAND_UDSC_CREATE_REQUEST:
                diag_master_cammand_udsc_create_handler(diag_master, recode, id);
                break;
            case DM_CAMMAND_UDSC_DESTORY_REQUEST:
                diag_master_cammand_udsc_destory_handler(diag_master, recode, id);
                break;
            case DM_CAMMAND_START_SCRIPT_REQUEST:
                diag_master_cammand_udsc_start_handler(diag_master, recode, id);                
                break;
            case DM_CAMMAND_STOP_SCRIPT_REQUEST:
                diag_master_cammand_udsc_stop_handler(diag_master, recode, id);                
                break;
            case DM_CAMMAND_DIAG_MASTER_RESET_REQUEST:
                diag_master_cammand_master_reset_handler(diag_master, recode, id);  
                break;
            case DM_CAMMAND_CONFIG_SCRIPT_REQUEST:
                diag_master_cammand_service_config_handler(diag_master, recode, id);
                break;
            case DM_CAMMAND_OTA_GENERAL_CONFIG_REQUEST:
                diag_master_cammand_general_config_handler(diag_master, recode, id);                
                break;
            case DM_CAMMAND_SERVICE_SA_KEY_REQUEST:
                diag_master_cammand_sa_key_handler(diag_master, recode, id);
                break;
            case DM_CAMMAND_DOIPC_CREATE_REQUEST:
                diag_master_cammand_doipc_create_handler(diag_master, recode, id);
                break;
            default:
                log_d("diag master IPC unknown cammand => %d \n", dm_cmd);    
                break;
        }
        memset(diag_master->rxbuf, 0, recvByte);
    }

    return ;
REPLY:
    log_d("Reply Command: 0x%02X(%s) Recode: %d(%s) udsc ID: %d \n", \
        dm_cmd | DM_CMD_REPLY_MASK, dm_ipc_command_str(dm_cmd | DM_CMD_REPLY_MASK), \
        recode, dm_command_rcode_str(recode), id);
    dm_common_encode(diag_master->txbuf, sizeof(diag_master->txbuf), dm_cmd | DM_CMD_REPLY_MASK, recode, id, tv_sec, tv_usec);
    diag_master_sendto_api(diag_master, diag_master->txbuf, DM_IPC_COMMON_MSG_SIZE);
    
    return ;    
}

#ifdef __HAVE_LIBEV__
static void diag_master_ipc_write(struct ev_loop *loop, ev_io *w, int e)
{

}
#endif /* __HAVE_LIBEV__ */

#ifdef _WIN32
static int diag_master_ipc_create(char *ip, short port)
{
    struct in_addr addr;
    if (inet_pton(AF_INET, ip, &addr) <= 0) {
        log_e("inet_pton() error \n");
        return -1;
    }

    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (!(udpSocket > 0)) {
        log_e("socket() error \n");
        return -2;
    }

    SOCKADDR_IN localAddress;
    localAddress.sin_family = AF_INET;
    localAddress.sin_port = htons(port); // 本地端口号  
    localAddress.sin_addr.S_un.S_addr = addr.s_addr;
    if (bind(udpSocket, (SOCKADDR*)&localAddress, sizeof(localAddress)) != 0) {
        closesocket(udpSocket);
        log_e("bind() error \n");
        return -3;
    }
    log_d("diag master udpSocket = %d \n", udpSocket);

    return udpSocket;
}
#endif /* #ifdef _WIN32 */
#ifdef __linux__
static int diag_master_ipc_create(struct sockaddr_un *un)
{
    int handle = -1;
    int flag = 0;

    if (!un) {
        goto CREATE_FALIED;
    }

    handle = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (handle < 0) {
        goto CREATE_FALIED;    
    }
    flag = fcntl(handle, F_GETFL, 0);
    fcntl(handle, F_SETFL, flag | O_NONBLOCK);

    unlink(un->sun_path);
    if (bind(handle, (struct sockaddr *)un, sizeof(struct sockaddr_un)) < 0) {
        goto CREATE_FALIED;
    }

    return handle;
CREATE_FALIED:
    if (handle > 0) {
        close(handle);
    }

    return -1;
}
#endif /* __linux__ */

/*
    UDS请求发送给diag master api，由diag master api负责把UDS请求发送给其他ECU
*/
int diag_master_service_request(diag_master_t *diag_master, INT16U id, const INT8U *data, INT32U size, INT32U sa, INT32U ta, INT32U tatype)
{
    INT8U *byte_addr = diag_master->txbuf + DM_IPC_COMMON_MSG_SIZE;
    INT32U request_cmd = DM_CAMMAND_SERVICE_INDICATION_REQUEST;
    INT32U recode = DM_CAMMAND_ERR_NO;
    INT32U tv_sec = 0; INT32U tv_usec = 0;
    
    if (sizeof(diag_master->txbuf) - DM_IPC_COMMON_MSG_SIZE - SERVICE_INDICATION_PAYLOAD_OFFSET < size) {
        log_d("Service request error txbuff is little. \n");
        goto REQUEST_FINISH;
    }

    /* encode 4byte UDS请求源地址 */
    memcpy(byte_addr + SERVICE_INDICATION_A_SA_OFFSET, &sa, SERVICE_INDICATION_A_SA_SIZE);    
    /* encode 4byte UDS请求目的地址 */
    memcpy(byte_addr + SERVICE_INDICATION_A_TA_OFFSET, &ta, SERVICE_INDICATION_A_TA_SIZE);    
    /* encode 4byte UDS请求目的地址类型 */
    memcpy(byte_addr + SERVICE_INDICATION_TA_TYPE_OFFSET, &tatype, SERVICE_INDICATION_TA_TYPE_SIZE);    
    /* encode 4byte UDS请求报文长度 */
    memcpy(byte_addr + SERVICE_INDICATION_PAYLOAD_LEN_OFFSET, &size, SERVICE_INDICATION_PAYLOAD_LEN_SIZE);    
    /* encode nbyte UDS请求报文 */
    memcpy(byte_addr + SERVICE_INDICATION_PAYLOAD_OFFSET, data, size);

    INT32U A_SA = *(INT32U *)(byte_addr + SERVICE_INDICATION_A_SA_OFFSET);
    INT32U A_TA = *(INT32U *)(byte_addr + SERVICE_INDICATION_A_TA_OFFSET);
    INT32U payload_length = *(INT32U *)(byte_addr + SERVICE_INDICATION_PAYLOAD_LEN_OFFSET);
    INT32U TA_TYPE = *(INT32U *)(byte_addr + SERVICE_INDICATION_TA_TYPE_OFFSET);
    log_d("Mtype: %d A_SA: 0x%08X A_TA: 0x%08X TA_TYPE: %d pl: %d \n", 0, A_SA, A_TA, TA_TYPE, payload_length);
    byte_addr += SERVICE_INDICATION_PAYLOAD_OFFSET;
    log_hex_d("UDS Request: ", byte_addr, payload_length);

REQUEST_FINISH:
    dm_common_encode(diag_master->txbuf, sizeof(diag_master->txbuf), request_cmd, recode, id, tv_sec, tv_usec);
    diag_master_sendto_api(diag_master, diag_master->txbuf, DM_IPC_COMMON_MSG_SIZE + SERVICE_INDICATION_PAYLOAD_OFFSET + size);

    return 0;
}

/*
    所有的UDS服务处理结束或者因异常中断执行UDS服务后，将结果发送给diag master api处理，
    结果包含最后一次UDS服务请求和应答数据
*/
int diag_master_all_service_result(diag_master_t *diag_master, INT16U id, INT32U result, const INT8U *ind, INT32U indl, const INT8U *resp, INT32U respl)
{
    INT32U request_cmd = DM_CAMMAND_SCRIPT_RESULT_REQUEST;
    INT32U recode = result;
    INT32U tv_sec = 0; INT32U tv_usec = 0;

    if (ind == NULL) {indl = 0;}
    if (resp == NULL) {respl = 0;}
    /* encode 4 byte 请求数据长度 */
    memcpy(diag_master->txbuf + SERVICE_FINISH_RESULT_IND_LEN_OFFSET, &indl, SERVICE_FINISH_RESULT_IND_LEN_SIZE);
    /* encode n byte 请求数据 */
    memcpy(diag_master->txbuf + SERVICE_FINISH_RESULT_IND_OFFSET, ind, indl);
    /* encode 4 byte 应答数据长度 */
    memcpy(diag_master->txbuf + SERVICE_FINISH_RESULT_IND_OFFSET + indl, &respl, SERVICE_FINISH_RESULT_RESP_LEN_SIZE);    
    /* encode n byte 应答数据 */
    memcpy(diag_master->txbuf + SERVICE_FINISH_RESULT_IND_OFFSET + indl + SERVICE_FINISH_RESULT_RESP_LEN_SIZE, resp, respl);
    /* encode 通用的头部固定数据 */
    dm_common_encode(diag_master->txbuf, sizeof(diag_master->txbuf), request_cmd, recode, id, tv_sec, tv_usec);
    /* 发送给diag master api */
    diag_master_sendto_api(diag_master, diag_master->txbuf, DM_IPC_COMMON_MSG_SIZE + \
        SERVICE_FINISH_RESULT_IND_OFFSET + indl + \
        SERVICE_FINISH_RESULT_RESP_LEN_SIZE + respl);

    return 0;
}

/* 
   如果diag master api注册了UDS服务结果处理函数，
   diag master 将UDS服务的应答数据，发送给 diag master api 处理 
*/
int diag_master_service_request_result(diag_master_t *diag_master, INT16U id, const INT8U *data, INT32U size, INT32U rr_callid)
{
    INT32U request_cmd = DM_CAMMAND_SERVICE_REQUEST_RESULT_REQUEST;
    INT32U recode = DM_CAMMAND_ERR_NO;
    INT32U tv_sec = 0; INT32U tv_usec = 0;

    /* 防止数据过长越界读写buff */
    if (size > sizeof(diag_master->txbuf) - SERVICE_REQUEST_RESULT_SIZE - DM_IPC_COMMON_MSG_SIZE) {
        size = sizeof(diag_master->txbuf) - SERVICE_REQUEST_RESULT_SIZE - DM_IPC_COMMON_MSG_SIZE;
        recode = DM_CAMMAND_ERR_UDS_RESPONSE_OVERLEN; /* 错误码 */
    }
    /* encode 4 byte diag master api管理的诊断结果处理回调函数ID */
    memcpy(diag_master->txbuf + SERVICE_REQUEST_RESULT_RR_CALLID_OFFSET, &rr_callid, SERVICE_REQUEST_RESULT_RR_CALLID_SIZE);    
    /* encode 4 byte 诊断结果数据长度 */
    memcpy(diag_master->txbuf + SERVICE_REQUEST_RESULT_DATA_LEN_OFFSET, &size, SERVICE_REQUEST_RESULT_DATA_LEN_SIZE);    
    /* encode n byte 诊断结果数据 */
    memcpy(diag_master->txbuf + SERVICE_REQUEST_RESULT_DATA_OFFSET, data, size);
    INT8U *payload = diag_master->txbuf + SERVICE_REQUEST_RESULT_DATA_OFFSET;
    log_hex_d("UDS Request result: ", payload, size);    
    /* encode 通用的头部固定数据 */
    dm_common_encode(diag_master->txbuf, sizeof(diag_master->txbuf), request_cmd, recode, id, tv_sec, tv_usec);
    diag_master_sendto_api(diag_master, diag_master->txbuf, DM_IPC_COMMON_MSG_SIZE + SERVICE_REQUEST_RESULT_SIZE + size);

    return 0;
}

int diag_master_service_sa_seed_request(diag_master_t *diag_master, INT16U id, INT8U level, const INT8U *data, INT32U size)
{
    INT32U request_cmd = DM_CAMMAND_SERVICE_SA_SEED_REQUEST;
    INT32U recode = DM_CAMMAND_ERR_NO;
    INT32U tv_sec = 0; INT32U tv_usec = 0;

    /* encode 1 byte level */
    memcpy(diag_master->txbuf + SERVICE_SA_SEED_LEVEL_OFFSET, &level, SERVICE_SA_SEED_LEVEL_SIZE);    
    /* encode 2 byte 种子长度 */
    memcpy(diag_master->txbuf + SERVICE_SA_SEED_SIZE_OFFSET, &size, SERVICE_SA_SEED_SIZE_SIZE);    
    /* encode n byte 种子数据 */
    memcpy(diag_master->txbuf + SERVICE_SA_SEED_OFFSET, data, size);    
    log_hex_d("UDS Request seed: ", data, size);    
    /* encode 通用的头部固定数据 */
    dm_common_encode(diag_master->txbuf, sizeof(diag_master->txbuf), request_cmd, recode, id, tv_sec, tv_usec);
    diag_master_sendto_api(diag_master, diag_master->txbuf, SERVICE_SA_SEED_OFFSET + size);

    return 0;
}

diag_master_t *diag_master_create(const char *dm_path, const char *dmapi_path)
{
    int ret = 0;
    int udsc_id = 0;
#ifdef __linux__ 
    struct sockaddr_un un;
#endif /* __linux__  */
    if (dm_path == NULL || \
        dmapi_path == NULL) {
        return NULL;
    }
    diag_master_t *diag_master = DM_MALLOC(sizeof(*diag_master));
    if (diag_master == NULL) {
        return NULL;
    }
    memset(diag_master, 0, sizeof(*diag_master));
    diag_master->index = -1;
    diag_master->sockfd = -1;
#ifdef   __HAVE_LIBUV__
    diag_master->loop = uv_loop_new();
#endif /* __HAVE_LIBUV__ */
#ifdef __HAVE_LIBEV__
    diag_master->loop = ev_loop_new(0);
#endif /* __HAVE_LIBEV__ */
    if (diag_master->loop == NULL) {
        log_e("new event loop failed  \n");
        goto CREAT_FAILED;
    }

    diag_master->keepalive_interval = 1000; /* ms */
    diag_master->dms = NULL;
#ifdef _WIN32
    /* win上使用UDP socket通信，传入的地址参数格式为ip:port例如127.0.0.1:111 */
    char *om_ip = NULL, *dmi_ip = NULL;
    char *om_port = NULL, *dmi_port = NULL;
    char *context = NULL;
    char ompath_tmp[128] = { 0 };
    snprintf(ompath_tmp, sizeof(ompath_tmp), "%s", dm_path);
    context = NULL;
    om_ip = strtok_s(ompath_tmp, ":", &context);
    om_port = strtok_s(NULL, "", &context);
    if (om_ip == NULL || om_port == NULL) {
        log_e("diag master address format error \n");
        goto CREAT_FAILED;
    }

    char omipath_tmp[128] = { 0 };
    snprintf(omipath_tmp, sizeof(omipath_tmp), "%s", dmapi_path);
    context = NULL;
    dmi_ip = strtok_s(omipath_tmp, ":", &context);
    dmi_port = strtok_s(NULL, "", &context);
    if (dmi_ip == NULL || dmi_port == NULL) {
        log_e("diag master api address format error \n");
        goto CREAT_FAILED;
    }
    struct in_addr addr;
    if (inet_pton(AF_INET, dmi_ip, &addr) <= 0) {
        log_e("inet_pton() error \n");
        goto CREAT_FAILED;
    }
    snprintf(diag_master->ip_str, sizeof(diag_master->ip_str), "%s", dmi_ip);
    diag_master->api_ip = ntohl(addr.s_addr);
    diag_master->api_port = atoi(dmi_port);
#endif /* _WIN32 */
#ifdef __HAVE_LIBUV__
    struct sockaddr_in upd_addr;
    /* 监听UDP读事件 */
    if ((ret = uv_udp_init(diag_master->loop, &diag_master->iread_watcher)) != 0) {
        log_e("uv_udp_init error: %s \n", uv_strerror(ret));
        goto CREAT_FAILED;
    }
    log_d("diag master recv ip4 addr => %s:%d \n", om_ip, atoi(om_port));
    if ((ret = uv_ip4_addr(om_ip, atoi(om_port), &upd_addr)) != 0) {
        log_e("uv_ip4_addr error: %s \n", uv_strerror(ret));
        goto CREAT_FAILED;
    }
    if ((ret = uv_udp_bind(&diag_master->iread_watcher, (const struct sockaddr*)&upd_addr, UV_UDP_REUSEADDR)) != 0) {
        log_e("uv_udp_bind error: %s \n", uv_strerror(ret));
        goto CREAT_FAILED;
    }
    if ((ret = uv_udp_recv_start(&diag_master->iread_watcher, diag_master_ev_io_om_alloc, diag_master_ev_io_om_ipc_read)) != 0) {
        log_e("uv_udp_recv_start error: %s \n", uv_strerror(ret));
        goto CREAT_FAILED;
    }
    if ((ret = uv_udp_init(diag_master->loop, &diag_master->iwrite_watcher)) != 0) {
        log_e("uv_udp_init error: %s \n", uv_strerror(ret));
        goto CREAT_FAILED;
    }
    if ((ret = uv_timer_init(diag_master->loop, &diag_master->keepalive_watcher)) != 0) {
        log_e("uv_timer_init error: %s \n", uv_strerror(ret));
        goto CREAT_FAILED;
    }
    if ((ret = uv_timer_start(&diag_master->keepalive_watcher, diag_master_ev_timer_keepalive_handler, \
        diag_master->keepalive_interval, diag_master->keepalive_interval)) != 0) {
        log_e("uv_timer_start error: %s \n", uv_strerror(ret));
        goto CREAT_FAILED;
    }
#endif /* __HAVE_LIBUV__  */
#ifdef __HAVE_LIBEV__
#ifdef __linux__ 
    memset(&un, 0, sizeof(struct sockaddr_un));
    un.sun_family = AF_UNIX;
    memcpy(un.sun_path, dm_path, strlen(dm_path));
    diag_master->sockfd = diag_master_ipc_create(&un);
    if (diag_master->sockfd < 0) {
        log_e("fatal error diag_master_ipc_create failed \n");
        goto CREAT_FAILED;
    }
    /* 保存diag master的unix socket的路径 */
    snprintf(diag_master->dm_path, sizeof(diag_master->dm_path), "%s", dm_path);

    /* 保存diag master api的unix socket的路径 */
    snprintf(diag_master->dmapi_path, sizeof(diag_master->dmapi_path), "%s", dmapi_path);
#endif /* __linux__  */
#ifdef _WIN32
    diag_master->sockfd = diag_master_ipc_create(om_ip, atoi(om_port));
    if (diag_master->sockfd < 0) {
        log_e("fatal error diag_master_ipc_create failed \n");
        goto CREAT_FAILED;
    }
#endif /* _WIN32 */

    /* 传入用户数据指针, 便于在各个回调函数内使用 */
    ev_set_userdata(diag_master->loop, diag_master);

    /* 监听IPC读事件 */
    ev_io_init(&diag_master->iread_watcher, diag_master_ev_io_om_ipc_read, diag_master->sockfd, EV_READ);
    ev_io_start(diag_master->loop, &diag_master->iread_watcher);

    /* 监听IPC写事件 */
    // ev_io_init(&diag_master->iwrite_watcher, diag_master_ipc_write, diag_master->sockfd, EV_WRITE);
    /* 启动心跳在线检测 */
    ev_timer_init(&diag_master->keepalive_watcher, diag_master_ev_timer_keepalive_handler, \
        diag_master->keepalive_interval * 0.001, diag_master->keepalive_interval * 0.001);
    ev_timer_start(diag_master->loop, &diag_master->keepalive_watcher);
#endif /* #ifdef __HAVE_LIBEV__ */
    /* 初始化部分默认诊断客户端，后续就不需要重复的创建销毁 */
    for (udsc_id = 0; udsc_id < DM_UDSC_CAPACITY_DEF; udsc_id++) {
        uds_client *udsc = dm_udsc_create();
        if (udsc) {            
            dm_udsc_ev_loop_set(udsc, diag_master_ev_loop(diag_master));
            udsc->id = udsc_id;
            diag_master->udscs[udsc->id] = udsc;
            diag_master->udsc_cnt++;
        }
    }

    return diag_master;
CREAT_FAILED:
    log_d("dm diag master create error \n");
    if (diag_master->sockfd > 0) {
#ifdef _WIN32
        closesocket(diag_master->sockfd);
#endif /* _WIN32 */
#ifdef __linux__
        close(diag_master->sockfd);
#endif /* __linux__ */
    }
    if (diag_master->loop) {
#ifdef __HAVE_LIBUV__
        uv_loop_delete(diag_master->loop);
#endif /* __HAVE_LIBUV__ */
#ifdef __HAVE_LIBEV__
        ev_loop_destroy(diag_master->loop);
#endif /* __HAVE_LIBEV__ */
    }
    if (diag_master) {
        DM_FREE(diag_master);
    }

    return NULL;
}

void diag_master_destory(diag_master_t *diag_master)
{
    uds_client *udsc = NULL;
    int udsc_index = 0;

    /* 停止活跃的UDS客户端并销毁 */
    for (udsc_index = 0; udsc_index < diag_master->udsc_cnt; udsc_index++) {
        udsc = diag_master->udscs[udsc_index];
        if (udsc) {
            dm_udsc_services_stop(udsc);
            dm_udsc_destory(udsc); /* 销毁释放UDS客户端 */
        }
    }
    
    if (diag_master->loop) {
        diag_master_ev_stop(diag_master);
#ifdef __HAVE_LIBUV__
        uv_loop_delete(diag_master->loop);
#endif /* __HAVE_LIBUV__ */
#ifdef __HAVE_LIBEV__
        ev_loop_destroy(diag_master->loop);
#endif /* __HAVE_LIBEV__ */
    }

    if (diag_master->sockfd > 0) {       
#ifdef _WIN32
        closesocket(diag_master->sockfd);
#endif /* _WIN32 */
#ifdef __linux__
        close(diag_master->sockfd);
#endif /* __linux__ */
#ifdef __linux__
        unlink(diag_master->dm_path);
        unlink(diag_master->dmapi_path);
#endif /* __linux__ */
    }
    
    DMS_LOCK;
    if (diag_master->dms && \
        diag_master->index < DMS_DIAG_MASTER_NUM_MAX) {
        /* 移除diag master在dms上的记录 */
        diag_master->dms->diag_master[diag_master->index] = 0;
    }
    DMS_UNLOCK;
    memset(diag_master, 0, sizeof(*diag_master));
    
    DM_FREE(diag_master);
}

#ifdef __HAVE_LIBUV__
uv_loop_t* diag_master_ev_loop(diag_master_t *diag_master)
{
    return diag_master->loop;
}
#endif /* __HAVE_LIBUV__  */

#ifdef __HAVE_LIBEV__
struct ev_loop *diag_master_ev_loop(diag_master_t *diag_master)
{
    return diag_master->loop;
}
#endif /* __HAVE_LIBEV__  */

#ifdef _WIN32
DWORD WINAPI diag_master_thread_run(LPVOID* arg)
#else /* _WIN32 */
void *diag_master_thread_run(void *arg)
#endif /* _WIN32 */
{
    diag_master_t *diag_master = (diag_master_t *)arg;
    if (diag_master == NULL) {
        log_d("Parameter error Thread exits \n");
        return 0;
    }
#ifdef _WIN32
#else /* _WIN32 */
    pthread_detach(pthread_self());
#endif /* _WIN32 */
    /* 开始事件循环，任意时刻都得存在事件，如果没有事件将导致线程退出 */
    log_d("diag master event loop start. \n");

#ifdef __HAVE_LIBEV__
    ev_run(diag_master_ev_loop(diag_master), 0);
#endif /* #ifdef __HAVE_LIBEV__ */
#ifdef __HAVE_LIBUV__
    uv_run(diag_master_ev_loop(diag_master), UV_RUN_DEFAULT);
#endif /* #ifdef __HAVE_LIBUV__ */
    /* 事件循环退出销毁diag master */
    diag_master_destory(diag_master);
    log_d("diag master event loop exit thread exits. \n");

    return 0;
}

#ifdef __HAVE_LIBUV__
static void diag_master_ev_io_dms_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) 
{
    DMS* dms = (DMS*)((char*)handle - offsetof(DMS, iread_watcher));
    buf->base = dms->rxbuf;
    buf->len = sizeof(dms->rxbuf);
}
static void diag_master_ev_io_dms_read(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags)
#endif /* #ifdef __HAVE_LIBUV__ */
#ifdef __HAVE_LIBEV__
static void diag_master_ev_io_dms_read(struct ev_loop *loop, ev_io *w, int e)
#endif /* #ifdef __HAVE_LIBEV__ */
{
#ifdef __HAVE_LIBUV__
    DMS *dms = (DMS*)((char*)handle - offsetof(DMS, iread_watcher));
#endif /* #ifdef __HAVE_LIBUV__ */
#ifdef __HAVE_LIBEV__
    DMS *dms = ev_userdata(loop);
#endif /* #ifdef __HAVE_LIBEV__ */
    INT32U dm_cmd = 0; INT32U recode = DM_CAMMAND_ERR_NO; INT16U id = 0;
    INT32U tv_sec = 0; INT32U tv_usec = 0;
#ifdef __linux__
    struct sockaddr_un un;
    int len = sizeof(struct sockaddr_un);
#endif /* __linux__  */
    int bytesRecv = -1;
    char dmapi_path[256] = {0};
    char dm_path[256] = {0};
    INT32U dm_index = 0;
    INT32U path_len = 0;
    INT32U keepalive_inter = 0;
#ifdef _WIN32
    HANDLE tidp = 0;
#endif /* _WIN32 */
#ifdef __linux__
    pthread_t tidp = 0;
#endif /* _WIN32 */
    diag_master_t *diag_master = NULL;

#ifdef __HAVE_LIBUV__
    bytesRecv = nread;
    log_hex_d("RECV", buf->base, bytesRecv);
#endif /* __HAVE_LIBUV__ */

#ifdef __HAVE_LIBEV__
#ifdef __linux__
        memset(dms->rxbuf, 0, sizeof(dms->rxbuf));
        bytesRecv = recvfrom(dms->sockfd, dms->rxbuf, sizeof(dms->rxbuf), 0, \
                                    (struct sockaddr*)&un, (socklen_t *)&len);
#endif /* __linux__ */
#ifdef _WIN32
        SOCKADDR_IN fromAddress;
        int fromLength = sizeof(fromAddress);
        bytesRecv = recvfrom(dms->sockfd, dms->rxbuf, sizeof(dms->rxbuf), 0, (SOCKADDR*)&fromAddress, &fromLength);
#endif /* _WIN32 */
#endif /* __HAVE_LIBEV__ */
    if (bytesRecv > 0) {     
        dm_common_decode(dms->rxbuf, sizeof(dms->rxbuf), &dm_cmd, &recode, &id, &tv_sec, &tv_usec);
        log_d("Request Command: 0x%02X(%s) Recode: %d udsc ID: %d tv_sec: %d tv_usec: %d \n", \
            dm_cmd, dm_ipc_command_str(dm_cmd), recode, id, tv_sec, tv_usec);
        if (dm_cmd == DM_CAMMAND_CONNECT_DIAG_MASTER_REQUEST) {
            keepalive_inter = *(INT32U *)(dms->rxbuf + OMREQ_CONNECT_KEEPALIVE_INTERVAL_OFFSET);            
            path_len = *(INT32U *)(dms->rxbuf + OMREQ_CONNECT_OMAPI_PATH_LEN_OFFSET);
            snprintf(dmapi_path, sizeof(dmapi_path), "%s", dms->rxbuf + OMREQ_CONNECT_OMAPI_PATH_OFFSET);
            log_d("keepalive_inter => %d path_len => %d dmapi_path => %s\n", keepalive_inter, path_len, dmapi_path);
#ifdef __linux__
            log_d("PATH1 %s \n", dmapi_path);            
            log_d("PATH2 %s \n", un.sun_path);
            if (access(dmapi_path, F_OK) != 0) {
                /* 无法获取diag master api的存在 */
                recode = DM_CAMMAND_ERR_OMAPI_UNKNOWN;
                goto REPLY;
            }
#endif /* __linux__ */
            /* 找到一个空闲的diag master位置 */            
            DMS_LOCK;
            for (dm_index = 0; dm_index < DMS_DIAG_MASTER_NUM_MAX; dm_index++) {
                if (dms->diag_master[dm_index] == NULL) {
                    break;
                }
            }            
            DMS_UNLOCK;
            /* 判断索引是否有效 */
            if (!(dm_index < DMS_DIAG_MASTER_NUM_MAX)) {
                recode = DM_CAMMAND_ERR_DIAG_MASTER_MAX;            
                goto REPLY;
            }
#ifdef __linux__           
            /* 创建新的diag master的unix socket path */
            snprintf(dm_path, sizeof(dm_path), DM_UNIX_SOCKET_PATH_PREFIX"%d", dm_index);
#endif /* __linux__ */
#ifdef _WIN32
            /* 生成一个diag master使用的UDP SOCKET用于和diag master api通信 */
            snprintf(dm_path, sizeof(dm_path), "%s:%d", DMS_UDS_SOCKET_PATH, \
                (unsigned short)(DMS_UDS_SOCKET_PORT_BASE + (dms->port_base++)));
#endif /* _WIN32 */
            /* 创建新的diag master */
            log_d("diag master address: %s diag master api address: %s \n", dm_path, dmapi_path);
            diag_master = diag_master_create(dm_path, dmapi_path);
            if (diag_master == NULL) {
                log_d("diag master create error \n");
                recode = DM_CAMMAND_ERR_DIAG_MASTER_CREATE;
                goto REPLY;
            }
            /* 创建diag master的事件循环线程,用于和diag master api进行IPC通信处理 */
#ifdef _WIN32
            if ((tidp = CreateThread(NULL, 0, diag_master_thread_run, diag_master, 0, NULL)) == NULL) {
#else /* _WIN32 */
            if (pthread_create(&tidp, 0, diag_master_thread_run, diag_master) != 0) {
#endif /* _WIN32 */
                log_d("dm diag master thread create failed. \n");  
                diag_master_destory(diag_master); /* 创建失败销毁释放diag master */              
                recode = DM_CAMMAND_ERR_DIAG_MASTER_CREATE;
                goto REPLY;
            }
            log_d("diag master Create success. \n");            
            DMS_LOCK;
            dms->diag_master[dm_index] = diag_master;            
            DMS_UNLOCK;
            diag_master->index = dm_index;
            diag_master->dms = dms;
            goto REPLY;
        }
    }
     
    return ;
REPLY:    
    memset(dms->txbuf, 0, sizeof(dms->txbuf));
    log_d("Reply Command: 0x%02X Recode: %d udsc ID: %d \n", DM_CAMMAND_CONNECT_DIAG_MASTER_REPLY, recode, id);   
    path_len = strlen(dm_path);
    memcpy(dms->txbuf + OMREP_CONNECT_DM_PATH_LEN_OFFSET, &path_len, OMREP_CONNECT_DM_PATH_LEN_SIZE);    
    memcpy(dms->txbuf + OMREP_CONNECT_DM_PATH_OFFSET, dm_path, path_len);
    dm_common_encode(dms->txbuf, sizeof(dms->txbuf), DM_CAMMAND_CONNECT_DIAG_MASTER_REPLY, recode, id, tv_sec, tv_usec);
#ifdef __HAVE_LIBEV__
#ifdef __linux__
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    memcpy(un.sun_path, dmapi_path, strlen(dmapi_path));    
    sendto(dms->sockfd, dms->txbuf, OMREQ_CONNECT_DM_MSG_MIN_SIZE + path_len, 0,\
                        (struct sockaddr *)&un, sizeof(struct sockaddr_un));
#endif /* __linux__ */
#ifdef _WIN32
    int bytes = -1;
    char* dmi_ip = NULL;
    char* dmi_port = NULL;
    char* context = NULL;
    SOCKADDR_IN remoteAddress;

    dmi_ip = strtok_s(dmapi_path, ":", &context);
    dmi_port = strtok_s(NULL, "", &context);
    if (dmi_ip == NULL || dmi_port == NULL) {
        log_e("diag master api address format error \n");
        return ;
    }
    struct in_addr apiaddr;
    if (inet_pton(AF_INET, dmi_ip, &apiaddr) <= 0) {
        log_e("inet_pton() error \n");
        return ;
    }
    remoteAddress.sin_family = AF_INET;
    remoteAddress.sin_port = htonl(atoi(dmi_port));
    remoteAddress.sin_addr.S_un.S_addr = apiaddr.s_addr ;
    bytes = sendto(dms->sockfd, dms->txbuf, OMREQ_CONNECT_DM_MSG_MIN_SIZE + path_len, 0, \
        (SOCKADDR*)&remoteAddress, sizeof(remoteAddress));

    return bytes;
#endif /* _WIN32 */
#endif /*__HAVE_LIBEV__*/
#ifdef __HAVE_LIBUV__
    uv_udp_send_t req;
    struct sockaddr_in sendaddr;
    char *dmi_ip = NULL;
    char *dmi_port = NULL;
    char *context = NULL;

    dmi_ip = strtok_s(dmapi_path, ":", &context);
    dmi_port = strtok_s(NULL, "", &context);
    if (dmi_ip == NULL || dmi_port == NULL) {
        log_e("diag master api address format error \n");
        return ;
    }

    uv_buf_t sendbuf = uv_buf_init(dms->txbuf, OMREQ_CONNECT_DM_MSG_MIN_SIZE + path_len);
    log_d("dms send ip4 addr => %s:%d \n", dmi_ip, atoi(dmi_port));
    uv_ip4_addr(dmi_ip, atoi(dmi_port), &sendaddr);
    uv_udp_send(&dms->udp_send, &dms->iwrite_watcher, &sendbuf, 1, (const struct sockaddr*)&sendaddr, NULL);
#endif /* __HAVE_LIBUV__ */

    return ;    
}

DMS *diag_master_dms_create()
{
    int ret = 0;
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        log_e("WSAStartup() error \n");
        return NULL;
    }
#endif /* _WIN32 */
#ifdef __linux__
    struct sockaddr_un un;
#endif /*__linux__ */
    DMS *dms = DM_MALLOC(sizeof(*dms));
    if (dms == NULL) {
        return NULL;
    }
    memset(dms, 0, sizeof(*dms));
#ifdef __HAVE_LIBEV__
        dms->sockfd = -1;
#ifdef __linux__
        memset(&un, 0, sizeof(struct sockaddr_un));
        un.sun_family = AF_UNIX;
        memcpy(un.sun_path, DMS_UNIX_SOCKET_PATH, sizeof(DMS_UNIX_SOCKET_PATH));
        dms->sockfd = diag_master_ipc_create(&un);
        if (dms->sockfd < 0) {
            log_d("fatal error diag_master_ipc_create failed \n");
            goto CREAT_FAILED;
        }
#endif /*__linux__ */
#ifdef _WIN32
        dms->sockfd = diag_master_ipc_create(DMS_UDS_SOCKET_PATH, DMS_UDS_SOCKET_PORT);
        if (dms->sockfd < 0) {
            log_d("fatal error diag_master_ipc_create failed \n");
            goto CREAT_FAILED;
        }
#endif /*_WIN32 */
    dms->loop = ev_loop_new(0);;
    /* 传入用户数据指针, 便于在各个回调函数内使用 */
    ev_set_userdata(dms->loop, dms);

    /* 监听IPC读事件 */
    ev_io_init(&dms->iread_watcher, diag_master_ev_io_dms_read, dms->sockfd, EV_READ);
    ev_io_start(dms->loop, &dms->iread_watcher);
#endif /* __HAVE_LIBEV__ */
#ifdef __HAVE_LIBUV__
    /* 创建UV事件循环主结构 */
    dms->loop = uv_loop_new();
    struct sockaddr_in udp_addr;

    log_d("dms recv ip4 addr => %s:%d \n", DMS_UDS_SOCKET_PATH, DMS_UDS_SOCKET_PORT);
    uv_ip4_addr(DMS_UDS_SOCKET_PATH, DMS_UDS_SOCKET_PORT, &udp_addr);
    /* 监听UDP读事件 */
    if ((ret = uv_udp_init(dms->loop, &dms->iread_watcher)) != 0) {
        log_e("uv_udp_init erro: %s \n", uv_strerror(ret));
        goto CREAT_FAILED;
    }
   
    if ((ret = uv_udp_bind(&dms->iread_watcher, (const struct sockaddr*)&udp_addr, UV_UDP_REUSEADDR)) != 0) {
        log_e("uv_udp_bind erro: %s \n", uv_strerror(ret));
        goto CREAT_FAILED;
    }
    if ((ret = uv_udp_recv_start(&dms->iread_watcher, diag_master_ev_io_dms_alloc, diag_master_ev_io_dms_read)) != 0) {
        log_e("uv_udp_recv_start erro: %s \n", uv_strerror(ret));
        goto CREAT_FAILED;
    }
    /* 处理UDP写事件 */
    if ((ret = uv_udp_init(dms->loop, &dms->iwrite_watcher)) != 0) {
        log_e("uv_udp_init erro: %s \n", uv_strerror(ret));
        goto CREAT_FAILED;
    }
    // uv_udp_bind(&dms->iwrite_watcher, (const struct sockaddr*)&udp_addr, UV_UDP_REUSEADDR);
#endif /* __HAVE_LIBUV__ */
    return dms;
CREAT_FAILED:
    log_d("dm diag master create error \n");
    if (dms->sockfd > 0) {
#ifdef _WIN32
        closesocket(dms->sockfd);
#endif /* _WIN32 */
#ifdef __linux__
        close(dms->sockfd);
#endif /* __linux__ */
    }
#ifdef __linux__ 
    unlink(DMS_UNIX_SOCKET_PATH);
#endif /* __linux__ */
    DM_FREE(dms);
#ifdef _WIN32
    WSACleanup();
#endif /* _WIN32 */

    return NULL;
}

#ifdef _WIN32
DWORD WINAPI diag_master_dms_thread_run(LPVOID* arg)
#else /* _WIN32 */
void *diag_master_dms_thread_run(void *arg)
#endif /* _WIN32 */
{
    DMS *dms = arg;
    if (dms == NULL) {
        log_d("Parameter error Thread exits \n");
        return 0;
    }
#ifdef __linux__
    pthread_detach(pthread_self());
#endif /* __linux__ */
    /* 开始事件循环，任意时刻都得存在事件，如果没有事件将导致线程退出 */
    log_d("diag master dms event loop start. \n");
#ifdef __HAVE_LIBEV__
    ev_run(dms->loop, 0);
#endif /* __HAVE_LIBEV__ */
#ifdef __HAVE_LIBUV__
    uv_run(dms->loop, UV_RUN_DEFAULT);
#endif /* __HAVE_LIBUV__ */
    /* 正常情况下事件循环是不会退出的，退出就说明不正常了 */
    log_e("diag master dms event loop exit thread exits. \n");
   
    return 0;
}

void diag_master_dms_start(void)
{
#ifdef _WIN32
    HANDLE tidp = 0;
#else /* _WIN32 */
    pthread_t tidp = 0;
#endif /* _WIN32 */
    /* DMS结构体用于管理接入的diag master api */
    DMS *dms = diag_master_dms_create();
    if (dms == NULL) {
        log_d("malloc failed. \n");
        return ;
    }
    /* 创建线程用于事件循环 */
#ifdef _WIN32
    if ((tidp = CreateThread(NULL, 0, diag_master_dms_thread_run, dms, 0, NULL)) == NULL) {
#else /* _WIN32 */
    if (pthread_create(&tidp, 0, diag_master_dms_thread_run, dms) != 0) {
#endif /* _WIN32 */
        log_d("dm diag master dms thread create success. \n");
    }
}

