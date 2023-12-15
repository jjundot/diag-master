#ifndef __DM_UDS_CLIENT_H__
#define __DM_UDS_CLIENT_H__

#define SA_KEY_SEED_SIZE (512)

typedef enum service_response_stat {
    SERVICE_RESPONSE_NORMAL,
    SERVICE_RESPONSE_TIMEOUT,
} service_response_stat;

typedef struct service_36_td {
#define MAX_NUMBER_OF_BLOCK_LENGTH (10000) /* �豸֧�ֵ���������ݿ鳤�� */
    INT32U maxNumberOfBlockLength; /* �����鳤�� */    
    char *local_path;
} service_36_td;

typedef struct service_34_rd {
    INT8U dataFormatIdentifier;
    INT8U addressAndLengthFormatIdentifier;
    INT32U memoryAddress;
    INT32U memorySize;
} service_34_rd;

typedef struct service_38_rft {
    INT8U modeOfOperation;
    INT16U filePathAndNameLength;
    char filePathAndName[256];
    INT8U dataFormatIdentifier;
    INT8U fileSizeParameterLength;
    INT32U fileSizeUnCompressed;
    INT32U fileSizeCompressed;
} service_38_rft;

typedef int (*DMKey)(
    const unsigned char*    iSeedArray,        // seed����
    unsigned short          iSeedArraySize,    // seed�����С
    unsigned int            iSecurityLevel,    // ��ȫ���� 1,3,5...
    const char*             iVariant,          // ��������, ������Ϊ��
    unsigned char*          iKeyArray,         // key����, �ռ����û�����
    unsigned short*         iKeyArraySize      // key�����С, ��������ʱΪkey����ʵ�ʴ�С
);

typedef struct service_27_ac {
    DMKey key_callback;
} service_27_ac;

typedef void (*service_variable_byte_callback)(void *item, ByteArray *variable_byte, void *arg);

typedef void (*service_response_callback)(void *item, const INT8U *data, INT32U len, void *arg);

/* ����ṹ���ڵı���ֵ�����������Ϸ�����ʱ��ʼ����ֵ�ģ�����ִ�й����в��޸����б���ֵ */
typedef struct service_item {
    BOOLEAN isenable;
    INT8U sid; /* ��Ϸ���ID */
    INT8U sub; /* ��Ϸ����ӹ��ܣ�option */
    INT16U did; /* ���ݱ�ʶ����option */
    INT32 delay; /* ��������ʱʱ�䣬��ʱʱ�䵽���Żᴦ������������ unit ms */
    INT32 timeout; /* Ӧ��ȴ���ʱʱ�� unit ms */
    BOOLEAN issuppress; /* �Ƿ���������Ӧ */
    INT32U tatype; /* Ŀ�ĵ�ַ���� */
    INT32U ta; /* ���Ŀ�ĵ�ַ */
    INT32U sa; /* ���Դ��ַ */
    ByteArray *request_byte; /* �������� */
    ByteArray *variable_byte; /* �ǹ̶���ʽ�����ݣ�����31 2e������治�̶������� */
    service_variable_byte_callback vb_callback; /* �ڹ�����������ʱ����� */
    void *vb_callback_arg;
    service_response_callback response_callback; /* ���յ�Ӧ���ʱ����� */
    void *response_callback_arg;
    service_36_td td_36; /* 36��������,��ѡ���� */
    service_34_rd rd_34; /* 34��������,��ѡ���� */
    service_38_rft rft_38; /* 38��������,��ѡ���� */
    service_27_ac ac_27;
    
    serviceResponseExpect response_rule; /* Ԥ����Ӧ���� */ 
    ByteArray *expect_byte; /* Ԥ����Ӧ�ַ� */

    serviceFinishCondition finish_rule; /* �������� */
    ByteArray *finish_byte; /* ������Ӧ�ַ� */
    INT32U finish_try_num; /* �������������� */
    INT32U finish_num_max; /* ��������Դ���������������������Ƿ���Ͻ���������������������� */
    INT32U finish_time_max; /* �������ȴ�ʱ�䣬�������ʱ�䲻���Ƿ���Ͻ���������������������� */

    INT32U rr_callid; /* ota master api �˵��������ص�����ID������0����Ч */
    char *desc; /* ��Ϸ�����������Ϣ */
} service_item;

/* ��Ϸ�������ص����� */
typedef INT32 (*udsc_request_sent_callback)(INT16U id, void *arg, const INT8U *data, INT32U size, INT32U sa, INT32U ta, INT32U tatype);

typedef enum udsc_finish_stat {
    UDSC_NORMAL_FINISH = 0, /* �������� */
    UDSC_SENT_ERROR_FINISH, /* �������������� */
    UDSC_UNEXPECT_RESPONSE_FINISH, /* ��Ԥ��Ӧ�������� */
    UDSC_TIMEOUT_RESPONSE_FINISH, /* ��ʱӦ�������� */
} udsc_finish_stat;
/* ��������������� */
typedef void (*udsc_services_finish_callback)(void *udsc, udsc_finish_stat stat, void *arg/* �û�����ָ�� */, const INT8U *ind, INT32U indl, const INT8U *resp, INT32U respl);

/* ������������ */
typedef void (*service_sa_seed_callback)(void *arg/* �û�����ָ�� */, INT16U id, INT8U level, INT8U *seed, INT16U seed_size);

typedef struct uds_client {
    BOOLEAN isidle; /* �Ƿ�ʹ�� */
    INT16U id; /* uds�ͻ���ID */
/* -------------------------------------------------------------- */
#define IPC_CAMMAND_VALID_TIME (150) /* unit */
    INT32U cmd_valid_time; /* ������Чʱ��,�շ��˳�ʱ��������ɵ��쳣 */
/* -------------------------------------------------------------- */
#ifdef __HAVE_LIBEV__
    struct ev_loop *loop; /* ���¼�ѭ���ṹ�� */
    ev_timer request_watcher; /* �������������ʱ�� */
    ev_timer response_watcher; /* �������Ӧ��ʱ��ʱ�� */
    ev_timer testerPresent_watcher; /* �������������ʱ�� */
#endif /* __HAVE_LIBEV__ */
#ifdef __HAVE_LIBUV__
    uv_loop_t* loop; /* ���¼�ѭ���ṹ�� */
    uv_timer_t request_watcher; /* �������������ʱ�� */
    uv_timer_t response_watcher; /* �������Ӧ��ʱ��ʱ�� */
    uv_timer_t testerPresent_watcher; /* �������������ʱ�� */
#endif /* __HAVE_LIBUV__ */
    BOOLEAN isFailAbort; /* ������������������ʱ����ִֹ�з���������� */
    BOOLEAN response_meet_expect; /* ���й����б�־λ��Ӧ�����Ԥ��Ҫ�� */
    INT32 sindex; /* ��ǰ���д������Ϸ��������� service_items[sindex] */
    BOOLEAN iskeep; /* �Ƿ����ִ�е�ǰ��Ϸ����Ĭ�ϲ�����֧�ֵ�ǰ������й�����ȷ�� */
    INT32U service_cnt; /* ��Ϸ������� */
#define SERVICE_ITEM_SIZE_MAX (1000)
    INT32U service_size; /* service_items �Ĵ�С */
    service_item **service_items; /* ��Ϸ������б� */
    void *sent_arg;
    udsc_request_sent_callback sent_callback; /* �����Ϣ���ͺ���������Ҫ������������ʱ����� */
    void *finish_arg;
    udsc_services_finish_callback finish_callback; /* ��Ͽͻ��˽������� */
    service_sa_seed_callback saseed_callback; /* ����Ҫ����key��ʱ��ص�������� */
    void *saseed_arg;
    /* ������Ϸ�����֮�乲������ */
    struct common_variable {
        INT8U fileTransferCount; /* 36���������к� */
        INT32U fileTransferTotalSize; /* 36�������ļ����ֽ��� */
        INT32U fileSize; /* �ļ���С */
        INT8U *td_buff; /* 36������buff */
        void *filefd; /* �򿪵��ļ�������34 36 37 38 ����ʹ�� */
        INT32U maxNumberOfBlockLength; /* �����鳤�� */
        ByteArray *seed_byte;
        ByteArray *key_byte; /* ������seed�����ɵ�key */
        INT16U p2_server_max;
        INT16U p2_e_server_max;
    } common_var;

    /* ������������� */
    BOOLEAN tpEnable; /* ʹ����������������� */
    BOOLEAN isTpRefresh; /* �Ƿ�UDS����ˢ�¶�ʱ�� */
    INT32U tpInterval; /* ���ͼ�� unit ms */
    INT32U tpta; /* ���Ŀ�ĵ�ַ */
    INT32U tpsa; /* ���Դ��ַ */ 
    void *doipc_channel; /* doip����ͨ�� */
} uds_client;

/*
    �����ڲ������ uds_client * �� service_item * ָ���Ƿ�Ϊ�գ��ڴ���ǰ���
*/

/*
    ����һ��UDS �ͻ��ˣ���ECU����ʱ���Դ������ECU�ͻ���
*/
extern uds_client *dm_udsc_create();

/*
    �����������ǰ��ȷ��û���¼�ѭ����û�����ҵ����ִ��
    ����UDS �ͻ��ˣ������������ִ�л����¼�ѭ��û��ֹͣ��ʱ�򷵻ش���
*/
extern BOOLEAN dm_udsc_destory(uds_client *udsc);

/*
    �¼�ѭ������Ҫѭ����������������ܽ����¼�ѭ��
    �� dm_udsc_thread_loop_start ��ѡһ����
*/
extern BOOLEAN dm_udsc_event_loop_start(uds_client *udsc);

/*
    ���߳��ڽ����¼�ѭ����ֻ�õ���һ�ξ��У���Ҫ��ע�߳���Դ��������
    �� dm_udsc_event_loop_start ��ѡһ����
*/
extern BOOLEAN dm_udsc_thread_loop_start(uds_client *udsc);

/*
    ֹͣ�¼�ѭ��
*/
extern BOOLEAN dm_udsc_loop_stop(uds_client *udsc);

/*
    �Ƿ������������ִ����
*/
extern BOOLEAN dm_udsc_service_isactive(uds_client *udsc);

/*
    ��ʼִ���������
*/
extern BOOLEAN dm_udsc_services_start(uds_client *udsc);

/*
    ִֹͣ���������
*/
extern BOOLEAN dm_udsc_services_stop(uds_client *udsc);

/*
    ���������ִ�г����Ƿ���ִֹ���������
*/
extern void dm_udsc_service_fail_abort(uds_client *udsc, BOOLEAN b);

/*
    ����һ����Ϸ�����Ѿ����뵽��Ϸ���ִ�б���
*/
extern service_item *dm_udsc_service_item_add(uds_client *udsc, char *desc);

/*
    ����Ϸ���ִ�б���ɾ��һ����Ϸ��������������ʹ����
*/
extern void dm_udsc_service_item_del(uds_client *udsc, service_item *item);

/*
    ���ݽṹ���ڵ���������������
*/
extern BOOLEAN dm_udsc_service_request_build(service_item *sitem);

/*
    ������������ͻص�����
*/
extern void dm_udsc_request_sent_callback_set(uds_client *udsc, udsc_request_sent_callback sent_callback, void *arg);

/*
    ����������Ϸ���ִ�н�����Ļص�������
*/
extern void dm_udsc_services_finish_callback_set(uds_client *udsc, udsc_services_finish_callback finish_callback, void *arg);

extern void dm_udsc_service_variable_byte_callback_set(service_item *sitem, service_variable_byte_callback vb_callback, void *arg);

extern void dm_udsc_service_response_callback_set(service_item *sitem, service_response_callback response_callback, void *arg);

extern void dm_udsc_service_response_finish(uds_client *udsc, service_response_stat stat, INT32U sa, INT32U ta, INT8U *data, INT32U size);

extern service_item *dm_udsc_curr_service_item(uds_client *udsc);

extern BOOLEAN dm_udsc_reset(uds_client *udsc); 

extern void dm_udsc_service_sid_set(service_item *sitem, INT8U sid);

extern INT8U dm_udsc_service_sid_get(service_item *sitem);

extern void dm_udsc_service_sub_set(service_item *sitem, INT8U sub);

extern INT8U dm_udsc_service_sub_get(service_item *sitem);

extern void dm_udsc_service_did_set(service_item *sitem, INT16U did);

extern INT16U dm_udsc_service_did_get(service_item *sitem);

extern void dm_udsc_service_delay_set(service_item *sitem, INT32 delay);

extern void dm_udsc_service_timeout_set(service_item *sitem, INT32 timeout);

extern void dm_udsc_service_suppress_set(service_item *sitem, BOOLEAN b);

extern void dm_udsc_service_enable_set(service_item *sitem, BOOLEAN b);

extern void dm_udsc_service_expect_response_set(service_item *sitem, serviceResponseExpect rule, INT8U *data, INT32U size);

extern void dm_udsc_service_key_set(service_item *sitem, DMKey key_callback);

extern void dm_udsc_ev_loop_set(uds_client *udsc, struct ev_loop *loop);
#ifdef __HAVE_LIBUV__
extern void dm_udsc_ev_loop_set(uds_client* udsc, uv_loop_t* loop);
#else /* __HAVE_LIBUV__ */
extern void dm_udsc_ev_loop_set(uds_client* udsc, struct ev_loop* loop);
#endif /* __HAVE_LIBUV__ */
extern void dm_udsc_service_key_generate(uds_client *udsc, INT8U *key, INT16U key_size);

extern void dm_udsc_service_saseed_callback_set(uds_client *udsc, service_sa_seed_callback call, void *arg);

extern void dm_udsc_doip_channel_bind(uds_client *udsc, void *doip_channel);

extern void dm_udsc_doip_channel_unbind(uds_client *udsc);

extern void *dm_udsc_doip_channel(uds_client *udsc);
#endif /* __dm_UDS_CLIENT_H__ */
