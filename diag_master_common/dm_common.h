#ifndef __DM_COMMON_H__
#define __DM_COMMON_H__

// #define __HAVE_SHARED_MEMORY__ /* ota master 和 ota master api之间是否使用共享内存通信 */

#define DM_UDSC_CAPACITY_MAX (1000) /* 最多支持创建多少个UDS客户端 */
#define DM_DOIPC_CAPACITY_MAX (DM_UDSC_CAPACITY_MAX) /* 最多支持创建多少个DOIP客户端 */

#ifdef __linux__
#define DM_UNIX_SOCKET_PATH_PREFIX "/tmp/dm_uds_"
#define DMS_UNIX_SOCKET_PATH "/tmp/dms_uds"
#define DM_API_UNIX_SOCKET_PATH_PREFIX "/tmp/dm_api_uds_"
#endif /* __linux__ */

#ifdef _WIN32
#define DMS_UDS_SOCKET_PATH "127.0.0.1"
#define DMS_UDS_SOCKET_PORT (37777)
#define DMS_UDS_SOCKET_PORT_BASE (38888)

#define DM_UDS_SOCKET_PATH "127.0.0.1"
#define DM_UDS_SOCKET_PORT_BASE (47777)
#endif /* _WIN32S */

#define DM_SHM_KEY (0x4f4d) /* 共享内存key */
#define DM_SHM_SIZE (4096 * 1000) /* 共享内存大小 */

#ifdef DEBUG

#else /* DEBUG */
#include <sys/timeb.h>
#endif /* DEBUG */
#ifdef _WIN32
 
#else /* _WIN32 */
#include <pthread.h>
#endif /* _WIN32 */
enum log_level_e {
    LOG_LEVEL_NON,
    LOG_LEVEL_ERR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_MAX,
};
#define LOG_LEVEL (LOG_LEVEL_DEBUG)

#ifdef _WIN32 
#include <winternl.h>  
#define FFILENAME(x) strrchr(x, '\\')?strrchr(x, '\\')+1:x
#define log_printf(desc, level, fmt, ...) do {\
            if (level > LOG_LEVEL) { \
                break; \
            }\
            SYSTEMTIME st;  \
            GetSystemTime(&st); \
            printf("[%04d/%02d/%02d %02d:%02d:%02d]", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);\
            printf("[%s]", desc); \
            printf("[%s:%d]", FFILENAME(__FILE__), __LINE__); \
            printf(fmt, ## __VA_ARGS__); \
        } while (0)
#endif /* _WIN32 */
#ifdef __linux__
#define log_printf(desc, level, fmt, ...) do {\
            if (level > LOG_LEVEL) { \
                break; \
            }\
            struct timeb tTimeB; \
            ftime(&tTimeB); \
            struct tm* tTM = localtime(&tTimeB.time); \
            printf("[%s][%04d/%02d/%02d %02d:%02d:%02d.%03d] ", desc, tTM->tm_year + 1900, \
            tTM->tm_mon + 1, tTM->tm_mday, tTM->tm_hour, tTM->tm_min, tTM->tm_sec, \
            tTimeB.millitm); \
            printf("[%s:%d]", __FILE__, __LINE__); \
            printf(fmt, ## __VA_ARGS__); \
        } while (0)
#endif /* __linux__ */

#define log_e(fmt, ...) log_printf("ERROR", LOG_LEVEL_ERR, fmt, ##__VA_ARGS__)
#define log_w(fmt, ...) log_printf("WARNING", LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define log_d(fmt, ...) log_printf("DEBUG", LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)

#define log_hex_printf(desc, level, describe, msg, msglen) do { \
        if (level > LOG_LEVEL) { \
            break; \
        }\
        if ((msglen) > 10240) { \
            log_printf(desc, level, "Messages that are too long are not dumped. \n"); \
            break; \
        } \
        int nn = 0, pp = 0; \
        char hexstr[1024] = {0}; \
        for (; nn < (msglen); nn++, pp++) { \
            if (pp >= 300) {\
                log_printf(desc, level, "%s(%d-%d/%d):%s \n", (char *)describe, nn - pp + 1, nn, (int)(msglen), hexstr);\
                pp = 0;\
                memset(hexstr, 0, 1024); \
            }\
            snprintf(hexstr + (pp * 3), 1024 - (pp * 3), "%02X ", (unsigned char)msg[nn]); \
        } \
        log_printf(desc, level, "%s(%d-%d/%d):%s \n", describe, nn - pp + 1, nn, (int)(msglen), (char *)hexstr);\
    } while(0)
#define log_hex_e(describe, msg, msglen) log_hex_printf("ERROR", LOG_LEVEL_ERR, describe, msg, msglen)
#define log_hex_w(describe, msg, msglen) log_hex_printf("WARNING", LOG_LEVEL_WARN, describe, msg, msglen)
#define log_hex_d(describe, msg, msglen) log_hex_printf("DEBUG", LOG_LEVEL_DEBUG, describe, msg, msglen)

/* ------------------------------------------------------------------------------------------------------ */
typedef struct ByteArray {
    unsigned char *data;
    unsigned int dlen;
    unsigned int size;
} ByteArray;

/* -----------------------------------------诊断服务配置中的关键字---------------------------------------- */
#define C_KEY_SI_ID                         "ServiceID"
#define C_KEY_SI_ID_38_FRT                  "RequestFileTransfer"
#define C_KEY_SI_ID_38_MOO                  "ModeOfOperation"
#define C_KEY_SI_ID_38_FPANL                "FilePathAndNameLength"
#define C_KEY_SI_ID_38_FPAN                 "FilePathAndName"
#define C_KEY_SI_ID_38_DFI                  "DataFormatIdentifier"
#define C_KEY_SI_ID_38_FSPL                 "fileSizeParameterLength"
#define C_KEY_SI_ID_38_FSUC                 "FileSizeUnCompressed"
#define C_KEY_SI_ID_38_FSC                  "FileSizeCompressed"
#define C_KEY_SI_ID_34_RD                   "RequestDownload"
#define C_KEY_SI_ID_34_DFI                  "DataFormatIdentifier"
#define C_KEY_SI_ID_34_AALFI                "AddressAndLengthFormatIdentifier"
#define C_KEY_SI_ID_34_MA                   "MemoryAddress"
#define C_KEY_SI_ID_34_MS                   "MemorySize"
#define C_KEY_SI_SUB                        "SubFunction"
#define C_KEY_SI_DID                        "DataIdentifier"
#define C_KEY_SI_ITEM_DELAY_TIME            "DelayTime"
#define C_KEY_SI_TIMEOUT                    "Timeout"
#define C_KEY_SI_IS_SUPPRESS                "IsSuppress"
#define C_KEY_SI_TA_TYPE                    "TaType"
#define C_KEY_SI_TA                         "Ta" 
#define C_KEY_SI_SA                         "Sa"
#define C_KEY_SI_REQUEST_BYTE               "RequestByte"
#define C_KEY_SI_VARIABLE_BYTE              "VariableByte" 
#define C_KEY_SI_EXPECT_RESPONSE_BYTE       "ExpectResponseByte"
#define C_KEY_SI_EXPECT_RESPONSE_RULE       "ExpectResponseRule"
#define C_KEY_SI_FINISH_RESPONSE_BYTE       "FinishByte"
#define C_KEY_SI_FINISH_RESPONSE_RULE       "FinishRule"
#define C_KEY_SI_FINISH_RESPONSE_TIMEOUT    "FinishTimeout"
#define C_KEY_SI_FINISH_RESPONSE_TRYMAX     "FinishTryMax"
#define C_KEY_SI_DESC                       "ServiceDesc"
#define C_KEY_SI_LOCAL_FILE_PATH            "LocalFilePath"
#define C_KEY_SI_REQUEST_RESULT_CALL_ID     "RequestResultCallId"
#define C_KEY_SI_MAX_NUMBER_OF_BLOCK_LEN    "MaxNumberOfBlockLength"

typedef enum serviceResponseExpect {
    NOT_SET_RESPONSE_EXPECT = 0, /* 未设置预期响应 */
    POSITIVE_RESPONSE_EXPECT, /* 预期正响应 */
    NEGATIVE_RESPONSE_EXPECT, /* 预期负响应 */
    MATCH_RESPONSE_EXPECT, /* 校验预期响应 */
    NO_RESPONSE_EXPECT, /* 预期无响应 */
} serviceResponseExpect;

typedef enum serviceFinishCondition {
    FINISH_DEFAULT_SETTING = 0, /* 默认设置 */
    FINISH_EQUAL_TO, /* 等于结束响应字符 */
    FINISH_UN_EQUAL_TO, /* 不等于结束响应字符 */
} serviceFinishCondition;
    
typedef struct service_config {
    unsigned char sid; /* 诊断服务ID */
    unsigned char sub; /* 诊断子功能 */
    unsigned short did; /* 诊断数据标识符 */
    unsigned int delay; /* 诊断服务请求前的延时时间 */
    unsigned int timeout; /* 诊断服务响应超时时间 */
    unsigned char issuppress; /* 是否是抑制响应 */
    unsigned int tatype; /* 目标地址类型 */
    unsigned int ta; /* 目标地址 */
    unsigned int sa; /* 源地址 */
    ByteArray *variableByte; /* 诊断服务中的可变数据，UDS 客户端将根据 sid sub did和这个自动构建UDS请求数据 */
    ByteArray *expectResponByte; /* 预期诊断响应数据，用于判断当前诊断服务执行是否符合预期 */
    unsigned int expectRespon_rule; /* 预期响应规则 */
    ByteArray *finishByte;  /* 响应结束匹配数据，用于判断当前诊断服务是否需要重复执行 */
    unsigned int finish_rule; /* 响应结束规则 */
    unsigned int finish_num_max; /* 响应结束最大匹配次数 */
    unsigned int rr_callid; /* 请求结果回调函数的ID, 大于0才是有效值 */
    struct  {
        unsigned char dataFormatIdentifier;
        unsigned char addressAndLengthFormatIdentifier;
        unsigned int memoryAddress;
        unsigned int memorySize;
    } service_34_rd;
    struct  {
        unsigned char modeOfOperation;
        unsigned short filePathAndNameLength;
        char filePathAndName[256];
        unsigned char dataFormatIdentifier;
        unsigned char fileSizeParameterLength;
        unsigned int fileSizeUnCompressed;
        unsigned int fileSizeCompressed;
    } service_38_rft;
    unsigned int maxNumberOfBlockLength;
    char local_path[256]; /* 本地刷写文件路径 */
    char desc[256]; /* 诊断服务项的描述信息 */
} service_config;

#define C_KEY_GC_IS_FAILABORT             "IsFailAbort"
#define C_KEY_GC_TP_ENABLE                "TesterPresentEnable"
#define C_KEY_GC_TP_IS_REFRESH            "TesterPresentIsRefresh"
#define C_KEY_GC_TP_INTERVAL              "TesterPresentInterval"
#define C_KEY_GC_TP_TA                    "TesterPresentTa"
#define C_KEY_GC_TP_SA                    "TesterPresentSa"

typedef struct udsc_general_config {
    unsigned char isFailAbort; /* 诊断项任务处理发生错误后时候中止执行服务表项任务 */
    unsigned char tpEnable; /* 使能诊断仪在线请求报文 */
    unsigned char isTpRefresh; /* 是否被UDS报文刷新定时器 */
    unsigned int tpInterval; /* 发送间隔 unit ms */
    unsigned int tpta; /* 诊断目的地址 */
    unsigned int tpsa; /* 诊断源地址 */  
} udsc_general_config;

/* ------------------------------------------------------------------------------------------------------- */
/* ---------------------------------------------响应错误码------------------------------------------------ */
#define DM_CAMMAND_ERR_NO                   (0)
#define DM_CAMMAND_ERR_SERVICE_REQUEST      (1) /* 诊断请求发送失败 */
#define DM_CAMMAND_ERR_START_SCRIPT_FAILED  (2) /* 启动诊断服务脚本失败 */
#define DM_CAMMAND_ERR_NOT_FOUND_FILE       (3) /* 未发现脚本文件 */
#define DM_CAMMAND_ERR_UNABLE_PARSE_FILE    (4) /* 无法解析脚本文件 */
#define DM_CAMMAND_ERR_UNABLE_PARSE_CONFIG  (5) /* 无法解析脚本配置 */
#define DM_CAMMAND_ERR_NOT_FOUND_SCRIPT     (6) /* 未发现脚本 */
#define DM_CAMMAND_ERR_UDSC_CREATE          (7) /* uds客户端创建失败 */
#define DM_CAMMAND_ERR_UDSC_INVALID         (8) /* 诊断客户端ID无效 */
#define DM_CAMMAND_ERR_UDSC_RUNING          (9) /* udsc客户端运行中无法销毁 */
#define DM_CAMMAND_ERR_SHM_OUTOF            (10) /* 共享内存空间不足 */
#define DM_CAMMAND_ERR_REPLY_TIMEOUT        (11) /* 应答超时 */
#define DM_CAMMAND_ERR_UDS_RESPONSE_TIMEOUT (12) /* 诊断应答超时 */
#define DM_CAMMAND_ERR_UDS_RESPONSE_UNEXPECT (13) /* 诊断非预期响应 */
#define DM_CAMMAND_ERR_UDS_RESPONSE_OVERLEN  (14) /* 诊断结果长度过长，超过Buff长度 */
#define DM_CAMMAND_ERR_UDS_SERVICE_ADD       (15) /* UDS服务项添加失败 */
#define DM_CAMMAND_ERR_DIAG_MASTER_MAX        (16) /* 连接OTA MASTER错误，ota master达到最大接入数量 */
#define DM_CAMMAND_ERR_OMAPI_UNKNOWN         (17) /* 连接OTA MASTER错误，未知ota master api */
#define DM_CAMMAND_ERR_DIAG_MASTER_CREATE     (18) /* 连接OTA MASTER错误，ota master创建失败 */
#define DM_CAMMAND_ERR_DOIPC_INVALID         (19) /* doip客户端ID无效 */
#define DM_CAMMAND_ERR_DOIPC_CREATE_FAILED   (20) /* doip客户端创建失败 */
/* ------------------------------------------------------------------------------------------------------- */
/* ----------------------------------diag_master 和diag_master_api的交互命令--------------------------------- */
#define DM_CMD_REPLY_MASK (0x40) /* 响应数据掩码 */

/* diag_master 发送诊断请求 */
#define DM_CAMMAND_SERVICE_INDICATION_REQUEST  (1) /* diag_master -> diag_master_api  */
#define DM_CAMMAND_SERVICE_INDICATION_REPLY    (DM_CAMMAND_SERVICE_INDICATION_REQUEST | DM_CMD_REPLY_MASK) /* diag_master_api -> diag_master  */

/* diag_master_api 发送诊断应答请求 */
#define DM_CAMMAND_SERVICE_RESPONSE_REQUEST    (2) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_SERVICE_RESPONSE_REPLY      (DM_CAMMAND_SERVICE_RESPONSE_REQUEST | DM_CMD_REPLY_MASK)  /* diag_master -> diag_master_api  */

/* diag_master_api 控制 diag_master 启动诊断脚本 */
#define DM_CAMMAND_START_SCRIPT_REQUEST        (3) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_START_SCRIPT_REPLY          (DM_CAMMAND_START_SCRIPT_REQUEST | DM_CMD_REPLY_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api 控制 diag_master 停止诊断脚本 */
#define DM_CAMMAND_STOP_SCRIPT_REQUEST         (4) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_STOP_SCRIPT_REPLY           (DM_CAMMAND_STOP_SCRIPT_REQUEST | DM_CMD_REPLY_MASK) /* diag_master -> diag_master_api  */

/* diag_master 通知 diag_master_api 诊断任务执行结果 */
#define DM_CAMMAND_SCRIPT_RESULT_REQUEST       (5) /* diag_master -> diag_master_api  */
#define DM_CAMMAND_SCRIPT_RESULT_REPLY         (DM_CAMMAND_SCRIPT_RESULT_REQUEST | DM_CMD_REPLY_MASK) /* diag_master_api -> diag_master  */

/* diag_master_api 配置诊断脚本 */
#define DM_CAMMAND_CONFIG_SCRIPT_REQUEST       (6) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_CONFIG_SCRIPT_REPLY         (DM_CAMMAND_CONFIG_SCRIPT_REQUEST | DM_CMD_REPLY_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api 让 diag_master直接读脚本文件 */
#define DM_CAMMAND_SCRIPT_PATH_REQUEST         (7) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_SCRIPT_PATH_REPLY           (DM_CAMMAND_SCRIPT_PATH_REQUEST | DM_CMD_REPLY_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api 让 diag_master 创建 uds客户端 */
#define DM_CAMMAND_UDSC_CREATE_REQUEST         (8) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_UDSC_CREATE_REPLY           (DM_CAMMAND_UDSC_CREATE_REQUEST | DM_CMD_REPLY_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api 让 diag_master 销毁 uds客户端 */
#define DM_CAMMAND_UDSC_DESTORY_REQUEST        (9) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_UDSC_DESTORY_REPLY          (DM_CAMMAND_UDSC_DESTORY_REQUEST | DM_CMD_REPLY_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api 请求 diag_master 复位 */
#define DM_CAMMAND_DIAG_MASTER_RESET_REQUEST    (10) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_DIAG_MASTER_RESET_REPLY      (DM_CAMMAND_DIAG_MASTER_RESET_REQUEST | DM_CMD_REPLY_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api 请求 diag_master 配置通用项 */
#define DM_CAMMAND_OTA_GENERAL_CONFIG_REQUEST  (11) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_OTA_GENERAL_CONFIG_REPLY    (DM_CAMMAND_OTA_GENERAL_CONFIG_REQUEST | DM_CMD_REPLY_MASK) /* diag_master -> diag_master_api  */

/* diag_master 通知 diag_master_api 诊断请求结果 */
#define DM_CAMMAND_SERVICE_REQUEST_RESULT_REQUEST (12) /* diag_master -> diag_master_api  */
#define DM_CAMMAND_SERVICE_REQUEST_RESULT_REPLY (DM_CAMMAND_SERVICE_REQUEST_RESULT_REQUEST | DM_CMD_REPLY_MASK) /* diag_master_api -> diag_master  */

/* diag_master发送种子给diag_master_api生成key */
#define DM_CAMMAND_SERVICE_SA_SEED_REQUEST (13) /* diag_master -> diag_master_api  */
#define DM_CAMMAND_SERVICE_SA_SEED_REPLY (DM_CAMMAND_SERVICE_SA_SEED_REQUEST | DM_CMD_REPLY_MASK) /* diag_master_api -> diag_master  */

/* diag_master_api将生成的key发送给diag_master */
#define DM_CAMMAND_SERVICE_SA_KEY_REQUEST (14) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_SERVICE_SA_KEY_REPLY (DM_CAMMAND_SERVICE_SA_KEY_REQUEST | DM_CMD_REPLY_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api发起连接ota master的请求 */
#define DM_CAMMAND_CONNECT_DIAG_MASTER_REQUEST (15) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_CONNECT_DIAG_MASTER_REPLY (DM_CAMMAND_CONNECT_DIAG_MASTER_REQUEST | DM_CMD_REPLY_MASK) /* diag_master -> diag_master_api  */

/* ota master发送心跳保活以确认diag_master_api是否在线 */
#define DM_CAMMAND_OMAPI_KEEPALIVE_REQUEST (16) /* diag_master -> diag_master_api  */
#define DM_CAMMAND_OMAPI_KEEPALIVE_REPLY (DM_CAMMAND_OMAPI_KEEPALIVE_REQUEST | DM_CMD_REPLY_MASK) /* diag_master_api -> diag_master  */

/* diag_master_api请求创建DOIP客户端 */
#define DM_CAMMAND_DOIPC_CREATE_REQUEST (17) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_DOIPC_CREATE_REPLY (DM_CAMMAND_DOIPC_CREATE_REQUEST | DM_CMD_REPLY_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api请求doip通道绑定uds客户端 */
#define DM_CAMMAND_DOIPC_BIND_UDSC_REQUEST (17) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_DOIPC_BIND_UDSC_REPLY (DM_CAMMAND_DOIPC_BIND_UDSC_REQUEST | DM_CMD_REPLY_MASK) /* diag_master -> diag_master_api  */

#define DM_CAMMAND_MAX (18) /* 命令最大值 */
/* ------------------------------------------------------------------------------------------------------- */

#define DIAGNOSTIC_TYPE_CAN (1)
#define DIAGNOSTIC_TYPE_DOIP (2)
#define DIAGNOSTIC_TYPE_REMOTE (3)
#define DIAGNOSTIC_TYPE_RUNTIME_DBG (4)

#define PHYSICAL_ADDRESS (1)
#define FUNCTION_ADDRESS (2)

/* 进程间交互数据结构定义，直接定义各个变量在结构体内的偏移量，便于直接访问共享内存中的数据，减少数据拷贝动作 */
/* ------------------------------------------------------------------------------------------------------- */
/* struct ipc_common_msg */
#define DM_IPC_COMMON_CMD_OFFSET (0)
#define DM_IPC_COMMON_CMD_SIZE   (4)

#define DM_IPC_COMMON_RECODE_OFFSET (DM_IPC_COMMON_CMD_OFFSET + DM_IPC_COMMON_CMD_SIZE)
#define DM_IPC_COMMON_RECODE_SIZE   (4) 

#define DM_IPC_COMMON_UDSC_ID_OFFSET (DM_IPC_COMMON_RECODE_OFFSET + DM_IPC_COMMON_RECODE_SIZE)
#define DM_IPC_COMMON_UDSC_ID_SIZE   (2)

#define DM_IPC_COMMON_TV_SEC_OFFSET (DM_IPC_COMMON_UDSC_ID_OFFSET + DM_IPC_COMMON_UDSC_ID_SIZE)
#define DM_IPC_COMMON_TV_SEC_SIZE   (4)

#define DM_IPC_COMMON_TV_USEC_OFFSET (DM_IPC_COMMON_TV_SEC_OFFSET + DM_IPC_COMMON_TV_SEC_SIZE)
#define DM_IPC_COMMON_TV_USEC_SIZE   (4)

#define DM_IPC_COMMON_MSG_SIZE (DM_IPC_COMMON_CMD_SIZE + \
                                DM_IPC_COMMON_RECODE_SIZE + \
                                DM_IPC_COMMON_UDSC_ID_SIZE + \
                                DM_IPC_COMMON_TV_SEC_SIZE + \
                                DM_IPC_COMMON_TV_USEC_SIZE)
typedef struct ipc_common_msg {
    unsigned int cmd; /* 命令 */
    unsigned int recode; /* 命令响应码 */
    unsigned short udsc_id; /* UDS 客户端ID，用于标识是哪个UDS客户端 */
    unsigned int tv_sec; /* 时间戳 s */
    unsigned int tv_usec; /* 时间戳 us */
} ipc_common_msg;

/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
     ----------------------------------------------------------
    |       n nyte     | 4byte  |  4byte  |         4byte      |  
     ----------------------------------------------------------
    | ipc common header| shmkey | shmsize | cammand valid time | 
     ----------------------------------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define DM_IPC_UDSC_CREATE_SHMKEY_OFFSET (DM_IPC_COMMON_MSG_SIZE)
#define DM_IPC_UDSC_CREATE_SHMKEY_SIZE   (4) 

#define DM_IPC_UDSC_CREATE_SHMSIZE_OFFSET (DM_IPC_UDSC_CREATE_SHMKEY_OFFSET + DM_IPC_UDSC_CREATE_SHMKEY_SIZE)
#define DM_IPC_UDSC_CREATE_SHMSIZE_SIZE   (4)

#define DM_IPC_UDSC_CREATE_CMD_VALID_TIME_OFFSET (DM_IPC_UDSC_CREATE_SHMSIZE_OFFSET + DM_IPC_UDSC_CREATE_SHMSIZE_SIZE)
#define DM_IPC_UDSC_CREATE_CMD_VALID_TIME_SIZE (4)

#define DM_IPC_UDSC_CREATE_UDS_IND_OFFSET_OFFSET (DM_IPC_UDSC_CREATE_CMD_VALID_TIME_OFFSET + DM_IPC_UDSC_CREATE_CMD_VALID_TIME_SIZE)
#define DM_IPC_UDSC_CREATE_UDS_IND_OFFSET_SIZE (4)

#define DM_IPC_UDSC_CREATE_UDS_IND_SIZE_OFFSET (DM_IPC_UDSC_CREATE_UDS_IND_OFFSET_OFFSET + DM_IPC_UDSC_CREATE_UDS_IND_OFFSET_SIZE)
#define DM_IPC_UDSC_CREATE_UDS_IND_SIZE_SIZE (4)

#define DM_IPC_UDSC_CREATE_UDS_REP_OFFSET_OFFSET (DM_IPC_UDSC_CREATE_UDS_IND_SIZE_OFFSET + DM_IPC_UDSC_CREATE_UDS_IND_SIZE_SIZE)
#define DM_IPC_UDSC_CREATE_UDS_REP_OFFSET_SIZE (4)

#define DM_IPC_UDSC_CREATE_UDS_REP_SIZE_OFFSET (DM_IPC_UDSC_CREATE_UDS_REP_OFFSET_OFFSET + DM_IPC_UDSC_CREATE_UDS_REP_OFFSET_SIZE)
#define DM_IPC_UDSC_CREATE_UDS_REP_SIZE_SIZE (4)

#define DM_IPC_UDSC_CREATE_SIZE (DM_IPC_UDSC_CREATE_SHMKEY_SIZE + \
                                 DM_IPC_UDSC_CREATE_SHMSIZE_SIZE + \
                                 DM_IPC_UDSC_CREATE_CMD_VALID_TIME_SIZE + \
                                 DM_IPC_UDSC_CREATE_UDS_IND_OFFSET_SIZE + \
                                 DM_IPC_UDSC_CREATE_UDS_IND_SIZE_SIZE + \
                                 DM_IPC_UDSC_CREATE_UDS_REP_OFFSET_SIZE + \
                                 DM_IPC_UDSC_CREATE_UDS_REP_SIZE_SIZE)
typedef struct ipc_udsc_create_msg {
    unsigned int shmkey; /* 共享内存key */
    unsigned int shmsize; /* 共享内存大小 */    
    unsigned int cmd_valid_time; /* 命令有效时间,收发端超时的命令造成的异常 */
    unsigned int shm_udsInd_offset; /* UDS请求在共享内存中的偏移地址 */    
    unsigned int shm_udsInd_size; /* UDS请求在共享内存中大小 */
    unsigned int shm_udsRep_offset; /* UDS应答在共享内存中的偏移地址 */    
    unsigned int shm_udsRep_size; /* UDS应答在共享内存中大小 */
} ipc_udsc_create_msg;


/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    --------------------------------------------------------------------------------------------------------------------------
   |       n nyte     |       4byte         |     4byte      |      4byte     |  4byte  |        4byte      |      nbyte      |
    --------------------------------------------------------------------------------------------------------------------------
   | ipc common header| diag type ip or can | target address | source address | ta type | request data len  |  request data   |
    --------------------------------------------------------------------------------------------------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
/* struct service_indication_s */
#define SERVICE_INDICATION_MTYPE_OFFSET (0)
#define SERVICE_INDICATION_MTYPE_SIZE (4)

#define SERVICE_INDICATION_A_SA_OFFSET (SERVICE_INDICATION_MTYPE_OFFSET + SERVICE_INDICATION_MTYPE_SIZE)
#define SERVICE_INDICATION_A_SA_SIZE (4)

#define SERVICE_INDICATION_A_TA_OFFSET (SERVICE_INDICATION_A_SA_OFFSET + SERVICE_INDICATION_A_SA_SIZE)
#define SERVICE_INDICATION_A_TA_SIZE (4)

#define SERVICE_INDICATION_TA_TYPE_OFFSET (SERVICE_INDICATION_A_TA_OFFSET + SERVICE_INDICATION_A_TA_SIZE)
#define SERVICE_INDICATION_TA_TYPE_SIZE (4)

#define SERVICE_INDICATION_PAYLOAD_LEN_OFFSET (SERVICE_INDICATION_TA_TYPE_OFFSET + SERVICE_INDICATION_TA_TYPE_SIZE)
#define SERVICE_INDICATION_PAYLOAD_LEN_SIZE (4)

#define SERVICE_INDICATION_PAYLOAD_OFFSET (SERVICE_INDICATION_PAYLOAD_LEN_OFFSET + SERVICE_INDICATION_PAYLOAD_LEN_SIZE)

typedef struct service_indication_s {
    unsigned int Mtype;
    unsigned int A_SA;
    unsigned int A_TA;
    unsigned int TA_TYPE;    
    int payload_length;
    unsigned char payload[0];
} service_indication;

/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    --------------------------------------------------------------------------------------------------------------------------
   |       n nyte     |       4byte         |     4byte      |      4byte     |  4byte  |        4byte      |      nbyte      |
    --------------------------------------------------------------------------------------------------------------------------
   | ipc common header| diag type ip or can | target address | source address | ta type | response data len |  response data  |
    --------------------------------------------------------------------------------------------------------------------------
  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
/* struct service_response_s */
#define SERVICE_RESPONSE_MTYPE_OFFSET (0)
#define SERVICE_RESPONSE_MTYPE_SIZE (4)

#define SERVICE_RESPONSE_A_SA_OFFSET (SERVICE_RESPONSE_MTYPE_OFFSET + SERVICE_RESPONSE_MTYPE_SIZE)
#define SERVICE_RESPONSE_A_SA_SIZE (4)

#define SERVICE_RESPONSE_A_TA_OFFSET (SERVICE_RESPONSE_A_SA_OFFSET + SERVICE_RESPONSE_A_SA_SIZE)
#define SERVICE_RESPONSE_A_TA_SIZE (4)

#define SERVICE_RESPONSE_TA_TYPE_OFFSET (SERVICE_RESPONSE_A_TA_OFFSET + SERVICE_RESPONSE_A_TA_SIZE)
#define SERVICE_RESPONSE_TA_TYPE_SIZE (4)

#define SERVICE_RESPONSE_PAYLOAD_LEN_OFFSET (SERVICE_RESPONSE_TA_TYPE_OFFSET + SERVICE_RESPONSE_TA_TYPE_SIZE)
#define SERVICE_RESPONSE_PAYLOAD_LEN_SIZE (4)

#define SERVICE_RESPONSE_PAYLOAD_OFFSET (SERVICE_RESPONSE_PAYLOAD_LEN_OFFSET + SERVICE_RESPONSE_PAYLOAD_LEN_SIZE)

typedef struct service_response_s {
    unsigned int Mtype;
    unsigned int A_SA;
    unsigned int A_TA;
    unsigned int TA_TYPE;    
    int payload_length;
    unsigned char payload[0];
} service_response;
/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    --------------------------------------------------------------------
   |       n nyte     |       4byte      |     4byte       |    nbyte   |
    --------------------------------------------------------------------
   | ipc common header| callback func id | result data len | result data| 
    --------------------------------------------------------------------
  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define SERVICE_REQUEST_RESULT_RR_CALLID_OFFSET (DM_IPC_COMMON_MSG_SIZE)
#define SERVICE_REQUEST_RESULT_RR_CALLID_SIZE (4)

#define SERVICE_REQUEST_RESULT_DATA_LEN_OFFSET (SERVICE_REQUEST_RESULT_RR_CALLID_OFFSET + SERVICE_REQUEST_RESULT_RR_CALLID_SIZE)
#define SERVICE_REQUEST_RESULT_DATA_LEN_SIZE (4)

#define SERVICE_REQUEST_RESULT_DATA_OFFSET (SERVICE_REQUEST_RESULT_DATA_LEN_OFFSET + SERVICE_REQUEST_RESULT_DATA_LEN_SIZE)
#define SERVICE_REQUEST_RESULT_DATA_MAX (512)

#define SERVICE_REQUEST_RESULT_SIZE (SERVICE_REQUEST_RESULT_RR_CALLID_SIZE + \
                                     SERVICE_REQUEST_RESULT_DATA_LEN_SIZE)
typedef struct service_request_result_s {
    unsigned int rr_callid;
    unsigned int dlen;
    unsigned char data[0];
}service_request_result;


/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ---------------------------------------------------------------------------------------
   |       n nyte     |       4byte      |     nbyte    |        4byte     |     nbyte     |
    ------------------ ------------------ -------------- ------------------ ---------------
   | ipc common header| request data len | request data | response data len| response data |
    ---------------------------------------------------------------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define SERVICE_FINISH_RESULT_IND_LEN_OFFSET (DM_IPC_COMMON_MSG_SIZE)
#define SERVICE_FINISH_RESULT_IND_LEN_SIZE (4)

#define SERVICE_FINISH_RESULT_IND_OFFSET (SERVICE_FINISH_RESULT_IND_LEN_OFFSET + SERVICE_FINISH_RESULT_IND_LEN_SIZE)

#define SERVICE_FINISH_RESULT_RESP_LEN_SIZE (4)
typedef struct service_finish_result_s {
    unsigned int indl;
    unsigned char ind[1];    
    unsigned int respl;
    unsigned char resp[1];
}service_finish_result_t;

/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    --------------------------------------------------------
   |       n nyte     |   1byte  | 2byte  | nbyte           |
    ------------------ ---------- -------- -----------------
   | ipc common header| SA level |seed len| seed data array |
    --------------------------------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define SERVICE_SA_SEED_LEVEL_OFFSET (DM_IPC_COMMON_MSG_SIZE)
#define SERVICE_SA_SEED_LEVEL_SIZE (1)

#define SERVICE_SA_SEED_SIZE_OFFSET (SERVICE_SA_SEED_LEVEL_OFFSET + SERVICE_SA_SEED_LEVEL_SIZE)
#define SERVICE_SA_SEED_SIZE_SIZE (2)

#define SERVICE_SA_SEED_OFFSET (SERVICE_SA_SEED_SIZE_OFFSET + SERVICE_SA_SEED_SIZE_SIZE)
typedef struct service_sa_seed_s {
    unsigned short seed_size;
    unsigned char seed[0];
}service_sa_seed_t;

/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    --------------------------------------------------------
   |       n nyte     |   1byte  | 2byte  | nbyte           |
    ------------------ ---------- -------- -----------------
   | ipc common header| SA level |key len | key data array  |
    --------------------------------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define SERVICE_SA_KEY_LEVEL_OFFSET (DM_IPC_COMMON_MSG_SIZE)
#define SERVICE_SA_KEY_LEVEL_SIZE (1)

#define SERVICE_SA_KEY_SIZE_OFFSET (SERVICE_SA_KEY_LEVEL_OFFSET + SERVICE_SA_KEY_LEVEL_SIZE)
#define SERVICE_SA_KEY_SIZE_SIZE (2)

#define SERVICE_SA_KEY_OFFSET (SERVICE_SA_KEY_SIZE_OFFSET + SERVICE_SA_KEY_SIZE_SIZE)
typedef struct service_sa_key_s {
    unsigned short key_size;
    unsigned char key[0];
}service_sa_key_t;

/* ota master api请求连接ota master */
/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    -------------------------------------------------------------------------------------------------
   |       n nyte     |         4byte      |            4byte             |     nbyte                |
    ------------------ -------------------- ------------------------------ --------------------------
   | ipc common header| keepalive interval | ota master api unix path len | ota master api unix path | 
    -------------------------------------------------------------------------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define OMREQ_CONNECT_KEEPALIVE_INTERVAL_OFFSET (DM_IPC_COMMON_MSG_SIZE)
#define OMREQ_CONNECT_KEEPALIVE_INTERVAL_SIZE (4)

#define OMREQ_CONNECT_OMAPI_PATH_LEN_OFFSET (OMREQ_CONNECT_KEEPALIVE_INTERVAL_OFFSET + OMREQ_CONNECT_KEEPALIVE_INTERVAL_SIZE)
#define OMREQ_CONNECT_OMAPI_PATH_LEN_SIZE (4)

#define OMREQ_CONNECT_OMAPI_PATH_OFFSET (OMREQ_CONNECT_OMAPI_PATH_LEN_OFFSET + OMREQ_CONNECT_OMAPI_PATH_LEN_SIZE)

#define OMREQ_CONNECT_OMAPI_MSG_MIN_SIZE (DM_IPC_COMMON_MSG_SIZE + OMREQ_CONNECT_KEEPALIVE_INTERVAL_SIZE +OMREQ_CONNECT_OMAPI_PATH_LEN_SIZE )
typedef struct diag_master_connect_request_s {
    unsigned int keepalive_inter;
    unsigned int path_len;
    char path[0];
} diag_master_connect_request_t;

/* ota master 响应ota master api的连接请求 */
/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    --------------------------------------------------------------------
   |       n nyte     |            4byte         |     nbyte            |
    ------------------ -------------------------- ----------------------
   | ipc common header| ota master unix path len | ota master unix path | 
    --------------------------------------------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define OMREP_CONNECT_DM_PATH_LEN_OFFSET (DM_IPC_COMMON_MSG_SIZE)
#define OMREP_CONNECT_DM_PATH_LEN_SIZE (4)
    
#define OMREP_CONNECT_DM_PATH_OFFSET (OMREP_CONNECT_DM_PATH_LEN_OFFSET + OMREP_CONNECT_DM_PATH_LEN_SIZE)

#define OMREQ_CONNECT_DM_MSG_MIN_SIZE (DM_IPC_COMMON_MSG_SIZE + OMREP_CONNECT_DM_PATH_LEN_SIZE)
typedef struct diag_master_connect_reply_s {
    unsigned int path_len;
    char path[0];
} diag_master_connect_reply_t;

/* ota master api请求创建doip客户端 */
/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ---------------------------------------------------------------------------------------------------
   |       n nyte     |     1byte    |     2byte   |   4byte   |     2byte   |   4byte   | 2byte       |
    ------------------ -------------- ------------- ----------- ------------- ----------- -------------
   | ipc common header| doip version | source addr | client ip | client port | server ip | server port |           
    ---------------------------------------------------------------------------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define DOIPC_CREATE_VERSION_OFFSET (DM_IPC_COMMON_MSG_SIZE)
#define DOIPC_CREATE_VERSION_SIZE (1)

#define DOIPC_CREATE_SOURCE_ADDR_OFFSET (DOIPC_CREATE_VERSION_OFFSET + DOIPC_CREATE_VERSION_SIZE)
#define DOIPC_CREATE_SOURCE_ADDR_SIZE (2)

#define DOIPC_CREATE_CLIENT_IP_OFFSET (DOIPC_CREATE_SOURCE_ADDR_OFFSET + DOIPC_CREATE_SOURCE_ADDR_SIZE)
#define DOIPC_CREATE_CLIENT_IP_SIZE (4)

#define DOIPC_CREATE_CLIENT_PORT_OFFSET (DOIPC_CREATE_CLIENT_IP_OFFSET + DOIPC_CREATE_CLIENT_IP_SIZE)
#define DOIPC_CREATE_CLIENT_PORT_SIZE (2)

#define DOIPC_CREATE_SERVER_IP_OFFSET (DOIPC_CREATE_CLIENT_PORT_OFFSET + DOIPC_CREATE_CLIENT_PORT_SIZE)
#define DOIPC_CREATE_SERVER_IP_SIZE (4)

#define DOIPC_CREATE_SERVER_PORT_OFFSET (DOIPC_CREATE_SERVER_IP_OFFSET + DOIPC_CREATE_SERVER_IP_SIZE)
#define DOIPC_CREATE_SERVER_PORT_SIZE (2)

#define DOIPC_CREATE_REQUEST_SIZE (DM_IPC_COMMON_MSG_SIZE + \
                                   DOIPC_CREATE_VERSION_SIZE + \
                                   DOIPC_CREATE_SOURCE_ADDR_SIZE + \
                                   DOIPC_CREATE_CLIENT_IP_SIZE + \
                                   DOIPC_CREATE_CLIENT_PORT_SIZE + \
                                   DOIPC_CREATE_SERVER_IP_SIZE + \
                                   DOIPC_CREATE_SERVER_PORT_SIZE)
typedef struct doipc_create_request_s {
    unsigned char ver;
    unsigned short source_addr;
    unsigned int src_ip;
    unsigned short src_port;
    unsigned int dst_ip;
    unsigned short dst_port;
} doipc_create_request_t;

/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    -----------------------------------
   |       n nyte     |     1byte      |  
    ------------------ ---------------- 
   | ipc common header| doip client id | 
    -----------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define DOIPC_CREATE_DOIP_CLIENT_ID_OFFSET (DM_IPC_COMMON_MSG_SIZE)
#define DOIPC_CREATE_DOIP_CLIENT_ID_SIZE (4)

#define DOIPC_CREATE_REPLY_SIZE (DM_IPC_COMMON_MSG_SIZE + DOIPC_CREATE_DOIP_CLIENT_ID_SIZE)
typedef struct doipc_create_reply_s {
    unsigned int id;
} doipc_create_reply_t;


ByteArray *ByteArrayNew();

void ByteArrayDelete(ByteArray *arr);

void ByteArrayClear(ByteArray *arr);

const unsigned char *ByteArrayConstData(ByteArray *arr);

int ByteArrayCount(ByteArray *arr);

void ByteArrayAppendChar(ByteArray *dest, unsigned char c);

void ByteArrayAppendArray(ByteArray *dest, ByteArray *src);

void ByteArrayAppendNChar(ByteArray *dest, unsigned char *c, unsigned int count);

int ByteArrayEqual(ByteArray *arr1, ByteArray *arr2);

int ByteArrayCharEqual(ByteArray *arr1, unsigned char *c, unsigned int count);

int dm_command_sendto_om(int sockfd, unsigned int cmd, unsigned int recode);
int dm_command_sendto_om_api(int sockfd, unsigned int cmd, unsigned int recode);
int dm_command_reply(int sockfd, unsigned int *cmd, unsigned int *recode);

int dm_sendto_master(int sockfd, unsigned char *buff, unsigned int size);
int dm_sendto_api(int sockfd, unsigned char *buff, unsigned int size);
int dm_recvfrom(int sockfd, unsigned char *buff, unsigned int size, unsigned int toms);

int dm_tv_get(unsigned int *tv_sec, unsigned int *tv_usec);
int dm_tv_currtdms(unsigned int tv_sec, unsigned int tv_usec);

int dm_common_encode(unsigned char *buff, unsigned int size, unsigned int cmd, unsigned int recode, unsigned short id, unsigned int tv_sec, unsigned int tv_usec);

int dm_common_decode(unsigned char *buff, unsigned int size,  unsigned int *cmd, unsigned int *recode, unsigned short *id, unsigned int *tv_sec, unsigned int *tv_usec);

int dm_service_config_encode(unsigned char *buff, unsigned int size, service_config *config);

int dm_service_config_decode(unsigned char *buff, unsigned int size, service_config *config);

int dm_general_config_encode(unsigned char *buff, unsigned int size, udsc_general_config *config);

int dm_general_config_decode(unsigned char *buff, unsigned int size, udsc_general_config *config);

char *dm_ipc_command_str(unsigned int cmd);
char *dm_command_rcode_str(int rcode);

#endif /* __DM_COMMON_H__ */
