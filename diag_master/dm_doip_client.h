#ifndef __DM_DOIP_CLIENT_H__
#define __DM_DOIP_CLIENT_H__

typedef enum doipc_tcp_status_e {
    DOIPC_TCP_DISCONNECT,
    DOIPC_TCP_CONNECT_PROCESS,
    DOIPC_TCP_CONNECT_SUCCESS,
} doipc_tcp_status_e;

typedef struct doipc_config_s {
    unsigned char ver;
    unsigned short sa_addr;
    unsigned int src_ip;
    unsigned short src_port;
    unsigned int dst_ip;
    unsigned short dst_port;
} doipc_config_t;

typedef struct doip_client_s {
#ifdef __HAVE_LIBEV__
    struct ev_loop *loop; /* ���¼�ѭ���ṹ�� */
    ev_io tcprecv_watcher; /* ����TCP���¼� */
    ev_io tcpsend_watcher; /* ����TCPд�¼� */
#endif /* __HAVE_LIBEV__ */
    int index; /* ������� */
    int udp_sfd; /* UDP socket */
    int tcp_sfd; /* TCP socket */
#define RXTX_BUFF_LEN_DEF (10240)
    char *rxbuf; /* ����Buff */
    unsigned int rxlen; /* ����Buff���� */
    char *txbuf; /* ����Buff */
    unsigned int txlen; /* ����buff���� */
    doipc_tcp_status_e con_stat; /* tcp����״̬ */
    int isactive; /* �Ƿ��Ѿ�·�ɼ��� */    
    doipc_config_t config;
} doip_client_t;

int dm_doipc_diagnostic_request(doip_client_t *doipc, INT16U sa, INT16U ta, const INT8U *msg, INT32U len, INT32U timeout);

int dm_doipc_disconnect_server(doip_client_t *doipc);

int dm_doipc_connect_active_server(doip_client_t *doipc, doipc_config_t *config);

doip_client_t *dm_doipc_create(struct ev_loop *loop);

int dm_doipc_destory(doip_client_t *doipc);


#endif /* __DM_DOIP_CLIENT_H__ */
