#ifndef __DIAG_MASTER_H__
#define __DIAG_MASTER_H__

#define DM_UDSC_CAPACITY_DEF (10)

struct DMS;
typedef struct DMS DMS;

typedef struct diag_master {
#ifdef __linux__
    int sockfd; /* ��diag_master_apiͨ��ʹ�� */
#endif /* __linux__ */
#ifdef _WIN32
    SOCKET sockfd;
#endif /* _WIN32 */
    int index; /* ����ֵ���ڴ���ʱ���� */
#ifdef __HAVE_LIBEV__
    struct ev_loop* loop; /* �¼���ѭ�� */
    ev_io iwrite_watcher; /* ����IPC���¼� */
    ev_io iread_watcher; /* ����IPCд�¼� */
    ev_timer keepalive_watcher; /* ������ʱ���������ж�API�Ƿ����� */
#endif /* __HAVE_LIBEV__ */
#ifdef __HAVE_LIBUV__
    uv_loop_t* loop; /* �¼���ѭ�� */
    uv_udp_send_t udp_send;
    uv_udp_t iwrite_watcher; /* ����IPC���¼� */
    uv_udp_t iread_watcher; /* ����IPCд�¼� */
    uv_timer_t keepalive_watcher; /* ������ʱ���������ж�API�Ƿ����� */
#endif /* __HAVE_LIBUV__ */
    INT32U keepalive_cnt; /* �����������յ�����Ӧ��������㣬������������ΪOTA MASTER API�����쳣 */
    INT32U keepalive_interval;
    INT32U udsc_cnt; /* UDS�ͻ������� */
    uds_client *udscs[DM_UDSC_CAPACITY_MAX]; /* uds�ͻ��˱������±����UDS�ͻ��˵�idֵ */
    unsigned char txbuf[10240]; /* socket ����buff */
    unsigned char rxbuf[10240]; /* socket ����buff */
#ifdef __linux__
    char dm_path[256]; /* OTA MASTER��UNIX socket·�� */
    char dmapi_path[256]; /* OTA MASTER API��UNIX socket·�� */
#endif /* __linux__ */
#ifdef _WIN32
    char ip_str[64];
    unsigned int api_ip;
    unsigned short api_port;
#endif /* _WIN32 */
    DMS *dms;
} diag_master_t;

typedef struct DMS {    
    int sockfd; /* ��diag_master_apiͨ��ʹ�� */
#ifdef _WIN32
    unsigned int port_base;
#endif /* _WIN32 */
#ifdef __HAVE_LIBEV__
    struct ev_loop* loop; /* �¼���ѭ�� */
    ev_io iwrite_watcher; /* ����IPC���¼� */
    ev_io iread_watcher; /* ����IPCд�¼� */
#else /* __HAVE_LIBEV__ */
    uv_loop_t* loop; /* �¼���ѭ�� */
    uv_udp_send_t udp_send;
    uv_udp_t iwrite_watcher; /* ����IPC���¼� */
    uv_udp_t iread_watcher; /* ����IPCд�¼� */
#endif /* __HAVE_LIBEV__ */
#define DMS_DIAG_MASTER_NUM_MAX (128)    
    diag_master_t *diag_master[DMS_DIAG_MASTER_NUM_MAX];     
    unsigned char txbuf[2048]; /* socket ����buff */
    unsigned char rxbuf[2048]; /* socket ����buff */
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
