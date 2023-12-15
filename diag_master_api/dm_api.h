#ifndef __DM_API_H__
#define __DM_API_H__

typedef void (*uds_service_request_callback)(void *arg, unsigned short udscid, const unsigned char *data, unsigned int size, unsigned int sa, unsigned int ta, unsigned int tatype);
typedef void (*uds_all_service_finish_callback)(void *arg, unsigned short udscid, unsigned int result, const unsigned char *ind, unsigned int indl, const unsigned char *resp, unsigned int respl);
typedef void (*uds_service_result_callback)(void *arg, unsigned short udscid, const unsigned char *data, unsigned int size);
typedef void (*uds_service_sa_seed_callback)(void *arg, unsigned short udscid, unsigned char level, const unsigned char *data, unsigned int size);

typedef struct dm_udsc_config {   
    unsigned int cmd_valid_time; /* 命令有效时间,收发端超时的命令造成的异常 */
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
    这些接口用于控制OTA Master 做出相应动作
*/
/* ---------- OTA master api 所有的接口均为同步接口，会阻塞 --------- */

/* 打开或者关闭调试信息默认关闭 */
void dm_debug_enable(int eb);

/* 创建初始化OTA MASTER API */
diag_master_api *dm_api_create();
void dm_api_destory(diag_master_api *dm_api);

/* 获取和OTA MASTER进程间通信的socket，可以用于监听是否有进程通信数据 */
int dm_api_sockfd(diag_master_api *dm_api);

/* 销毁OTA MASTER API */
void dm_api_destory(diag_master_api *dm_api);

/* 请求OTA MASTER创建一个UDS客户端，成功返回非负数UDS客户端ID，失败返回-1 */
int dm_api_udsc_create(diag_master_api *dm_api, dm_udsc_config *config);

/* 请求OTA MASTER销毁一个UDS客户端 */
int dm_api_udsc_destory(diag_master_api *dm_api, unsigned short udscid);

/* 请求OTA MASTER创建一个DOIP客户端，并将UDS客户端与DOIP客户端绑定,
   成功返回非负数UDS客户端ID，UDS客户端销毁的同时将销毁DOIP客户端，失败返回-1 */
int dm_api_doipc_create(diag_master_api *dm_api, unsigned short udscid, doipc_config_t *config);

/*  请求OTA MASTER启动UDS 客户端开始执行诊断任务 */
int dm_api_udsc_start(diag_master_api *dm_api, unsigned short udscid);

/*  请求OTA MASTER中止UDS 客户端执行诊断任务 */
int dm_api_udsc_stop(diag_master_api *dm_api, unsigned short udscid);    

/* 复位OTA MASTER防止OTA MASTER存在异常数据 */
int dm_api_master_reset(diag_master_api *dm_api);

/* 配置OTA MASTER中的UDS诊断刷写任务 */
int dm_api_udsc_service_config(diag_master_api *dm_api, unsigned short udscid, service_config *config);

/* UDS客户端的一些通用设置  包括3E服务、任务执行动作等配置 */
int dm_api_udsc_general_config(diag_master_api *dm_api, unsigned short udscid, udsc_general_config *config);

/* 将生成的key发送给ota master */
int dm_api_udsc_sa_key(diag_master_api *dm_api, unsigned short udscid, unsigned char level, const unsigned char *key, unsigned int key_size);

/* UDS的诊断响应报文发送给OTA MASTER中的UDS客户端处理 */
int dm_api_service_response(diag_master_api *dm_api, unsigned short udscid, const unsigned char *data, unsigned int size, unsigned int sa, unsigned int ta, unsigned int tatype);

/* 设置OTA MASTER有诊断请求需要发送时候的诊断请求发送回调函数 */
int dm_api_udsc_request_transfer_callback_set(diag_master_api *dm_api, unsigned short udscid, uds_service_request_callback call, void *arg);

/* 设置OTA MASTER中诊断客户端所有请求任务都执行完成后结果处理回调函数 */
int dm_api_udsc_service_finish_callback_set(diag_master_api *dm_api, unsigned short udscid, uds_all_service_finish_callback call, void *arg);

/* 设置单个诊断任务请求响应结果的回调处理函数 */
int dm_api_udsc_service_result_callback_set(diag_master_api *dm_api, unsigned short udscid, uds_service_result_callback call, void *arg);

/* 通过27服务获取的种子生成key的回调函数设置 */
int dm_api_udsc_service_sa_seed_callback_set(diag_master_api *dm_api, unsigned short udscid, uds_service_sa_seed_callback call, void *arg);

/* 保存诊断请求和响应ID/地址 */
int dm_api_udsc_diag_id_storage(diag_master_api *dm_api, unsigned short udscid, unsigned int diag_req_id, unsigned int diag_resp_id);

/* 通过诊断响应ID/地址查找UDS客户端ID */
int dm_api_udsc_index_by_resp_id(diag_master_api *dm_api, unsigned int diag_resp_id);

/* 通过诊断请求ID/地址查找UDS客户端ID */
int dm_api_udsc_index_by_req_id(diag_master_api *dm_api, unsigned int diag_req_id);

/* 通过UDS客户端ID查找诊断响应ID/地址 */
int dm_api_udsc_resp_id(diag_master_api *dm_api, unsigned short udscid);

/* 通过UDS客户端ID查找诊断请求ID/地址 */
int dm_api_udsc_req_id(diag_master_api *dm_api, unsigned short udscid);

/* 用于和OTA MASTER进行进程间通信建议和dm_api_sockfd() 和select epoll 函数配合使用 */
void dm_api_request_event_loop(diag_master_api *dm_api);

#endif /* __DM_API_H__ */
