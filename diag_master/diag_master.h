#ifndef __DIAG_MASTER_H__
#define __DIAG_MASTER_H__

#define DM_UDSC_CAPACITY_DEF (10)

struct DMS;
typedef struct DMS DMS;

typedef struct diag_master {
#ifdef __linux__
    int sockfd; /* 与diag_master_api通信使用 */
#endif /* __linux__ */
#ifdef _WIN32
    SOCKET sockfd;
#endif /* _WIN32 */
    int index; /* 索引值，在创建时设置 */
#ifdef __HAVE_LIBEV__
    struct ev_loop* loop; /* 事件主循环 */
    ev_io iwrite_watcher; /* 监听IPC读事件 */
    ev_io iread_watcher; /* 监听IPC写事件 */
    ev_timer keepalive_watcher; /* 心跳定时器，用于判断API是否在线 */
#endif /* __HAVE_LIBEV__ */
#ifdef __HAVE_LIBUV__
    uv_loop_t* loop; /* 事件主循环 */
    uv_udp_send_t udp_send;
    uv_udp_t iwrite_watcher; /* 监听IPC读事件 */
    uv_udp_t iread_watcher; /* 监听IPC写事件 */
    uv_timer_t keepalive_watcher; /* 心跳定时器，用于判断API是否在线 */
#endif /* __HAVE_LIBUV__ */
    INT32U keepalive_cnt; /* 心跳计数，收到心跳应答计数清零，超过计数则认为OTA MASTER API出现异常 */
    INT32U keepalive_interval;
    INT32U udsc_cnt; /* UDS客户端数量 */
    uds_client *udscs[DM_UDSC_CAPACITY_MAX]; /* uds客户端表，数组下标就是UDS客户端的id值 */
    unsigned char txbuf[10240]; /* socket 发送buff */
    unsigned char rxbuf[10240]; /* socket 接收buff */
#ifdef __linux__
    char dm_path[256]; /* OTA MASTER的UNIX socket路径 */
    char dmapi_path[256]; /* OTA MASTER API的UNIX socket路径 */
#endif /* __linux__ */
#ifdef _WIN32
    char ip_str[64];
    unsigned int api_ip;
    unsigned short api_port;
#endif /* _WIN32 */
    DMS *dms;
} diag_master_t;

typedef struct DMS {    
    int sockfd; /* 与diag_master_api通信使用 */
#ifdef _WIN32
    unsigned int port_base;
#endif /* _WIN32 */
#ifdef __HAVE_LIBEV__
    struct ev_loop* loop; /* 事件主循环 */
    ev_io iwrite_watcher; /* 监听IPC读事件 */
    ev_io iread_watcher; /* 监听IPC写事件 */
#else /* __HAVE_LIBEV__ */
    uv_loop_t* loop; /* 事件主循环 */
    uv_udp_send_t udp_send;
    uv_udp_t iwrite_watcher; /* 监听IPC读事件 */
    uv_udp_t iread_watcher; /* 监听IPC写事件 */
#endif /* __HAVE_LIBEV__ */
#define DMS_DIAG_MASTER_NUM_MAX (128)    
    diag_master_t *diag_master[DMS_DIAG_MASTER_NUM_MAX];     
    unsigned char txbuf[2048]; /* socket 发送buff */
    unsigned char rxbuf[2048]; /* socket 接收buff */
} DMS;

diag_master_t *diag_master_create(const char *dm_path, const char *dmapi_path);

void diag_master_destory(diag_master_t *diag_master);

int diag_master_service_request(diag_master_t *diag_master, INT16U id, const INT8U *data, INT32U size, INT32U sa, INT32U ta, INT32U tatype);

int diag_master_all_service_result(diag_master_t *diag_master, INT16U id, INT32U result, const INT8U *ind, INT32U indl, const INT8U *resp, INT32U respl);

int diag_master_service_request_result(diag_master_t *diag_master, INT16U id, const INT8U *data, INT32U size, INT32U rr_callid);

int diag_master_service_sa_seed_request(diag_master_t *diag_master, INT16U id, INT8U level, const INT8U *data, INT32U size);

#ifdef __HAVE_LIBUV__
uv_loop_t* diag_master_ev_loop(diag_master_t* diag_master);
#endif /* __HAVE_LIBUV__  */
#ifdef __HAVE_LIBEV__
struct ev_loop* diag_master_ev_loop(diag_master_t* diag_master);
#endif /* __HAVE_LIBEV__  */
#endif /* __DIAG_MASTER_H__ */
