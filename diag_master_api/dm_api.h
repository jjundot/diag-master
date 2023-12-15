#ifndef __DM_API_H__
#define __DM_API_H__

typedef void (*uds_service_request_callback)(void *arg, unsigned short udscid, const unsigned char *data, unsigned int size, unsigned int sa, unsigned int ta, unsigned int tatype);
typedef void (*uds_all_service_finish_callback)(void *arg, unsigned short udscid, unsigned int result, const unsigned char *ind, unsigned int indl, const unsigned char *resp, unsigned int respl);
typedef void (*uds_service_result_callback)(void *arg, unsigned short udscid, const unsigned char *data, unsigned int size);
typedef void (*uds_service_sa_seed_callback)(void *arg, unsigned short udscid, unsigned char level, const unsigned char *data, unsigned int size);

typedef struct dm_udsc_config {   
    unsigned int cmd_valid_time; /* ������Чʱ��,�շ��˳�ʱ��������ɵ��쳣 */
} dm_udsc_config;

typedef struct doipc_config_s {
    unsigned char ver;
    unsigned short sa_addr;
    unsigned int src_ip;
    unsigned short src_port;
    unsigned int dst_ip;
    unsigned short dst_port;
} doipc_config_t;

struct diag_master_api;
typedef struct diag_master_api diag_master_api;
/*
    ��Щ�ӿ����ڿ���OTA Master ������Ӧ����
*/
/* ---------- OTA master api ���еĽӿھ�Ϊͬ���ӿڣ������� --------- */

/* �򿪻��߹رյ�����ϢĬ�Ϲر� */
void dm_debug_enable(int eb);

/* ������ʼ��OTA MASTER API */
diag_master_api *dm_api_create();
void dm_api_destory(diag_master_api *dm_api);

/* ��ȡ��OTA MASTER���̼�ͨ�ŵ�socket���������ڼ����Ƿ��н���ͨ������ */
int dm_api_sockfd(diag_master_api *dm_api);

/* ����OTA MASTER API */
void dm_api_destory(diag_master_api *dm_api);

/* ����OTA MASTER����һ��UDS�ͻ��ˣ��ɹ����طǸ���UDS�ͻ���ID��ʧ�ܷ���-1 */
int dm_api_udsc_create(diag_master_api *dm_api, dm_udsc_config *config);

/* ����OTA MASTER����һ��UDS�ͻ��� */
int dm_api_udsc_destory(diag_master_api *dm_api, unsigned short udscid);

/* ����OTA MASTER����һ��DOIP�ͻ��ˣ�����UDS�ͻ�����DOIP�ͻ��˰�,
   �ɹ����طǸ���UDS�ͻ���ID��UDS�ͻ������ٵ�ͬʱ������DOIP�ͻ��ˣ�ʧ�ܷ���-1 */
int dm_api_doipc_create(diag_master_api *dm_api, unsigned short udscid, doipc_config_t *config);

/*  ����OTA MASTER����UDS �ͻ��˿�ʼִ��������� */
int dm_api_udsc_start(diag_master_api *dm_api, unsigned short udscid);

/*  ����OTA MASTER��ֹUDS �ͻ���ִ��������� */
int dm_api_udsc_stop(diag_master_api *dm_api, unsigned short udscid);    

/* ��λOTA MASTER��ֹOTA MASTER�����쳣���� */
int dm_api_master_reset(diag_master_api *dm_api);

/* ����OTA MASTER�е�UDS���ˢд���� */
int dm_api_udsc_service_config(diag_master_api *dm_api, unsigned short udscid, service_config *config);

/* UDS�ͻ��˵�һЩͨ������  ����3E��������ִ�ж��������� */
int dm_api_udsc_general_config(diag_master_api *dm_api, unsigned short udscid, udsc_general_config *config);

/* �����ɵ�key���͸�ota master */
int dm_api_udsc_sa_key(diag_master_api *dm_api, unsigned short udscid, unsigned char level, const unsigned char *key, unsigned int key_size);

/* UDS�������Ӧ���ķ��͸�OTA MASTER�е�UDS�ͻ��˴��� */
int dm_api_service_response(diag_master_api *dm_api, unsigned short udscid, const unsigned char *data, unsigned int size, unsigned int sa, unsigned int ta, unsigned int tatype);

/* ����OTA MASTER�����������Ҫ����ʱ�����������ͻص����� */
int dm_api_udsc_request_transfer_callback_set(diag_master_api *dm_api, unsigned short udscid, uds_service_request_callback call, void *arg);

/* ����OTA MASTER����Ͽͻ���������������ִ����ɺ�������ص����� */
int dm_api_udsc_service_finish_callback_set(diag_master_api *dm_api, unsigned short udscid, uds_all_service_finish_callback call, void *arg);

/* ���õ����������������Ӧ����Ļص������� */
int dm_api_udsc_service_result_callback_set(diag_master_api *dm_api, unsigned short udscid, uds_service_result_callback call, void *arg);

/* ͨ��27�����ȡ����������key�Ļص��������� */
int dm_api_udsc_service_sa_seed_callback_set(diag_master_api *dm_api, unsigned short udscid, uds_service_sa_seed_callback call, void *arg);

/* ��������������ӦID/��ַ */
int dm_api_udsc_diag_id_storage(diag_master_api *dm_api, unsigned short udscid, unsigned int diag_req_id, unsigned int diag_resp_id);

/* ͨ�������ӦID/��ַ����UDS�ͻ���ID */
int dm_api_udsc_index_by_resp_id(diag_master_api *dm_api, unsigned int diag_resp_id);

/* ͨ���������ID/��ַ����UDS�ͻ���ID */
int dm_api_udsc_index_by_req_id(diag_master_api *dm_api, unsigned int diag_req_id);

/* ͨ��UDS�ͻ���ID���������ӦID/��ַ */
int dm_api_udsc_resp_id(diag_master_api *dm_api, unsigned short udscid);

/* ͨ��UDS�ͻ���ID�����������ID/��ַ */
int dm_api_udsc_req_id(diag_master_api *dm_api, unsigned short udscid);

/* ���ں�OTA MASTER���н��̼�ͨ�Ž����dm_api_sockfd() ��select epoll �������ʹ�� */
void dm_api_request_event_loop(diag_master_api *dm_api);

#endif /* __DM_API_H__ */
