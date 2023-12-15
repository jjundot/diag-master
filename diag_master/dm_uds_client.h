#ifndef __DM_UDS_CLIENT_H__
#define __DM_UDS_CLIENT_H__

#define SA_KEY_SEED_SIZE (512)

typedef enum service_response_stat {
    SERVICE_RESPONSE_NORMAL,
    SERVICE_RESPONSE_TIMEOUT,
} service_response_stat;

typedef struct service_36_td {
#define MAX_NUMBER_OF_BLOCK_LENGTH (10000) /* 设备支持的最大传输数据块长度 */
    INT32U maxNumberOfBlockLength; /* 最大传输块长度 */    
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
    const unsigned char*    iSeedArray,        // seed数组
    unsigned short          iSeedArraySize,    // seed数组大小
    unsigned int            iSecurityLevel,    // 安全级别 1,3,5...
    const char*             iVariant,          // 其他数据, 可设置为空
    unsigned char*          iKeyArray,         // key数组, 空间由用户构造
    unsigned short*         iKeyArraySize      // key数组大小, 函数返回时为key数组实际大小
);

typedef struct service_27_ac {
    DMKey key_callback;
} service_27_ac;

typedef void (*service_variable_byte_callback)(void *item, ByteArray *variable_byte, void *arg);

typedef void (*service_response_callback)(void *item, const INT8U *data, INT32U len, void *arg);

/* 这个结构体内的变量值都是在添加诊断服务项时初始化赋值的，后续执行过程中不修改其中变量值 */
typedef struct service_item {
    BOOLEAN isenable;
    INT8U sid; /* 诊断服务ID */
    INT8U sub; /* 诊断服务子功能，option */
    INT16U did; /* 数据标识符，option */
    INT32 delay; /* 服务处理延时时间，延时时间到达后才会处理这条服务项 unit ms */
    INT32 timeout; /* 应答等待超时时间 unit ms */
    BOOLEAN issuppress; /* 是否是抑制响应 */
    INT32U tatype; /* 目的地址类型 */
    INT32U ta; /* 诊断目的地址 */
    INT32U sa; /* 诊断源地址 */
    ByteArray *request_byte; /* 请求数据 */
    ByteArray *variable_byte; /* 非固定格式的数据，比如31 2e服务后面不固定的数据 */
    service_variable_byte_callback vb_callback; /* 在构建请求数据时候调用 */
    void *vb_callback_arg;
    service_response_callback response_callback; /* 接收到应答的时候调用 */
    void *response_callback_arg;
    service_36_td td_36; /* 36服务配置,可选数据 */
    service_34_rd rd_34; /* 34服务配置,可选数据 */
    service_38_rft rft_38; /* 38服务配置,可选数据 */
    service_27_ac ac_27;
    
    serviceResponseExpect response_rule; /* 预期响应规则 */ 
    ByteArray *expect_byte; /* 预期响应字符 */

    serviceFinishCondition finish_rule; /* 结束规则 */
    ByteArray *finish_byte; /* 结束响应字符 */
    INT32U finish_try_num; /* 结束条件检测次数 */
    INT32U finish_num_max; /* 结束最大尝试次数，到了这个次数不论是否符合结束条件都会结束这个诊断项 */
    INT32U finish_time_max; /* 结束最大等待时间，到了这个时间不论是否符合结束条件都会结束这个诊断项 */

    INT32U rr_callid; /* ota master api 端的请求结果回调函数ID，大于0才有效 */
    char *desc; /* 诊断服务项描述信息 */
} service_item;

/* 诊断发送请求回调函数 */
typedef INT32 (*udsc_request_sent_callback)(INT16U id, void *arg, const INT8U *data, INT32U size, INT32U sa, INT32U ta, INT32U tatype);

typedef enum udsc_finish_stat {
    UDSC_NORMAL_FINISH = 0, /* 正常结束 */
    UDSC_SENT_ERROR_FINISH, /* 发送请求错误结束 */
    UDSC_UNEXPECT_RESPONSE_FINISH, /* 非预期应答错误结束 */
    UDSC_TIMEOUT_RESPONSE_FINISH, /* 超时应答错误结束 */
} udsc_finish_stat;
/* 诊断任务结束后调用 */
typedef void (*udsc_services_finish_callback)(void *udsc, udsc_finish_stat stat, void *arg/* 用户数据指针 */, const INT8U *ind, INT32U indl, const INT8U *resp, INT32U respl);

/* 用于生成种子 */
typedef void (*service_sa_seed_callback)(void *arg/* 用户数据指针 */, INT16U id, INT8U level, INT8U *seed, INT16U seed_size);

typedef struct uds_client {
    BOOLEAN isidle; /* 是否被使用 */
    INT16U id; /* uds客户端ID */
/* -------------------------------------------------------------- */
#define IPC_CAMMAND_VALID_TIME (150) /* unit */
    INT32U cmd_valid_time; /* 命令有效时间,收发端超时的命令造成的异常 */
/* -------------------------------------------------------------- */
#ifdef __HAVE_LIBEV__
    struct ev_loop *loop; /* 主事件循环结构体 */
    ev_timer request_watcher; /* 诊断任务请求处理定时器 */
    ev_timer response_watcher; /* 诊断任务应答超时定时器 */
    ev_timer testerPresent_watcher; /* 诊断仪在线请求定时器 */
#endif /* __HAVE_LIBEV__ */
#ifdef __HAVE_LIBUV__
    uv_loop_t* loop; /* 主事件循环结构体 */
    uv_timer_t request_watcher; /* 诊断任务请求处理定时器 */
    uv_timer_t response_watcher; /* 诊断任务应答超时定时器 */
    uv_timer_t testerPresent_watcher; /* 诊断仪在线请求定时器 */
#endif /* __HAVE_LIBUV__ */
    BOOLEAN isFailAbort; /* 诊断项任务处理发生错误后时候中止执行服务表项任务 */
    BOOLEAN response_meet_expect; /* 运行过程中标志位，应答符合预期要求 */
    INT32 sindex; /* 当前进行处理的诊断服务项索引 service_items[sindex] */
    BOOLEAN iskeep; /* 是否继续执行当前诊断服务项，默认不继续支持当前项，在运行过程中确定 */
    INT32U service_cnt; /* 诊断服务数量 */
#define SERVICE_ITEM_SIZE_MAX (1000)
    INT32U service_size; /* service_items 的大小 */
    service_item **service_items; /* 诊断服务处理列表 */
    void *sent_arg;
    udsc_request_sent_callback sent_callback; /* 诊断消息发送函数，在需要发送诊断请求的时候调用 */
    void *finish_arg;
    udsc_services_finish_callback finish_callback; /* 诊断客户端结束调用 */
    service_sa_seed_callback saseed_callback; /* 在需要生成key的时候回调这个函数 */
    void *saseed_arg;
    /* 用于诊断服务项之间共享数据 */
    struct common_variable {
        INT8U fileTransferCount; /* 36服务传输序列号 */
        INT32U fileTransferTotalSize; /* 36服务传输文件总字节数 */
        INT32U fileSize; /* 文件大小 */
        INT8U *td_buff; /* 36服务传输buff */
        void *filefd; /* 打开的文件描述符34 36 37 38 服务使用 */
        INT32U maxNumberOfBlockLength; /* 最大传输块长度 */
        ByteArray *seed_byte;
        ByteArray *key_byte; /* 保存由seed的生成的key */
        INT16U p2_server_max;
        INT16U p2_e_server_max;
    } common_var;

    /* 诊断仪在线请求 */
    BOOLEAN tpEnable; /* 使能诊断仪在线请求报文 */
    BOOLEAN isTpRefresh; /* 是否被UDS报文刷新定时器 */
    INT32U tpInterval; /* 发送间隔 unit ms */
    INT32U tpta; /* 诊断目的地址 */
    INT32U tpsa; /* 诊断源地址 */ 
    void *doipc_channel; /* doip传输通道 */
} uds_client;

/*
    函数内部不检查 uds_client * 和 service_item * 指针是否为空，在传参前检查
*/

/*
    创建一个UDS 客户端，多ECU交互时可以创建多个ECU客户端
*/
extern uds_client *dm_udsc_create();

/*
    调用这个函数前，确保没有事件循环和没有诊断业务在执行
    销毁UDS 客户端，在有诊断任务执行或者事件循环没有停止的时候返回错误
*/
extern BOOLEAN dm_udsc_destory(uds_client *udsc);

/*
    事件循环，需要循环调用这个函数才能进行事件循环
    和 dm_udsc_thread_loop_start 二选一即可
*/
extern BOOLEAN dm_udsc_event_loop_start(uds_client *udsc);

/*
    在线程内进行事件循环，只用调用一次就行，需要关注线程资源互斥问题
    和 dm_udsc_event_loop_start 二选一即可
*/
extern BOOLEAN dm_udsc_thread_loop_start(uds_client *udsc);

/*
    停止事件循环
*/
extern BOOLEAN dm_udsc_loop_stop(uds_client *udsc);

/*
    是否有诊断任务在执行中
*/
extern BOOLEAN dm_udsc_service_isactive(uds_client *udsc);

/*
    开始执行诊断任务
*/
extern BOOLEAN dm_udsc_services_start(uds_client *udsc);

/*
    停止执行诊断任务
*/
extern BOOLEAN dm_udsc_services_stop(uds_client *udsc);

/*
    设置诊断项执行出错是否终止执行诊断任务
*/
extern void dm_udsc_service_fail_abort(uds_client *udsc, BOOLEAN b);

/*
    增加一个诊断服务项，已经加入到诊断服务执行表内
*/
extern service_item *dm_udsc_service_item_add(uds_client *udsc, char *desc);

/*
    从诊断服务执行表内删除一个诊断服务项，后续不能再使用了
*/
extern void dm_udsc_service_item_del(uds_client *udsc, service_item *item);

/*
    根据结构体内的配置生成请求报文
*/
extern BOOLEAN dm_udsc_service_request_build(service_item *sitem);

/*
    设置诊断请求发送回调函数
*/
extern void dm_udsc_request_sent_callback_set(uds_client *udsc, udsc_request_sent_callback sent_callback, void *arg);

/*
    设置所有诊断服务执行结束后的回调处理函数
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
