#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/timeb.h>

#ifdef _WIN32
#include <winsock2.h>  
#include <ws2tcpip.h>
#endif /* _WIN32 */

#ifdef __linux__
#include <sys/select.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif /* __linux__ */

#include "dm_common.h"
#include "dm_api.h"

static int __dbg_info_enable__ = 1;

struct dm_api_udscs{
    unsigned int isvalid; /* 当前uds客户端的信息是否有效 */
    unsigned short udsc_id; /* uds客户端id */       
    uds_service_request_callback service_request; /* UDS客户端需要发送诊断请求的时候会回调这个函数 */
    void* rt_arg; /* uds_request_transfer回调函数注册时传入的用户数据指针，在触发回调函数时会传入该指针 */
    uds_all_service_finish_callback all_service_finish; /* 所有的诊断服务结束后会回调这个函数 */
    void* asf_arg; /* uds_all_service_finish_callback 回调函数注册时传入的用户数据指针，在触发回调函数时会传入该指针 */
    uds_service_sa_seed_callback sa_seed_callback;
    void* sa_seed_arg;
    dm_udsc_config config; /* uds客户端的基本配置信息 */
#define UDSC_REQUEST_RESULT_CALLBACK_MAX (1000)    
    struct {
        uds_service_result_callback service_result; /* 单个诊断服务结束后会回调这个函数 */
        void *rr_arg; /* uds_request_result 回调函数注册时传入的用户数据指针，在触发回调函数时会传入该指针 */
    } rr_handler[UDSC_REQUEST_RESULT_CALLBACK_MAX];
    unsigned int rr_cnt; /* rr_handler 总数 */
    unsigned int diag_req_id; /* 诊断请求ID */
    unsigned int diag_resp_id; /* 诊断应答ID */
};

#define DM_API_IPC_RXTX_BUFF_SIZE (10240)

typedef struct diag_master_api {
#ifdef __linux__
    int sockfd; /* 与diag_master通信使用 */
#endif /* __linux__ */
#ifdef _WIN32
    SOCKET sockfd;
#endif /* _WIN32 */
#define DIAG_MASTER_API_SOCKET_RECV_TIME_DEF (100) /* 100ms */    
    unsigned int recv_timeout; /* 接收等待时间 */
    struct dm_api_udscs udscs[DM_UDSC_CAPACITY_MAX];
    unsigned int udsc_cnt;
    unsigned char txbuf[DM_API_IPC_RXTX_BUFF_SIZE]; /* socket 发送buff */
    unsigned char rxbuf[DM_API_IPC_RXTX_BUFF_SIZE]; /* socket 接收buff */
#ifdef __linux__
    char dm_path[256]; /* OTA MASTER unix socket 路径 */
    char dmapi_path[256]; /* OTA MASTER API unix socket 路径 */
#endif /* __linux__ */
#ifdef _WIN32
    char ip[64];
    unsigned short port;
    int dm_ip;
    unsigned short dm_port;
#endif /* _WIN32 */
} diag_master_api;
static int dm_api_ipc_create(struct sockaddr_un *un);
static int dm_api_request_handler(diag_master_api *dm_api);
static int dm_api_recv_reply_handler(diag_master_api *dm_api, unsigned short *id, unsigned int expect_reply, unsigned int expect_code);
static struct dm_api_udscs *dm_api_udsc_get(diag_master_api *dm_api, unsigned short udscid);
static int dm_api_udsc_reset(diag_master_api *dm_api, unsigned short udscid);
static void dm_api_cammand_service_request_handler(diag_master_api *dm_api, unsigned int recode, unsigned short udscid);
static void dm_api_cammand_services_finish_handler(diag_master_api *dm_api, unsigned int recode, unsigned short udscid);
static void dm_api_cammand_service_result_handler(diag_master_api *dm_api, unsigned int recode, unsigned short udscid);
static void dm_api_cammand_service_sa_seed_handler(diag_master_api *dm_api, unsigned int recode, unsigned short udscid);
static void dm_api_cammand_keepalive_request_handler(diag_master_api *dm_api, unsigned int recode, unsigned short udscid);

static int dm_api_sendto_diag_master(diag_master_api *dm_api, unsigned char *buff, unsigned int size)
{
#ifdef __linux__
    struct sockaddr_un un;
    ssize_t bytesSent = -1;
    
    if (dm_api->sockfd < 0) return -1;
    
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    snprintf(un.sun_path, sizeof(un.sun_path), dm_api->dm_path, strlen(dm_api->dm_path));    
    bytesSent = sendto(dm_api->sockfd, buff, size, 0,\
                        (struct sockaddr *)&un, sizeof(struct sockaddr_un));
    // log_hex_d("send", buff, bytesSent);
    return bytesSent;
#endif /* __linux__ */
#ifdef _WIN32
    int bytes = -1;

    SOCKADDR_IN remoteAddress;
    remoteAddress.sin_family = AF_INET;
    remoteAddress.sin_port = htons(dm_api->dm_port);
    remoteAddress.sin_addr.S_un.S_addr = htonl(dm_api->dm_ip);
    bytes = sendto(dm_api->sockfd, buff, size, 0, \
        (SOCKADDR*)&remoteAddress, sizeof(remoteAddress));
    return bytes;
#endif /* _WIN32 */
}
/*
    diag master api创建和diag master通信的SOCKET
*/
#ifdef _WIN32
static SOCKET dm_api_ipc_create(char *ip, short port)
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
    log_d("OTA master api udpSocket = %d \n", udpSocket);
    
    return udpSocket;
}
#endif /* _WIN32 */

#ifdef __linux__
static int dm_api_ipc_create(struct sockaddr_un *un)
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
#endif /* #ifdef __linux_ */

static int dm_api_request_handler(diag_master_api *dm_api)
{
    unsigned int reply_cmd = 0;
    unsigned int recode = 0;
    unsigned int tv_sec = 0; unsigned int tv_usec = 0;
    unsigned short udscid = 0;
    struct dm_api_udscs *dm_udsc = 0;

    dm_common_decode(dm_api->rxbuf, sizeof(dm_api->rxbuf), &reply_cmd, &recode, &udscid, &tv_sec, &tv_usec);
    if (reply_cmd != DM_CAMMAND_OMAPI_KEEPALIVE_REQUEST) {
        log_d("Recv command: 0x%02X(%s) Recode: %d(%s) udsc ID: %d tv_sec: %d tv_usec: %d \n", \
            reply_cmd, dm_ipc_command_str(reply_cmd), recode, dm_command_rcode_str(recode), udscid, tv_sec, tv_usec);
    }
    if (DM_CMD_REPLY_MASK & reply_cmd) {
        /* 只处理请求消息，应答消息直接忽略 */
        return -1;
    }
    if (reply_cmd != DM_CAMMAND_OMAPI_KEEPALIVE_REQUEST) {
        dm_udsc = dm_api_udsc_get(dm_api, udscid);
        if (dm_udsc == 0) {
            log_e("The udsc id is invalid => %d \n", udscid);
            return -2;
        }
        /* 判断IPC命令有效性 */
        if (dm_udsc->config.cmd_valid_time > 0 && \
            dm_tv_currtdms(tv_sec, tv_usec) > dm_udsc->config.cmd_valid_time) {
            log_w("When the command expires Ignore the command. \n");
            return -3;    
        }
    }
    switch (reply_cmd) {
        case DM_CAMMAND_SERVICE_INDICATION_REQUEST:
            dm_api_cammand_service_request_handler(dm_api, recode, udscid);
            break;
        case DM_CAMMAND_SCRIPT_RESULT_REQUEST:                
            dm_api_cammand_services_finish_handler(dm_api, recode, udscid);
            break;
        case DM_CAMMAND_SERVICE_REQUEST_RESULT_REQUEST:
            dm_api_cammand_service_result_handler(dm_api, recode, udscid);
            break;
        case DM_CAMMAND_SERVICE_SA_SEED_REQUEST:
            dm_api_cammand_service_sa_seed_handler(dm_api, recode, udscid);
            break;
        case DM_CAMMAND_OMAPI_KEEPALIVE_REQUEST:
            dm_api_cammand_keepalive_request_handler(dm_api, recode, udscid);
            break;
        default:
            log_w("OTA Master API IPC unknown cammand => %d \n", reply_cmd);    
            break;
    }

    return 0;
}

/* 接收来自OTA MASTER的预期响应，接收到预期值返回0，其他值非预期 */
static int dm_api_recv_reply_handler(diag_master_api *dm_api, unsigned short *id, unsigned int expect_reply, unsigned int expect_code)
{
    unsigned int reply_cmd = 0;
    unsigned int recode = 0;
    unsigned int tv_sec = 0; unsigned int tv_usec = 0;
    unsigned short udscid = 0;

    if (!(expect_reply & DM_CMD_REPLY_MASK)) {
        return -1;
    }

    /* 这里有一个接收超时时间就不判断命令有效时间了 */
    for ( ;dm_recvfrom(dm_api->sockfd, dm_api->rxbuf, sizeof(dm_api->rxbuf), dm_api->recv_timeout) > 0; ) {
        dm_common_decode(dm_api->rxbuf, sizeof(dm_api->rxbuf), &reply_cmd, &recode, &udscid, &tv_sec, &tv_usec);
        log_d("Recv command: %s Recode: %d(%s) udsc ID: %d \n", \
            dm_ipc_command_str(reply_cmd), recode, dm_command_rcode_str(recode), udscid);
        if (reply_cmd & DM_CMD_REPLY_MASK) {            
            /* 如果是响应消息 */
            if (reply_cmd == expect_reply && recode == expect_code) {
                /* 响应符合预期 */
                if (id) {
                    *id = udscid;
                }
                return 0;
            }
            else {
                log_w("expect reply: %d expect code: %d reply: %d recode: %d\n", expect_reply, expect_code, reply_cmd, recode);
                if (reply_cmd != expect_reply) {
                    continue;
                }
                /* 响应不及预期 */
                return -2;
            }
        }
        else {
            /* 这里防止遗漏diagmaster发起的请求消息 */
            if (dm_api_request_handler(dm_api) != 0) {
                return -3;
            }
        }
    }

    return -4;
}

/*
    获取diag master api端保存的UDS客户端信息
*/
static struct dm_api_udscs *dm_api_udsc_get(diag_master_api *dm_api, unsigned short udscid)
{
    /* UDS客户端ID不能为无效值，也不能未被diag master api记录 */
    if (!(udscid < DM_UDSC_CAPACITY_MAX) || \
        dm_api->udscs[udscid].isvalid == 0) {
        return 0;
    }
        
    return &dm_api->udscs[udscid];
}


int dm_api_connect_diag_master(diag_master_api *dm_api)
{
    unsigned int cmd = DM_CAMMAND_CONNECT_DIAG_MASTER_REQUEST;
    unsigned int recode = DM_CAMMAND_ERR_NO;
    struct dm_api_udscs *dm_udsc = 0;
    unsigned int tv_sec = 0; unsigned int tv_usec = 0; unsigned short id = 0;
#ifdef __linux__
    struct sockaddr_un un;
    int len = sizeof(struct sockaddr_un);
    ssize_t bytes = -1;
#endif /* __linux__ */
    char dm_path[256] = {0};
#ifdef _WIN32
    char om_addr_info[256] = { 0 };
#endif /* _WIN32 */
    unsigned int path_len = 0;
    unsigned int keepalive_inter = 1000;
    
    assert(dm_api);   
#ifdef __linux__
    path_len = strlen(dm_api->dmapi_path);
#endif /* __linux__ */
#ifdef _WIN32
    snprintf(om_addr_info, sizeof(om_addr_info), "%s:%d", dm_api->ip, dm_api->port);
    path_len = strlen(om_addr_info);
#endif /* _WIN32 */
    memcpy(dm_api->txbuf + OMREQ_CONNECT_KEEPALIVE_INTERVAL_OFFSET, &keepalive_inter, OMREQ_CONNECT_KEEPALIVE_INTERVAL_SIZE);
    memcpy(dm_api->txbuf + OMREQ_CONNECT_OMAPI_PATH_LEN_OFFSET, &path_len, OMREQ_CONNECT_OMAPI_PATH_LEN_SIZE);
#ifdef __linux__
    memcpy(dm_api->txbuf + OMREQ_CONNECT_OMAPI_PATH_OFFSET, dm_api->dmapi_path, path_len);
#endif /* __linux__ */
#ifdef _WIN32
    memcpy(dm_api->txbuf + OMREQ_CONNECT_OMAPI_PATH_OFFSET, om_addr_info, path_len);
#endif /* _WIN32 */
    dm_common_encode(dm_api->txbuf, sizeof(dm_api->txbuf), cmd, recode, 0, tv_sec, tv_usec);
#ifdef __linux__
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    memcpy(un.sun_path, DMS_UNIX_SOCKET_PATH, sizeof(DMS_UNIX_SOCKET_PATH));    
    bytes = sendto(dm_api->sockfd, dm_api->txbuf, OMREQ_CONNECT_OMAPI_MSG_MIN_SIZE + path_len, 0,\
                        (struct sockaddr *)&un, sizeof(struct sockaddr_un));
#endif /* __linux__ */
#ifdef _WIN32
    int bytes = -1;
    struct in_addr addr;
    if (inet_pton(AF_INET, DMS_UDS_SOCKET_PATH, &addr) <= 0) {
        return -1;
    }
    SOCKADDR_IN remoteAddress;
    remoteAddress.sin_family = AF_INET;
    remoteAddress.sin_port = htons(DMS_UDS_SOCKET_PORT);
    remoteAddress.sin_addr.S_un.S_addr = addr.s_addr;
    bytes = sendto(dm_api->sockfd, dm_api->txbuf, OMREQ_CONNECT_OMAPI_MSG_MIN_SIZE + path_len, 0, \
                        (SOCKADDR*)&remoteAddress, sizeof(remoteAddress));
#endif /* _WIN32 */
    if (bytes == (OMREQ_CONNECT_OMAPI_MSG_MIN_SIZE + path_len)) {        
        fd_set readfds;
        struct timeval timeout;
        int sret = 0;
        
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000 * 100; /* 100ms */
        FD_ZERO(&readfds);
        FD_SET(dm_api->sockfd, &readfds);
        if ((sret = select(dm_api->sockfd + 1, &readfds, NULL, NULL, &timeout)) > 0 &&\
            FD_ISSET(dm_api->sockfd, &readfds)) {
#ifdef __linux__
            bytes = recvfrom(dm_api->sockfd, dm_api->rxbuf, sizeof(dm_api->rxbuf), 0, \
                                    (struct sockaddr*)&un, (socklen_t *)&len);
#endif /* __linux__ */
#ifdef _WIN32
            SOCKADDR_IN fromAddress;
            int fromLength = sizeof(fromAddress);
            int bytes = recvfrom(dm_api->sockfd, dm_api->rxbuf, sizeof(dm_api->rxbuf), 0, (SOCKADDR*)&fromAddress, &fromLength);
#endif /* _WIN32 */
            if (bytes > 0) {     
                dm_common_decode(dm_api->rxbuf, sizeof(dm_api->rxbuf), &cmd, &recode, &id, &tv_sec, &tv_usec);
                log_d("Request Command: 0x%02X(%s) Recode: %d(%s) udsc ID: %d tv_sec: %d tv_usec: %d \n", \
                    cmd, dm_ipc_command_str(cmd), recode, dm_command_rcode_str(recode), id, tv_sec, tv_usec);
                if (cmd == DM_CAMMAND_CONNECT_DIAG_MASTER_REPLY && recode == DM_CAMMAND_ERR_NO) {
                    unsigned int path_len = *(unsigned int *)(dm_api->rxbuf + OMREP_CONNECT_DM_PATH_LEN_OFFSET);
                    snprintf(dm_path, sizeof(dm_path), "%s", dm_api->rxbuf + OMREP_CONNECT_DM_PATH_OFFSET);
                    log_d("diag master path => %s \n", dm_path);
#ifdef __linux__
                    if (access(dm_path, F_OK) != 0) {
                        /* 无法获取diag master的存在 */
                        log_e("Not found diag master unix socket path (%s) \n", dm_path);
                        return -1;
                    }
                    snprintf(dm_api->dm_path, sizeof(dm_api->dm_path), "%s", dm_path);
#endif /* __linux__ */
#ifdef _WIN32
                    char* dm_ip = NULL;
                    char* dm_port = NULL;
                    char* context = NULL;
                    dm_ip = strtok_s(dm_path, ":", &context);
                    dm_port = strtok_s(NULL, "", &context);
                    if (dm_ip == NULL || dm_port == NULL) {
                        log_e("diag master address format error \n");
                        return -2;
                    }

                    struct in_addr addr;
                    if (inet_pton(AF_INET, dm_ip, &addr) <= 0) {
                        log_e("inet_pton() error \n");
                        return -3;
                    }
                    dm_api->dm_ip = ntohl(addr.s_addr);
                    dm_api->dm_port = atoi(dm_port);
#endif /* _WIN32 */
                    return 0;
                }
            }
        } 
    }

    return -4;      
}

/* 
    diag master api创建
*/
diag_master_api *dm_api_create()
{
    int udsc_ii = 0;
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        log_e("WSAStartup() error \n");
        return NULL;
    }
#endif /* _WIN32 */

#ifdef __linux__
    struct sockaddr_un un;
    char dmapi_path[256] = {0};

    /* 生成unix socket 路径     */
    snprintf(dmapi_path, sizeof(dmapi_path), DM_API_UNIX_SOCKET_PATH_PREFIX"%d_%d", \
        getpid() % 0xffffffff, rand() % 0xffffffff);
    if (access(dmapi_path, F_OK) == 0) {
        log_e("fatal error (%s) already exist. \n", dmapi_path);
        return 0;
    }
#endif /* #ifdef __linux__ */
    diag_master_api *dm_api = malloc(sizeof(*dm_api));
    if (dm_api == 0) {
        log_e("fatal error malloc failed. \n");
        return 0;
    }
    memset(dm_api, 0, sizeof(*dm_api));
    dm_api->sockfd = -1;
    /* API同步接口的超时时间 */
    dm_api->recv_timeout = DIAG_MASTER_API_SOCKET_RECV_TIME_DEF;
#ifdef _WIN32
    snprintf(dm_api->ip, sizeof(dm_api->ip), "%s", DM_UDS_SOCKET_PATH);
    dm_api->port = DM_UDS_SOCKET_PORT_BASE;
    srand(time(0));
    dm_api->port = rand() % 1000 + DM_UDS_SOCKET_PORT_BASE; /* 随机生成一个端口号 */
    dm_api->sockfd = dm_api_ipc_create(dm_api->ip, dm_api->port);
    if (!(dm_api->sockfd > 0)) {
        log_e("fatal error dm_api_ipc_create failed \n");
        goto CREAT_FAILED;
    }
#endif /* _WIN32  */
#ifdef __linux__   
    memset(&un, 0, sizeof(struct sockaddr_un));
    un.sun_family = AF_UNIX;
    memcpy(un.sun_path, dmapi_path, strlen(dmapi_path));

    /* 创建域套接字用于和OTA MASTER通信 */
    dm_api->sockfd = dm_api_ipc_create(&un);
    if (dm_api->sockfd < 0) {
        log_e("fatal error dm_api_ipc_create failed \n");
        goto CREAT_FAILED;
    }
    snprintf(dm_api->dmapi_path, sizeof(dm_api->dmapi_path), "%s", dmapi_path);
    log_d("OM API Unix path => %s \n", dm_api->dmapi_path);
#endif /* #ifdef __linux__ */
    /* 连接OTAM ASTER */
    if (dm_api_connect_diag_master(dm_api) < 0) {
        log_e("fatal error connect diag master failed. \n");
        goto CREAT_FAILED;
    }

    /* OTA MASTER API这边的UDS客户端信息初始化 */
    dm_api->udsc_cnt = 0;
    for (udsc_ii = 0; udsc_ii < DM_UDSC_CAPACITY_MAX; udsc_ii++) {
        dm_api->udscs[udsc_ii].isvalid = 0;
        dm_api->udscs[udsc_ii].udsc_id = udsc_ii;          
    }
    memset(dm_api->txbuf, 0, sizeof(dm_api->txbuf));
    memset(dm_api->rxbuf, 0, sizeof(dm_api->rxbuf));

    return dm_api;
CREAT_FAILED:    
    log_e("dm OTA master create error \n");
    if (dm_api->sockfd > 0) {
#ifdef _WIN32
        closesocket(dm_api->sockfd);
#endif /* _WIN32 */
#ifdef __linux__
        close(dm_api->sockfd);
#endif /* __linux__ */
    }
#ifdef __linux__ 
    unlink(dmapi_path);
#endif /* __linux__ */
    free(dm_api);
#ifdef _WIN32
    WSACleanup();
#endif /* _WIN32 */

    return 0;
}

void dm_api_destory(diag_master_api *dm_api)
{
    assert(dm_api);

    if (dm_api->sockfd > 0) {
#ifdef _WIN32
        closesocket(dm_api->sockfd);
#endif /* _WIN32 */
#ifdef __linux__
        close(dm_api->sockfd);
#endif /* __linux__ */
    }
#ifdef __linux__ 
    unlink(dm_api->dmapi_path);
#endif /* __linux__ */
    memset(dm_api, 0, sizeof(*dm_api));
    free(dm_api);
#ifdef _WIN32
    WSACleanup();
#endif /* _WIN32 */
}

int dm_api_sockfd(diag_master_api *oa_api)
{
    assert(oa_api);
    return oa_api->sockfd;
}

void dm_debug_enable(int eb)
{
    __dbg_info_enable__ = eb;
}

/*
    重置diag master api端的UDS客户端信息
*/
static int dm_api_udsc_reset(diag_master_api *dm_api, unsigned short udscid)
{
    if (udscid < DM_UDSC_CAPACITY_MAX) {
        struct dm_api_udscs *dm_udsc = &dm_api->udscs[udscid];
        if (dm_udsc->isvalid) {
            dm_api->udsc_cnt--;
        }
        dm_udsc->isvalid = 0;             
        memset(&dm_udsc->config, 0, sizeof(dm_udsc->config));
        memset(&dm_udsc->rr_handler, 0, sizeof(dm_udsc->rr_handler));
        dm_udsc->rr_cnt = 0;
    }
}

/* 创建uds客户端 */
int dm_api_udsc_create(diag_master_api *dm_api, dm_udsc_config *config)
{
    unsigned int cmd = DM_CAMMAND_UDSC_CREATE_REQUEST;
    unsigned int recode = DM_CAMMAND_ERR_NO;
    unsigned short udscid = 0;
    unsigned int tv_sec = 0; unsigned int tv_usec = 0;
    dm_udsc_config def_config;

    assert(dm_api);    
    if (config == 0) {
        memset(&def_config, 0, sizeof(def_config));
        config = &def_config;
    }
   
    /* 请求数据编码 */
    dm_common_encode(dm_api->txbuf, sizeof(dm_api->txbuf), cmd, recode, udscid, tv_sec, tv_usec);  
    memcpy(dm_api->txbuf + DM_IPC_UDSC_CREATE_CMD_VALID_TIME_OFFSET, &config->cmd_valid_time, DM_IPC_UDSC_CREATE_CMD_VALID_TIME_SIZE);
    if (dm_api_sendto_diag_master(dm_api, dm_api->txbuf, DM_IPC_COMMON_MSG_SIZE + DM_IPC_UDSC_CREATE_SIZE) == \
        DM_IPC_COMMON_MSG_SIZE + DM_IPC_UDSC_CREATE_SIZE) {
        if (dm_api_recv_reply_handler(dm_api, &udscid, DM_CAMMAND_UDSC_CREATE_REPLY, DM_CAMMAND_ERR_NO) == 0) {
            if (udscid < DM_UDSC_CAPACITY_MAX) {
                dm_api_udsc_reset(dm_api, udscid);
                struct dm_api_udscs *dm_udsc = &dm_api->udscs[udscid];
                dm_udsc->isvalid = 1;
                dm_udsc->udsc_id = udscid;                       
                memcpy(&dm_udsc->config, config, sizeof(*config));
                dm_api->udsc_cnt++;                  
                goto CREATE_SUCCESS;
            }
        } 
    }            
    log_e("UDS client created failed \n");   
    return -1;
CREATE_SUCCESS:
    log_d("UDS client[%d] created successfully \n", udscid);
    return udscid;
}

/*
    销毁diag master端创建的UDS客户端
*/
int dm_api_udsc_destory(diag_master_api *dm_api, unsigned short udscid)
{
    unsigned int cmd = DM_CAMMAND_UDSC_DESTORY_REQUEST;
    unsigned int recode = DM_CAMMAND_ERR_NO;
    struct dm_api_udscs *dm_udsc = 0;
    unsigned int tv_sec = 0; unsigned int tv_usec = 0;
    
    assert(dm_api);    

    dm_udsc = dm_api_udsc_get(dm_api, udscid);
    if (dm_udsc == 0) {
        log_e("The udsc id is invalid => %d \n", udscid);
        return -1;
    }
    /* 移除OTA MASTER API端的uds客户端信息 */                
    dm_api_udsc_reset(dm_api, udscid);

    /* 移除OTA MASTER 端的uds客户端信息 */
    dm_common_encode(dm_api->txbuf, sizeof(dm_api->txbuf), cmd, recode, udscid, tv_sec, tv_usec);
    if (dm_api_sendto_diag_master(dm_api, dm_api->txbuf, DM_IPC_COMMON_MSG_SIZE + 0) == \
        DM_IPC_COMMON_MSG_SIZE + 0) {    
        if (dm_api_recv_reply_handler(dm_api, &udscid, \
                DM_CAMMAND_UDSC_DESTORY_REPLY, DM_CAMMAND_ERR_NO) == 0) {
            return 0;
        }
    }
        
    return -2;      
}

int dm_api_doipc_create(diag_master_api *dm_api, unsigned short udscid, doipc_config_t *config)
{
    unsigned int cmd = DM_CAMMAND_DOIPC_CREATE_REQUEST;
    unsigned int recode = DM_CAMMAND_ERR_NO;
    struct dm_api_udscs *dm_udsc = 0;
    unsigned int tv_sec = 0; unsigned int tv_usec = 0;
    
    assert(dm_api);   
    
    dm_udsc = dm_api_udsc_get(dm_api, udscid);
    if (dm_udsc == 0) {
        log_e("The udsc id is invalid => %d \n", udscid);
        return -1;
    }

    memcpy(dm_api->txbuf + DOIPC_CREATE_VERSION_OFFSET, &config->ver, DOIPC_CREATE_VERSION_SIZE);
    memcpy(dm_api->txbuf + DOIPC_CREATE_SOURCE_ADDR_OFFSET, &config->sa_addr, DOIPC_CREATE_SOURCE_ADDR_SIZE);
    memcpy(dm_api->txbuf + DOIPC_CREATE_CLIENT_IP_OFFSET, &config->src_ip, DOIPC_CREATE_CLIENT_IP_SIZE);
    memcpy(dm_api->txbuf + DOIPC_CREATE_CLIENT_PORT_OFFSET, &config->src_port, DOIPC_CREATE_CLIENT_PORT_SIZE);
    memcpy(dm_api->txbuf + DOIPC_CREATE_SERVER_IP_OFFSET, &config->dst_ip, DOIPC_CREATE_SERVER_IP_SIZE);
    memcpy(dm_api->txbuf + DOIPC_CREATE_SERVER_PORT_OFFSET, &config->dst_port, DOIPC_CREATE_SERVER_PORT_SIZE);
    dm_common_encode(dm_api->txbuf, sizeof(dm_api->txbuf), cmd, recode, udscid, tv_sec, tv_usec);
    if (dm_api_sendto_diag_master(dm_api, dm_api->txbuf, DOIPC_CREATE_REQUEST_SIZE) == \
        DOIPC_CREATE_REQUEST_SIZE) {    
        if (dm_api_recv_reply_handler(dm_api, &udscid, \
                DM_CAMMAND_DOIPC_CREATE_REPLY, DM_CAMMAND_ERR_NO) == 0) {
            return 0;
        }
    }
        
    return -2;
}

/*  请求OTA MASTER启动UDS 客户端开始执行诊断任务 */
int dm_api_udsc_start(diag_master_api *dm_api, unsigned short udscid)
{
    unsigned int cmd = DM_CAMMAND_START_SCRIPT_REQUEST;
    unsigned int recode = DM_CAMMAND_ERR_NO;
    struct dm_api_udscs *dm_udsc = 0;
    unsigned int tv_sec = 0; unsigned int tv_usec = 0;
    
    assert(dm_api);   
    
    dm_udsc = dm_api_udsc_get(dm_api, udscid);
    if (dm_udsc == 0) {
        log_e("The udsc id is invalid => %d \n", udscid);
        return -1;
    }

    dm_common_encode(dm_api->txbuf, sizeof(dm_api->txbuf), cmd, recode, udscid, tv_sec, tv_usec);
    if (dm_api_sendto_diag_master(dm_api, dm_api->txbuf, DM_IPC_COMMON_MSG_SIZE + 0) == \
        DM_IPC_COMMON_MSG_SIZE + 0) {    
        if (dm_api_recv_reply_handler(dm_api, &udscid, \
                DM_CAMMAND_START_SCRIPT_REPLY, DM_CAMMAND_ERR_NO) == 0) {
            return 0;
        }
    }
        
    return -2;      
}

/*  请求OTA MASTER中止UDS 客户端执行诊断任务 */
int dm_api_udsc_stop(diag_master_api *dm_api, unsigned short udscid)
{
    unsigned int cmd = DM_CAMMAND_STOP_SCRIPT_REQUEST;
    unsigned int recode = DM_CAMMAND_ERR_NO;
    struct dm_api_udscs *dm_udsc = 0;
    unsigned int tv_sec = 0; unsigned int tv_usec = 0;
    
    assert(dm_api);
    dm_udsc = dm_api_udsc_get(dm_api, udscid);
    if (dm_udsc == 0) {
        log_e("The udsc id is invalid => %d \n", udscid);
        return -1;
    }

    dm_common_encode(dm_api->txbuf, sizeof(dm_api->txbuf), cmd, recode, udscid, tv_sec, tv_usec);
    if (dm_api_sendto_diag_master(dm_api, dm_api->txbuf, DM_IPC_COMMON_MSG_SIZE + 0) == \
        DM_IPC_COMMON_MSG_SIZE + 0) {    
        if (dm_api_recv_reply_handler(dm_api, &udscid, \
                DM_CAMMAND_STOP_SCRIPT_REPLY, DM_CAMMAND_ERR_NO) == 0) {
            return 0;
        }
    }

    return -2;
}

/*
    请求diag master重置，将导致所有的UDS任务停止并且销毁所有的UDS客户端
*/
int dm_api_master_reset(diag_master_api *dm_api)
{
    unsigned int cmd = DM_CAMMAND_DIAG_MASTER_RESET_REQUEST;
    unsigned int recode = DM_CAMMAND_ERR_NO;
    unsigned short udscid = 0;
    unsigned int tv_sec = 0; unsigned int tv_usec = 0;
    
    assert(dm_api);   
    dm_common_encode(dm_api->txbuf, sizeof(dm_api->txbuf), cmd, recode, 0, tv_sec, tv_usec);
    if (dm_api_sendto_diag_master(dm_api, dm_api->txbuf, DM_IPC_COMMON_MSG_SIZE + 0) == \
        DM_IPC_COMMON_MSG_SIZE + 0) {    
        if (dm_api_recv_reply_handler(dm_api, &udscid, \
                DM_CAMMAND_DIAG_MASTER_RESET_REPLY, DM_CAMMAND_ERR_NO) == 0) {
            return 0;
        }
    }
        
    return -1;      
}

/*
    在指定的UDS客户端上配置增加一个诊断服务项
*/
int dm_api_udsc_service_config(diag_master_api *dm_api, unsigned short udscid, service_config *config)
{
    unsigned int cmd = DM_CAMMAND_CONFIG_SCRIPT_REQUEST;
    unsigned int recode = DM_CAMMAND_ERR_NO;
    struct dm_api_udscs *dm_udsc = 0;
    unsigned int tv_sec = 0; unsigned int tv_usec = 0;
    unsigned char *byte_addr = 0;
    int clen = 0;

    assert(dm_api);   
    assert(config);   
    
    dm_udsc = dm_api_udsc_get(dm_api, udscid);
    if (dm_udsc == 0) {
        log_e("The udsc id is invalid => %d \n", udscid);
        return -1;
    }
    byte_addr = dm_api->txbuf + DM_IPC_COMMON_MSG_SIZE;
    clen = dm_service_config_encode(byte_addr, sizeof(dm_api->txbuf) - DM_IPC_COMMON_MSG_SIZE, config);
    /* 请求数据编码 */
    dm_common_encode(dm_api->txbuf, sizeof(dm_api->txbuf), cmd, recode, udscid, tv_sec, tv_usec);
    if (dm_api_sendto_diag_master(dm_api, dm_api->txbuf, DM_IPC_COMMON_MSG_SIZE + clen) == \
        DM_IPC_COMMON_MSG_SIZE + clen) {    
        if (dm_api_recv_reply_handler(dm_api, &udscid, \
                DM_CAMMAND_CONFIG_SCRIPT_REPLY, DM_CAMMAND_ERR_NO) == 0) {
            return 0;
        }
    }
        
    return -2;      
}

int dm_api_udsc_sa_key(diag_master_api *dm_api, unsigned short udscid, unsigned char level, const unsigned char *key, unsigned int key_size)
{
    unsigned int cmd = DM_CAMMAND_SERVICE_SA_KEY_REQUEST;
    unsigned int recode = DM_CAMMAND_ERR_NO;
    unsigned int tv_sec = 0; unsigned int tv_usec = 0;
    
    assert(dm_api);
    /* encode 1 byte level */
    memcpy(dm_api->txbuf + SERVICE_SA_KEY_LEVEL_OFFSET, &level, SERVICE_SA_KEY_LEVEL_SIZE);    
    /* encode 2 byte key长度 */
    memcpy(dm_api->txbuf + SERVICE_SA_KEY_SIZE_OFFSET, &key_size, SERVICE_SA_KEY_SIZE_SIZE);    
    /* encode n byte key数据 */
    memcpy(dm_api->txbuf + SERVICE_SA_KEY_OFFSET, key, key_size);    
    log_hex_d("UDS Request key: ", key, key_size);    
    /* encode 通用的头部固定数据 */
    dm_common_encode(dm_api->txbuf, sizeof(dm_api->txbuf), cmd, recode, udscid, tv_sec, tv_usec);
    if (dm_api_sendto_diag_master(dm_api, dm_api->txbuf, SERVICE_SA_KEY_OFFSET + key_size) == \
        SERVICE_SA_KEY_OFFSET + key_size) {    
        if (dm_api_recv_reply_handler(dm_api, &udscid, \
                DM_CAMMAND_SERVICE_SA_KEY_REPLY, DM_CAMMAND_ERR_NO) == 0) {
            return 0;
        }
    }
        
    return -1;      
}

/*
    设置UDS诊断请求处理回调函数
*/
int dm_api_udsc_service_result_callback_set(diag_master_api *dm_api, unsigned short udscid, uds_service_result_callback call, void *arg)
{
    struct dm_api_udscs *dm_udsc = 0;

    assert(dm_api);   
    if (call == 0) {
        return -1;
    }

    dm_udsc = dm_api_udsc_get(dm_api, udscid);
    if (dm_udsc == 0) {
        log_e("The udsc id is invalid => %d \n", udscid);
        return -2;
    }

    if (!(dm_udsc->rr_cnt < UDSC_REQUEST_RESULT_CALLBACK_MAX)) {
        return -3;
    }
    dm_udsc->rr_handler[dm_udsc->rr_cnt].service_result = call;
    dm_udsc->rr_handler[dm_udsc->rr_cnt].rr_arg = arg;
    dm_udsc->rr_cnt++;
    
    return dm_udsc->rr_cnt;
}

/*
    UDS客户端的通用配置
*/
int dm_api_udsc_general_config(diag_master_api *dm_api, unsigned short udscid, udsc_general_config *config)
{
    unsigned int cmd = DM_CAMMAND_OTA_GENERAL_CONFIG_REQUEST;
    unsigned int recode = DM_CAMMAND_ERR_NO;
    struct dm_api_udscs *dm_udsc = 0;
    unsigned int tv_sec = 0; unsigned int tv_usec = 0;
    unsigned char *byte_addr = 0;
    int clen = 0;

    assert(dm_api);   
    assert(config);   
    
    dm_udsc = dm_api_udsc_get(dm_api, udscid);
    if (dm_udsc == 0) {
        log_e("The udsc id is invalid => %d \n", udscid);
        return -1;      
    }
    byte_addr = dm_api->txbuf + DM_IPC_COMMON_MSG_SIZE;
    clen = dm_general_config_encode(byte_addr, sizeof(dm_api->txbuf) - DM_IPC_COMMON_MSG_SIZE, config);
    /* 请求数据编码 */
    dm_common_encode(dm_api->txbuf, sizeof(dm_api->txbuf), cmd, recode, udscid, tv_sec, tv_usec);
    if (dm_api_sendto_diag_master(dm_api, dm_api->txbuf, DM_IPC_COMMON_MSG_SIZE + clen) == \
        DM_IPC_COMMON_MSG_SIZE + clen) {    
        if (dm_api_recv_reply_handler(dm_api, &udscid, \
                DM_CAMMAND_OTA_GENERAL_CONFIG_REPLY, DM_CAMMAND_ERR_NO) == 0) {
            return 0;
        }
    }
        
    return -2;      
}

/*
    设置UDS服务请求处理的回调函数
*/
int dm_api_udsc_request_transfer_callback_set(diag_master_api *dm_api, unsigned short udscid, uds_service_request_callback call, void *arg)
{
    struct dm_api_udscs *dm_udsc = 0;

    assert(dm_api);   
    if (call == 0) {
        return -1;
    }
    
    dm_udsc = dm_api_udsc_get(dm_api, udscid);
    if (dm_udsc == 0) {
        log_e("The udsc id is invalid => %d \n", udscid);
        return -2;
    }
    dm_udsc->service_request = call;
    dm_udsc->rt_arg = arg;

    return 0;
}

static void dm_api_cammand_service_request_handler(diag_master_api *dm_api, unsigned int recode, unsigned short udscid)
{
    unsigned int to_cmd = DM_CAMMAND_SERVICE_INDICATION_REPLY;
    unsigned int to_recode = DM_CAMMAND_ERR_NO;
    unsigned short to_id = udscid;
    unsigned int tv_sec = 0; unsigned int tv_usec = 0;    
    unsigned int A_SA = 0, A_TA = 0, payload_length = 0, TA_TYPE = 0; 
    unsigned char *byte_addr = 0;
    unsigned char *payload = 0;
    struct dm_api_udscs *dm_udsc = 0;

    log_d("Send command: %s Recode: %d udsc ID: %d \n", dm_ipc_command_str(to_cmd), to_recode, to_id);
    dm_common_encode(dm_api->txbuf, sizeof(dm_api->txbuf), to_cmd, to_recode, to_id, tv_sec, tv_usec);
    dm_api_sendto_diag_master(dm_api, dm_api->txbuf, DM_IPC_COMMON_MSG_SIZE);  
    
    dm_udsc = dm_api_udsc_get(dm_api, udscid);
    byte_addr = dm_api->rxbuf + DM_IPC_COMMON_MSG_SIZE;
    A_SA = *(unsigned int *)(byte_addr + SERVICE_INDICATION_A_SA_OFFSET);
    A_TA = *(unsigned int *)(byte_addr + SERVICE_INDICATION_A_TA_OFFSET);
    payload_length = *(unsigned int *)(byte_addr + SERVICE_INDICATION_PAYLOAD_LEN_OFFSET);
    TA_TYPE = *(unsigned int *)(byte_addr + SERVICE_INDICATION_TA_TYPE_OFFSET);
    payload = byte_addr + SERVICE_INDICATION_PAYLOAD_OFFSET;
    log_d("Mtype: %d A_SA: 0x%8X A_TA: 0x%8X TA_TYPE: %d pl: %d \n", 0, A_SA, A_TA, TA_TYPE, payload_length);
    log_hex_d("payload: ", payload, payload_length);
    if (dm_udsc->service_request) {
        dm_udsc->service_request(dm_udsc->rt_arg, udscid, payload, payload_length, A_SA, A_TA, TA_TYPE);
    }
}

/*
    设置UDS客户端所有诊断服务任务结束后的回调函数
*/
int dm_api_udsc_service_finish_callback_set(diag_master_api *dm_api, unsigned short udscid, uds_all_service_finish_callback call, void *arg)
{
    struct dm_api_udscs *dm_udsc = 0;

    assert(dm_api);   
    if (call == 0) {
        return -1;
    }
    
    dm_udsc = dm_api_udsc_get(dm_api, udscid);
    if (dm_udsc == 0) {
        log_e("The udsc id is invalid => %d \n", udscid);
        return -2;
    }
    dm_udsc->all_service_finish = call;
    dm_udsc->asf_arg = arg;

    return 0;
}

int dm_api_udsc_service_sa_seed_callback_set(diag_master_api *dm_api, unsigned short udscid, uds_service_sa_seed_callback call, void *arg)
{
    struct dm_api_udscs *dm_udsc = 0;

    assert(dm_api);   
    if (call == 0) {
        return -1;
    }
    
    dm_udsc = dm_api_udsc_get(dm_api, udscid);
    if (dm_udsc == 0) {
        log_e("The udsc id is invalid => %d \n", udscid);
        return -2;
    }
    dm_udsc->sa_seed_callback = call;
    dm_udsc->sa_seed_arg = arg;

    return 0;
}

/*
    UDS客户端所有诊断服务任务结束后回调函数
*/
static void dm_api_cammand_services_finish_handler(diag_master_api *dm_api, unsigned int recode, unsigned short udscid)
{
    unsigned int to_cmd = DM_CAMMAND_SCRIPT_RESULT_REPLY;
    unsigned int to_recode = DM_CAMMAND_ERR_NO;
    unsigned short to_id = udscid;
    unsigned int tv_sec = 0; unsigned int tv_usec = 0;
    unsigned char *ind = NULL, *resp = NULL;
    unsigned int indl = 0, respl = 0;
    struct dm_api_udscs *dm_udsc = 0;
    
    log_d("Send command: %s Recode: %d udsc ID: %d \n", dm_ipc_command_str(to_cmd), to_recode, to_id);
    dm_common_encode(dm_api->txbuf, sizeof(dm_api->txbuf), to_cmd, to_recode, to_id, tv_sec, tv_usec);
    dm_api_sendto_diag_master(dm_api, dm_api->txbuf, DM_IPC_COMMON_MSG_SIZE); 

    indl = *(unsigned int *)(dm_api->rxbuf + SERVICE_FINISH_RESULT_IND_LEN_OFFSET);
    if (indl > 0) {
        ind = dm_api->rxbuf + SERVICE_FINISH_RESULT_IND_OFFSET;
    }
    respl = *(unsigned int *)(dm_api->rxbuf + SERVICE_FINISH_RESULT_IND_OFFSET + indl);
    if (respl > 0) {
        resp = dm_api->rxbuf + SERVICE_FINISH_RESULT_IND_OFFSET + indl + SERVICE_FINISH_RESULT_RESP_LEN_SIZE;
    }
    dm_udsc = dm_api_udsc_get(dm_api, udscid);
    if (dm_udsc->all_service_finish) {
        dm_udsc->all_service_finish(dm_udsc->asf_arg, udscid, recode, ind, indl, resp, respl);
    }
}

static void dm_api_cammand_service_result_handler(diag_master_api *dm_api, unsigned int recode, unsigned short udscid)
{
    unsigned int to_cmd = DM_CAMMAND_SERVICE_REQUEST_RESULT_REPLY;
    unsigned int to_recode = DM_CAMMAND_ERR_NO;
    unsigned short to_id = udscid;
    unsigned int tv_sec = 0; unsigned int tv_usec = 0;
    struct dm_api_udscs *dm_udsc = 0;
    unsigned int rr_callid = 0;
    unsigned int dlen = 0;
    unsigned char *payload = 0;

    log_d("Send command: %s Recode: %d udsc ID: %d \n", dm_ipc_command_str(to_cmd), to_recode, to_id);
    dm_common_encode(dm_api->txbuf, sizeof(dm_api->txbuf), to_cmd, to_recode, to_id, tv_sec, tv_usec);
    dm_api_sendto_diag_master(dm_api, dm_api->txbuf, DM_IPC_COMMON_MSG_SIZE);  
    
    dm_udsc = dm_api_udsc_get(dm_api, udscid);
    rr_callid = *(unsigned int *)(dm_api->rxbuf + SERVICE_REQUEST_RESULT_RR_CALLID_OFFSET);
    dlen = *(unsigned int *)(dm_api->rxbuf + SERVICE_REQUEST_RESULT_DATA_LEN_OFFSET);
    payload = dm_api->rxbuf + SERVICE_REQUEST_RESULT_DATA_OFFSET;    
    log_hex_d("UDS Request result: ", payload, dlen);
    if (rr_callid > 0 && rr_callid < UDSC_REQUEST_RESULT_CALLBACK_MAX) {
        if (dm_udsc->rr_handler[rr_callid - 1].service_result) {
            dm_udsc->rr_handler[rr_callid - 1].service_result(dm_udsc->rr_handler[rr_callid - 1].rr_arg, udscid, payload, dlen);
        }
    }  
}

static void dm_api_cammand_service_sa_seed_handler(diag_master_api *dm_api, unsigned int recode, unsigned short udscid)
{
    struct dm_api_udscs *dm_udsc = 0;
    unsigned int to_cmd = DM_CAMMAND_SERVICE_SA_SEED_REPLY;
    unsigned int to_recode = DM_CAMMAND_ERR_NO;
    unsigned short to_id = udscid;
    unsigned int tv_sec = 0; unsigned int tv_usec = 0;
    unsigned short seed_size = 0;
    unsigned char *seed = 0;
    unsigned char level = 0;

    log_d("Send command: %s Recode: %d udsc ID: %d \n", dm_ipc_command_str(to_cmd), to_recode, to_id);
    dm_common_encode(dm_api->txbuf, sizeof(dm_api->txbuf), to_cmd, to_recode, to_id, tv_sec, tv_usec);
    dm_api_sendto_diag_master(dm_api, dm_api->txbuf, DM_IPC_COMMON_MSG_SIZE);  
    
    dm_udsc = dm_api_udsc_get(dm_api, udscid);
    level = *(unsigned char *)(dm_api->rxbuf + SERVICE_SA_SEED_LEVEL_OFFSET);
    seed_size = *(unsigned short *)(dm_api->rxbuf + SERVICE_SA_SEED_SIZE_OFFSET);
    seed = dm_api->rxbuf + SERVICE_SA_SEED_OFFSET;    
    log_hex_d("UDS Request seed: ", seed, seed_size);
    if (dm_udsc->sa_seed_callback) {
        dm_udsc->sa_seed_callback(dm_udsc->sa_seed_arg, udscid, level, seed, seed_size);
    }
}

static void dm_api_cammand_keepalive_request_handler(diag_master_api *dm_api, unsigned int recode, unsigned short udscid)
{
    unsigned int to_cmd = DM_CAMMAND_OMAPI_KEEPALIVE_REPLY;
    unsigned int to_recode = DM_CAMMAND_ERR_NO;
    unsigned int tv_sec = 0; unsigned int tv_usec = 0;

    log_d("Send command: %s Recode: %d udsc ID: %d \n", dm_ipc_command_str(to_cmd), to_recode, 0);
    dm_common_encode(dm_api->txbuf, sizeof(dm_api->txbuf), to_cmd, to_recode, 0, tv_sec, tv_usec);
    dm_api_sendto_diag_master(dm_api, dm_api->txbuf, DM_IPC_COMMON_MSG_SIZE);  
}

int dm_api_udsc_diag_id_storage(diag_master_api *dm_api, unsigned short udscid, unsigned int diag_req_id, unsigned int diag_resp_id)
{
    struct dm_api_udscs *dm_udsc = 0;
    
    assert(dm_api);   
    dm_udsc = dm_api_udsc_get(dm_api, udscid);
    if (dm_udsc == 0) {
        log_e("The udsc id is invalid => %d \n", udscid);
        return -1;
    }
    dm_udsc->diag_req_id = diag_req_id;
    dm_udsc->diag_resp_id = diag_resp_id;

    return 0;
}

int dm_api_udsc_index_by_resp_id(diag_master_api *dm_api, unsigned int diag_resp_id)
{
    int index = 0;

    assert(dm_api);
    for (index = 0; index < DM_UDSC_CAPACITY_MAX; index++) {
        struct dm_api_udscs *dm_udsc = dm_api_udsc_get(dm_api, index);
        if (dm_udsc && \
            dm_udsc->diag_resp_id == diag_resp_id) {
            return index;
        }
    }

    return -1;
}

int dm_api_udsc_index_by_req_id(diag_master_api *dm_api, unsigned int diag_req_id)
{
    int index = 0;

    assert(dm_api);
    for (index = 0; index < DM_UDSC_CAPACITY_MAX; index++) {
        struct dm_api_udscs *dm_udsc = dm_api_udsc_get(dm_api, index);
        if (dm_udsc && \
            dm_udsc->diag_req_id == diag_req_id) {
            return index;
        }
    }

    return -1;
}

int dm_api_udsc_resp_id(diag_master_api *dm_api, unsigned short udscid)
{
    struct dm_api_udscs *dm_udsc = 0;

    assert(dm_api);
    dm_udsc = dm_api_udsc_get(dm_api, udscid);
    if (dm_udsc == 0) {
        log_d("The udsc id is invalid => %d \n", udscid);
        return 0;
    }
    
    return dm_udsc->diag_resp_id;
}

int dm_api_udsc_req_id(diag_master_api *dm_api, unsigned short udscid)
{
    struct dm_api_udscs *dm_udsc = 0;
    
    assert(dm_api);
    dm_udsc = dm_api_udsc_get(dm_api, udscid);
    if (dm_udsc == 0) {
        log_e("The udsc id is invalid => %d \n", udscid);
        return 0;
    }

    return dm_udsc->diag_req_id;
}

/*
    诊断应答给UDS客户端处理
*/
int dm_api_service_response(diag_master_api *dm_api, unsigned short udscid, const unsigned char *data, unsigned int size, unsigned int sa, unsigned int ta, unsigned int tatype)
{
    unsigned char *byte_addr = dm_api->txbuf + DM_IPC_COMMON_MSG_SIZE;
    unsigned int cmd = DM_CAMMAND_SERVICE_RESPONSE_REQUEST;
    unsigned int recode = DM_CAMMAND_ERR_NO;
    unsigned int tv_sec = 0; unsigned int tv_usec = 0;
    unsigned int A_SA = 0, A_TA = 0, payload_length = 0, TA_TYPE = 0; 

    assert(dm_api);
    /* encode 4byte UDS响应源地址 */
    memcpy(byte_addr + SERVICE_RESPONSE_A_SA_OFFSET, &sa, SERVICE_RESPONSE_A_SA_SIZE);
    /* encode 4byte UDS响应目的地址 */
    memcpy(byte_addr + SERVICE_RESPONSE_A_TA_OFFSET, &ta, SERVICE_RESPONSE_A_TA_SIZE);
    /* encode 4byte UDS响应目的地址类型 */
    memcpy(byte_addr + SERVICE_RESPONSE_TA_TYPE_OFFSET, &tatype, SERVICE_RESPONSE_TA_TYPE_SIZE);
    /* encode 4byte UDS响应数据长度 */
    memcpy(byte_addr + SERVICE_RESPONSE_PAYLOAD_LEN_OFFSET, &size, SERVICE_RESPONSE_PAYLOAD_LEN_SIZE);
    /* encode nbyte UDS响应数据 */
    memcpy(byte_addr + SERVICE_RESPONSE_PAYLOAD_OFFSET, data, size);

    A_SA = *(unsigned int *)(byte_addr + SERVICE_RESPONSE_A_SA_OFFSET);
    A_TA = *(unsigned int *)(byte_addr + SERVICE_RESPONSE_A_TA_OFFSET);
    payload_length = *(unsigned int *)(byte_addr + SERVICE_RESPONSE_PAYLOAD_LEN_OFFSET);
    TA_TYPE = *(unsigned int *)(byte_addr + SERVICE_RESPONSE_TA_TYPE_OFFSET);
    byte_addr += SERVICE_RESPONSE_PAYLOAD_OFFSET;    
    log_d("Mtype: %d A_SA: 0x%08X A_TA: 0x%08X TA_TYPE: %d pl: %d \n", 0, A_SA, A_TA, TA_TYPE, payload_length);
    log_hex_d("UDS Service Response", byte_addr, payload_length);

    dm_common_encode(dm_api->txbuf, sizeof(dm_api->txbuf), cmd, recode, udscid, tv_sec, tv_usec);
    if (dm_api_sendto_diag_master(dm_api, dm_api->txbuf, DM_IPC_COMMON_MSG_SIZE + SERVICE_RESPONSE_PAYLOAD_OFFSET + size) == \
        DM_IPC_COMMON_MSG_SIZE + SERVICE_RESPONSE_PAYLOAD_OFFSET + size) {    
        if (dm_api_recv_reply_handler(dm_api, &udscid, \
                DM_CAMMAND_SERVICE_RESPONSE_REPLY, DM_CAMMAND_ERR_NO) == 0) {
            return 0;
        }
    }
        
    return -1;      
}

void dm_api_request_event_loop(diag_master_api *dm_api)
{
    unsigned int cmd = 0; unsigned int recode = 0; unsigned short udscid = 0;
    unsigned int tv_sec = 0; unsigned int tv_usec = 0;
    int recvByte = -1;
    struct dm_api_udscs *dm_udsc = 0;

    assert(dm_api);
    recvByte = dm_recvfrom(dm_api->sockfd, dm_api->rxbuf, sizeof(dm_api->rxbuf), 0);
    if (recvByte > 0) {
        dm_api_request_handler(dm_api);
        memset(dm_api->rxbuf, 0, recvByte);
    } 

    return ;
}
