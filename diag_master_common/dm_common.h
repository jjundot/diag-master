#ifndef __DM_COMMON_H__
#define __DM_COMMON_H__

// #define __HAVE_SHARED_MEMORY__ /* ota master �� ota master api֮���Ƿ�ʹ�ù����ڴ�ͨ�� */

#define DM_UDSC_CAPACITY_MAX (1000) /* ���֧�ִ������ٸ�UDS�ͻ��� */
#define DM_DOIPC_CAPACITY_MAX (DM_UDSC_CAPACITY_MAX) /* ���֧�ִ������ٸ�DOIP�ͻ��� */

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

#define DM_SHM_KEY (0x4f4d) /* �����ڴ�key */
#define DM_SHM_SIZE (4096 * 1000) /* �����ڴ��С */

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

/* -----------------------------------------��Ϸ��������еĹؼ���---------------------------------------- */
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
    NOT_SET_RESPONSE_EXPECT = 0, /* δ����Ԥ����Ӧ */
    POSITIVE_RESPONSE_EXPECT, /* Ԥ������Ӧ */
    NEGATIVE_RESPONSE_EXPECT, /* Ԥ�ڸ���Ӧ */
    MATCH_RESPONSE_EXPECT, /* У��Ԥ����Ӧ */
    NO_RESPONSE_EXPECT, /* Ԥ������Ӧ */
} serviceResponseExpect;

typedef enum serviceFinishCondition {
    FINISH_DEFAULT_SETTING = 0, /* Ĭ������ */
    FINISH_EQUAL_TO, /* ���ڽ�����Ӧ�ַ� */
    FINISH_UN_EQUAL_TO, /* �����ڽ�����Ӧ�ַ� */
} serviceFinishCondition;
    
typedef struct service_config {
    unsigned char sid; /* ��Ϸ���ID */
    unsigned char sub; /* ����ӹ��� */
    unsigned short did; /* ������ݱ�ʶ�� */
    unsigned int delay; /* ��Ϸ�������ǰ����ʱʱ�� */
    unsigned int timeout; /* ��Ϸ�����Ӧ��ʱʱ�� */
    unsigned char issuppress; /* �Ƿ���������Ӧ */
    unsigned int tatype; /* Ŀ���ַ���� */
    unsigned int ta; /* Ŀ���ַ */
    unsigned int sa; /* Դ��ַ */
    ByteArray *variableByte; /* ��Ϸ����еĿɱ����ݣ�UDS �ͻ��˽����� sid sub did������Զ�����UDS�������� */
    ByteArray *expectResponByte; /* Ԥ�������Ӧ���ݣ������жϵ�ǰ��Ϸ���ִ���Ƿ����Ԥ�� */
    unsigned int expectRespon_rule; /* Ԥ����Ӧ���� */
    ByteArray *finishByte;  /* ��Ӧ����ƥ�����ݣ������жϵ�ǰ��Ϸ����Ƿ���Ҫ�ظ�ִ�� */
    unsigned int finish_rule; /* ��Ӧ�������� */
    unsigned int finish_num_max; /* ��Ӧ�������ƥ����� */
    unsigned int rr_callid; /* �������ص�������ID, ����0������Чֵ */
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
    char local_path[256]; /* ����ˢд�ļ�·�� */
    char desc[256]; /* ��Ϸ������������Ϣ */
} service_config;

#define C_KEY_GC_IS_FAILABORT             "IsFailAbort"
#define C_KEY_GC_TP_ENABLE                "TesterPresentEnable"
#define C_KEY_GC_TP_IS_REFRESH            "TesterPresentIsRefresh"
#define C_KEY_GC_TP_INTERVAL              "TesterPresentInterval"
#define C_KEY_GC_TP_TA                    "TesterPresentTa"
#define C_KEY_GC_TP_SA                    "TesterPresentSa"

typedef struct udsc_general_config {
    unsigned char isFailAbort; /* ������������������ʱ����ִֹ�з���������� */
    unsigned char tpEnable; /* ʹ����������������� */
    unsigned char isTpRefresh; /* �Ƿ�UDS����ˢ�¶�ʱ�� */
    unsigned int tpInterval; /* ���ͼ�� unit ms */
    unsigned int tpta; /* ���Ŀ�ĵ�ַ */
    unsigned int tpsa; /* ���Դ��ַ */  
} udsc_general_config;

/* ------------------------------------------------------------------------------------------------------- */
/* ---------------------------------------------��Ӧ������------------------------------------------------ */
#define DM_CAMMAND_ERR_NO                   (0)
#define DM_CAMMAND_ERR_SERVICE_REQUEST      (1) /* ���������ʧ�� */
#define DM_CAMMAND_ERR_START_SCRIPT_FAILED  (2) /* ������Ϸ���ű�ʧ�� */
#define DM_CAMMAND_ERR_NOT_FOUND_FILE       (3) /* δ���ֽű��ļ� */
#define DM_CAMMAND_ERR_UNABLE_PARSE_FILE    (4) /* �޷������ű��ļ� */
#define DM_CAMMAND_ERR_UNABLE_PARSE_CONFIG  (5) /* �޷������ű����� */
#define DM_CAMMAND_ERR_NOT_FOUND_SCRIPT     (6) /* δ���ֽű� */
#define DM_CAMMAND_ERR_UDSC_CREATE          (7) /* uds�ͻ��˴���ʧ�� */
#define DM_CAMMAND_ERR_UDSC_INVALID         (8) /* ��Ͽͻ���ID��Ч */
#define DM_CAMMAND_ERR_UDSC_RUNING          (9) /* udsc�ͻ����������޷����� */
#define DM_CAMMAND_ERR_SHM_OUTOF            (10) /* �����ڴ�ռ䲻�� */
#define DM_CAMMAND_ERR_REPLY_TIMEOUT        (11) /* Ӧ��ʱ */
#define DM_CAMMAND_ERR_UDS_RESPONSE_TIMEOUT (12) /* ���Ӧ��ʱ */
#define DM_CAMMAND_ERR_UDS_RESPONSE_UNEXPECT (13) /* ��Ϸ�Ԥ����Ӧ */
#define DM_CAMMAND_ERR_UDS_RESPONSE_OVERLEN  (14) /* ��Ͻ�����ȹ���������Buff���� */
#define DM_CAMMAND_ERR_UDS_SERVICE_ADD       (15) /* UDS���������ʧ�� */
#define DM_CAMMAND_ERR_DIAG_MASTER_MAX        (16) /* ����OTA MASTER����ota master�ﵽ���������� */
#define DM_CAMMAND_ERR_OMAPI_UNKNOWN         (17) /* ����OTA MASTER����δ֪ota master api */
#define DM_CAMMAND_ERR_DIAG_MASTER_CREATE     (18) /* ����OTA MASTER����ota master����ʧ�� */
#define DM_CAMMAND_ERR_DOIPC_INVALID         (19) /* doip�ͻ���ID��Ч */
#define DM_CAMMAND_ERR_DOIPC_CREATE_FAILED   (20) /* doip�ͻ��˴���ʧ�� */
/* ------------------------------------------------------------------------------------------------------- */
/* ----------------------------------diag_master ��diag_master_api�Ľ�������--------------------------------- */
#define DM_CMD_REPLY_MASK (0x40) /* ��Ӧ�������� */

/* diag_master ����������� */
#define DM_CAMMAND_SERVICE_INDICATION_REQUEST  (1) /* diag_master -> diag_master_api  */
#define DM_CAMMAND_SERVICE_INDICATION_REPLY    (DM_CAMMAND_SERVICE_INDICATION_REQUEST | DM_CMD_REPLY_MASK) /* diag_master_api -> diag_master  */

/* diag_master_api �������Ӧ������ */
#define DM_CAMMAND_SERVICE_RESPONSE_REQUEST    (2) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_SERVICE_RESPONSE_REPLY      (DM_CAMMAND_SERVICE_RESPONSE_REQUEST | DM_CMD_REPLY_MASK)  /* diag_master -> diag_master_api  */

/* diag_master_api ���� diag_master ������Ͻű� */
#define DM_CAMMAND_START_SCRIPT_REQUEST        (3) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_START_SCRIPT_REPLY          (DM_CAMMAND_START_SCRIPT_REQUEST | DM_CMD_REPLY_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api ���� diag_master ֹͣ��Ͻű� */
#define DM_CAMMAND_STOP_SCRIPT_REQUEST         (4) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_STOP_SCRIPT_REPLY           (DM_CAMMAND_STOP_SCRIPT_REQUEST | DM_CMD_REPLY_MASK) /* diag_master -> diag_master_api  */

/* diag_master ֪ͨ diag_master_api �������ִ�н�� */
#define DM_CAMMAND_SCRIPT_RESULT_REQUEST       (5) /* diag_master -> diag_master_api  */
#define DM_CAMMAND_SCRIPT_RESULT_REPLY         (DM_CAMMAND_SCRIPT_RESULT_REQUEST | DM_CMD_REPLY_MASK) /* diag_master_api -> diag_master  */

/* diag_master_api ������Ͻű� */
#define DM_CAMMAND_CONFIG_SCRIPT_REQUEST       (6) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_CONFIG_SCRIPT_REPLY         (DM_CAMMAND_CONFIG_SCRIPT_REQUEST | DM_CMD_REPLY_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api �� diag_masterֱ�Ӷ��ű��ļ� */
#define DM_CAMMAND_SCRIPT_PATH_REQUEST         (7) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_SCRIPT_PATH_REPLY           (DM_CAMMAND_SCRIPT_PATH_REQUEST | DM_CMD_REPLY_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api �� diag_master ���� uds�ͻ��� */
#define DM_CAMMAND_UDSC_CREATE_REQUEST         (8) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_UDSC_CREATE_REPLY           (DM_CAMMAND_UDSC_CREATE_REQUEST | DM_CMD_REPLY_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api �� diag_master ���� uds�ͻ��� */
#define DM_CAMMAND_UDSC_DESTORY_REQUEST        (9) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_UDSC_DESTORY_REPLY          (DM_CAMMAND_UDSC_DESTORY_REQUEST | DM_CMD_REPLY_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api ���� diag_master ��λ */
#define DM_CAMMAND_DIAG_MASTER_RESET_REQUEST    (10) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_DIAG_MASTER_RESET_REPLY      (DM_CAMMAND_DIAG_MASTER_RESET_REQUEST | DM_CMD_REPLY_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api ���� diag_master ����ͨ���� */
#define DM_CAMMAND_OTA_GENERAL_CONFIG_REQUEST  (11) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_OTA_GENERAL_CONFIG_REPLY    (DM_CAMMAND_OTA_GENERAL_CONFIG_REQUEST | DM_CMD_REPLY_MASK) /* diag_master -> diag_master_api  */

/* diag_master ֪ͨ diag_master_api ��������� */
#define DM_CAMMAND_SERVICE_REQUEST_RESULT_REQUEST (12) /* diag_master -> diag_master_api  */
#define DM_CAMMAND_SERVICE_REQUEST_RESULT_REPLY (DM_CAMMAND_SERVICE_REQUEST_RESULT_REQUEST | DM_CMD_REPLY_MASK) /* diag_master_api -> diag_master  */

/* diag_master�������Ӹ�diag_master_api����key */
#define DM_CAMMAND_SERVICE_SA_SEED_REQUEST (13) /* diag_master -> diag_master_api  */
#define DM_CAMMAND_SERVICE_SA_SEED_REPLY (DM_CAMMAND_SERVICE_SA_SEED_REQUEST | DM_CMD_REPLY_MASK) /* diag_master_api -> diag_master  */

/* diag_master_api�����ɵ�key���͸�diag_master */
#define DM_CAMMAND_SERVICE_SA_KEY_REQUEST (14) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_SERVICE_SA_KEY_REPLY (DM_CAMMAND_SERVICE_SA_KEY_REQUEST | DM_CMD_REPLY_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api��������ota master������ */
#define DM_CAMMAND_CONNECT_DIAG_MASTER_REQUEST (15) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_CONNECT_DIAG_MASTER_REPLY (DM_CAMMAND_CONNECT_DIAG_MASTER_REQUEST | DM_CMD_REPLY_MASK) /* diag_master -> diag_master_api  */

/* ota master��������������ȷ��diag_master_api�Ƿ����� */
#define DM_CAMMAND_OMAPI_KEEPALIVE_REQUEST (16) /* diag_master -> diag_master_api  */
#define DM_CAMMAND_OMAPI_KEEPALIVE_REPLY (DM_CAMMAND_OMAPI_KEEPALIVE_REQUEST | DM_CMD_REPLY_MASK) /* diag_master_api -> diag_master  */

/* diag_master_api���󴴽�DOIP�ͻ��� */
#define DM_CAMMAND_DOIPC_CREATE_REQUEST (17) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_DOIPC_CREATE_REPLY (DM_CAMMAND_DOIPC_CREATE_REQUEST | DM_CMD_REPLY_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api����doipͨ����uds�ͻ��� */
#define DM_CAMMAND_DOIPC_BIND_UDSC_REQUEST (17) /* diag_master_api -> diag_master  */
#define DM_CAMMAND_DOIPC_BIND_UDSC_REPLY (DM_CAMMAND_DOIPC_BIND_UDSC_REQUEST | DM_CMD_REPLY_MASK) /* diag_master -> diag_master_api  */

#define DM_CAMMAND_MAX (18) /* �������ֵ */
/* ------------------------------------------------------------------------------------------------------- */

#define DIAGNOSTIC_TYPE_CAN (1)
#define DIAGNOSTIC_TYPE_DOIP (2)
#define DIAGNOSTIC_TYPE_REMOTE (3)
#define DIAGNOSTIC_TYPE_RUNTIME_DBG (4)

#define PHYSICAL_ADDRESS (1)
#define FUNCTION_ADDRESS (2)

/* ���̼佻�����ݽṹ���壬ֱ�Ӷ�����������ڽṹ���ڵ�ƫ����������ֱ�ӷ��ʹ����ڴ��е����ݣ��������ݿ������� */
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
    unsigned int cmd; /* ���� */
    unsigned int recode; /* ������Ӧ�� */
    unsigned short udsc_id; /* UDS �ͻ���ID�����ڱ�ʶ���ĸ�UDS�ͻ��� */
    unsigned int tv_sec; /* ʱ��� s */
    unsigned int tv_usec; /* ʱ��� us */
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
    unsigned int shmkey; /* �����ڴ�key */
    unsigned int shmsize; /* �����ڴ��С */    
    unsigned int cmd_valid_time; /* ������Чʱ��,�շ��˳�ʱ��������ɵ��쳣 */
    unsigned int shm_udsInd_offset; /* UDS�����ڹ����ڴ��е�ƫ�Ƶ�ַ */    
    unsigned int shm_udsInd_size; /* UDS�����ڹ����ڴ��д�С */
    unsigned int shm_udsRep_offset; /* UDSӦ���ڹ����ڴ��е�ƫ�Ƶ�ַ */    
    unsigned int shm_udsRep_size; /* UDSӦ���ڹ����ڴ��д�С */
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

/* ota master api��������ota master */
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

/* ota master ��Ӧota master api���������� */
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

/* ota master api���󴴽�doip�ͻ��� */
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
