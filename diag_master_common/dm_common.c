#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else /* #ifdef _WIN32 */
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <sys/un.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif /* #ifdef _WIN32 */

#include "cjson.h"
#include "dm_common.h"

int dm_recvfrom(int sockfd, unsigned char *buff, unsigned int size, unsigned int toms)
{
#ifdef __linux__
    struct sockaddr_un un;
    int len = sizeof(struct sockaddr_un);
#endif /* __linux__ */
    int bytesRecv = -1;
    fd_set readfds;
    struct timeval timeout;
    int sret = 0;
    
    if (sockfd < 0) return 0;

    timeout.tv_sec = toms / 1000;
    timeout.tv_usec = (toms % 1000) * 1000;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    if ((sret = select(sockfd + 1, &readfds, NULL, NULL, &timeout)) > 0 &&\
        FD_ISSET(sockfd, &readfds)) {
#ifdef __linux__
        bytesRecv = recvfrom(sockfd, buff, size, 0, \
                                (struct sockaddr*)&un, (socklen_t *)&len);
#endif /* __linux__ */
#ifdef _WIN32
        SOCKADDR_IN fromAddress;
        int fromLength = sizeof(fromAddress);
        bytesRecv = recvfrom(sockfd, buff, size, 0, (SOCKADDR*)&fromAddress, &fromLength);
#endif /* _WIN32 */
    } 
    // log_hex_d("recv", buff, bytesRecv);
    return bytesRecv;
}

int dm_command_reply(int sockfd, unsigned int *cmd, unsigned int *recode)
{
#ifdef __linux__
    struct sockaddr_un un;
    int len = sizeof(struct sockaddr_un);
#endif /* __linux__ */
    unsigned char recvbuf[64] = {0};
    int bytesRecv = -1;

    if (!cmd || !recode) return 0;
    if (sockfd < 0) return 0;
#ifdef __linux__
    bytesRecv = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, \
                            (struct sockaddr*)&un, (socklen_t *)&len);
#endif /* __linux__ */
#ifdef _WIN32
    SOCKADDR_IN fromAddress;
    int fromLength = sizeof(fromAddress);
    bytesRecv = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, (SOCKADDR*)&fromAddress, &fromLength);
#endif /* _WIN32 */
    if (bytesRecv > 0) {        
        log_hex_d("recvfrom", recvbuf, bytesRecv);
        *cmd = *(unsigned int *)(recvbuf + DM_IPC_COMMON_CMD_OFFSET);
        *recode = *(unsigned int *)(recvbuf + DM_IPC_COMMON_RECODE_OFFSET);
    }

    return bytesRecv;
}

int dm_tv_get(unsigned int *tv_sec, unsigned int *tv_usec)
{
    if (!tv_sec || !tv_usec) return -1;
#ifdef _WIN32

#else /* #ifdef _WIN32 */
    struct timespec time = {0, 0};

    // gettimeofday(tv, NULL);
    clock_gettime(CLOCK_MONOTONIC, &time);
    *tv_sec = time.tv_sec;
    *tv_usec = time.tv_nsec / 1000;
     
    return 0;
#endif /* #ifdef _WIN32 */
}

int dm_tv_currtdms(unsigned int tv_sec, unsigned int tv_usec)
{
    unsigned int curr_tv_sec = 0; unsigned int curr_tv_usec = 0;
    /* 不考虑时间溢出的风险 */
    dm_tv_get(&curr_tv_sec, &curr_tv_usec);
    
    return (curr_tv_sec * 1000 + curr_tv_usec / 1000) - (tv_sec * 1000 + tv_usec / 1000); 
}

int dm_common_encode(unsigned char *buff, unsigned int size, unsigned int cmd, unsigned int recode, unsigned short id, unsigned int tv_sec, unsigned int tv_usec)
{
    if (!buff || size < DM_IPC_COMMON_MSG_SIZE) return -1;

    memset(buff, 0, DM_IPC_COMMON_MSG_SIZE);
    memcpy(buff + DM_IPC_COMMON_CMD_OFFSET, &cmd, DM_IPC_COMMON_CMD_SIZE);
    memcpy(buff + DM_IPC_COMMON_RECODE_OFFSET, &recode, DM_IPC_COMMON_RECODE_SIZE);
    memcpy(buff + DM_IPC_COMMON_UDSC_ID_OFFSET, &id, DM_IPC_COMMON_UDSC_ID_SIZE);    
    dm_tv_get(&tv_sec, &tv_usec);
    
    memcpy(buff + DM_IPC_COMMON_TV_SEC_OFFSET, &tv_sec, DM_IPC_COMMON_TV_SEC_SIZE);
    memcpy(buff + DM_IPC_COMMON_TV_USEC_OFFSET, &tv_usec, DM_IPC_COMMON_TV_USEC_SIZE);
    // log_hex_d("buff", buff, DM_IPC_COMMON_MSG_SIZE);

    return 0;
}

int dm_common_decode(unsigned char *buff, unsigned int size, unsigned int *cmd, unsigned int *recode, unsigned short *id, unsigned int *tv_sec, unsigned int *tv_usec)
{
    if (!buff || size < DM_IPC_COMMON_MSG_SIZE) return -1;
    
    // log_hex_d("buff", buff, DM_IPC_COMMON_MSG_SIZE);
    *cmd = *(unsigned int *)(buff + DM_IPC_COMMON_CMD_OFFSET);
    *recode = *(unsigned int *)(buff + DM_IPC_COMMON_RECODE_OFFSET);
    *id = *(unsigned short *)(buff + DM_IPC_COMMON_UDSC_ID_OFFSET); 
    *tv_sec = *(unsigned int *)(buff + DM_IPC_COMMON_TV_SEC_OFFSET);
    *tv_usec = *(unsigned int *)(buff + DM_IPC_COMMON_TV_USEC_OFFSET);

    return 0;
}

int dm_service_config_encode(unsigned char *buff, unsigned int size, service_config *config)
{
    int ii = 0;
    int rlen = 0;

    if (config == 0) return -1;

    cJSON *root = cJSON_CreateObject();
    if (root == 0) return -1;
    if (config->sid > 0) {
        cJSON_AddItemToObject(root, C_KEY_SI_ID, cJSON_CreateNumber(config->sid));
    }
    if (config->sub > 0) {
        cJSON_AddItemToObject(root, C_KEY_SI_SUB, cJSON_CreateNumber(config->sub));
    }    
    if (config->did > 0) {
        cJSON_AddItemToObject(root, C_KEY_SI_DID, cJSON_CreateNumber(config->did));    
    }    
    if (config->delay > 0) {
        cJSON_AddItemToObject(root, C_KEY_SI_ITEM_DELAY_TIME, cJSON_CreateNumber(config->delay));
    }    
    if (config->timeout > 0) {
        cJSON_AddItemToObject(root, C_KEY_SI_TIMEOUT, cJSON_CreateNumber(config->timeout));   
    }
    cJSON_AddItemToObject(root, C_KEY_SI_IS_SUPPRESS, cJSON_CreateBool(config->issuppress));
    if (config->tatype > 0) {
        cJSON_AddItemToObject(root, C_KEY_SI_TA_TYPE, cJSON_CreateNumber(config->tatype));
    }
    if (config->ta > 0) {
        cJSON_AddItemToObject(root, C_KEY_SI_TA, cJSON_CreateNumber(config->ta));
    }
    if (config->sa > 0) {
        cJSON_AddItemToObject(root, C_KEY_SI_SA, cJSON_CreateNumber(config->sa));
    }
    if (config->maxNumberOfBlockLength > 0) {        
        cJSON_AddItemToObject(root, C_KEY_SI_MAX_NUMBER_OF_BLOCK_LEN, cJSON_CreateNumber(config->maxNumberOfBlockLength));
    }
    cJSON_AddItemToObject(root, C_KEY_SI_EXPECT_RESPONSE_RULE, cJSON_CreateNumber(config->expectRespon_rule));
    cJSON_AddItemToObject(root, C_KEY_SI_FINISH_RESPONSE_RULE, cJSON_CreateNumber(config->finish_rule));
    if (config->finish_num_max > 0) {
        cJSON_AddItemToObject(root, C_KEY_SI_FINISH_RESPONSE_TRYMAX, cJSON_CreateNumber(config->finish_num_max));
    }
    if (config->rr_callid > 0) {
        cJSON_AddItemToObject(root, C_KEY_SI_REQUEST_RESULT_CALL_ID, cJSON_CreateNumber(config->rr_callid));
    }
    if (strlen(config->desc) > 0) {
        cJSON_AddItemToObject(root, C_KEY_SI_DESC, cJSON_CreateString(config->desc));
    }
    if (config->variableByte && ByteArrayCount(config->variableByte) > 0) {
        cJSON *arrobj = cJSON_CreateArray();
        if (arrobj) {
            for (ii = 0; ii < ByteArrayCount(config->variableByte); ii++) {
                cJSON_AddItemToArray(arrobj, cJSON_CreateNumber(ByteArrayConstData(config->variableByte)[ii]));
            }
            cJSON_AddItemToObject(root, C_KEY_SI_VARIABLE_BYTE, arrobj);
        }
    }
    if (config->expectResponByte && ByteArrayCount(config->expectResponByte) > 0) {
        cJSON *arrobj = cJSON_CreateArray();        
        if (arrobj) {
            for (ii = 0; ii < ByteArrayCount(config->expectResponByte); ii++) {
                cJSON_AddItemToArray(arrobj, cJSON_CreateNumber(ByteArrayConstData(config->expectResponByte)[ii]));
            }
            cJSON_AddItemToObject(root, C_KEY_SI_EXPECT_RESPONSE_BYTE, arrobj);
        }
    }
    if (config->finishByte && ByteArrayCount(config->finishByte) > 0) {
        cJSON *arrobj = cJSON_CreateArray();
        if (arrobj) {
            for (ii = 0; ii < ByteArrayCount(config->finishByte); ii++) {
                cJSON_AddItemToArray(arrobj, cJSON_CreateNumber(ByteArrayConstData(config->finishByte)[ii]));
            }
            cJSON_AddItemToObject(root, C_KEY_SI_FINISH_RESPONSE_BYTE, arrobj);
        }
    }
    if (config->sid == 0x34) {
        cJSON *rdobj = cJSON_CreateObject();
        if (rdobj) {
            cJSON_AddItemToObject(root, C_KEY_SI_ID_34_RD, rdobj);
            cJSON_AddItemToObject(rdobj, C_KEY_SI_ID_34_DFI, cJSON_CreateNumber(config->service_34_rd.dataFormatIdentifier));
            cJSON_AddItemToObject(rdobj, C_KEY_SI_ID_34_AALFI, cJSON_CreateNumber(config->service_34_rd.addressAndLengthFormatIdentifier));
            cJSON_AddItemToObject(rdobj, C_KEY_SI_ID_34_MA, cJSON_CreateNumber(config->service_34_rd.memoryAddress));
            cJSON_AddItemToObject(rdobj, C_KEY_SI_ID_34_MS, cJSON_CreateNumber(config->service_34_rd.memorySize));        
            cJSON_AddItemToObject(rdobj, C_KEY_SI_LOCAL_FILE_PATH, cJSON_CreateString(config->local_path));
        }
    }
    if (config->sid == 0x38) {
        cJSON *rftobj = cJSON_CreateObject();
        if (rftobj) {
            cJSON_AddItemToObject(root, C_KEY_SI_ID_38_FRT, rftobj);
            cJSON_AddItemToObject(rftobj, C_KEY_SI_ID_38_MOO, cJSON_CreateNumber(config->service_38_rft.modeOfOperation));
            cJSON_AddItemToObject(rftobj, C_KEY_SI_ID_38_FPANL, cJSON_CreateNumber(config->service_38_rft.filePathAndNameLength));
            cJSON_AddItemToObject(rftobj, C_KEY_SI_ID_38_FPAN, cJSON_CreateString(config->service_38_rft.filePathAndName));
            cJSON_AddItemToObject(rftobj, C_KEY_SI_ID_38_DFI, cJSON_CreateNumber(config->service_38_rft.dataFormatIdentifier));
            cJSON_AddItemToObject(rftobj, C_KEY_SI_ID_38_FSPL, cJSON_CreateNumber(config->service_38_rft.fileSizeParameterLength));
            cJSON_AddItemToObject(rftobj, C_KEY_SI_ID_38_FSUC, cJSON_CreateNumber(config->service_38_rft.fileSizeUnCompressed));
            cJSON_AddItemToObject(rftobj, C_KEY_SI_ID_38_FSC, cJSON_CreateNumber(config->service_38_rft.fileSizeCompressed));            
            cJSON_AddItemToObject(rftobj, C_KEY_SI_LOCAL_FILE_PATH, cJSON_CreateString(config->local_path));
        }
    }

    const char *str = cJSON_PrintUnformatted(root);
    if (str) {
        rlen = snprintf((char *)buff, size, "%s", str);
        if (rlen < size) {
            buff[rlen] = 0;
        }
        //log_d("%s \n", str);
        free((void *)str);
    }
    cJSON_Delete(root);

    return rlen;
}

int dm_service_config_decode(unsigned char *buff, unsigned int size, service_config *config)
{
    int arrcount = 0;
    cJSON *vobj = 0;

    if (config == 0) return -1;

    cJSON *root = cJSON_Parse((const char *)buff);
    if (root == 0) return -2;

    vobj = cJSON_GetObjectItem(root, C_KEY_SI_ID);
    if (vobj) {
        config->sid = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_SUB);
    if (vobj) {
        config->sub = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_DID);
    if (vobj) {
        config->did = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_ITEM_DELAY_TIME);
    if (vobj) {
        config->delay = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_TIMEOUT);
    if (vobj) {
        config->timeout = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_IS_SUPPRESS);
    if (vobj) {
        config->issuppress = cJSON_IsFalse(vobj) ? 0 : 1;        
    }    
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_TA_TYPE);    
    if (vobj) {
        config->tatype = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_TA);
    if (vobj) {
        config->ta = cJSON_GetNumberValue(vobj);        
    }    
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_SA);
    if (vobj) {
        config->sa = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_DESC);
    if (vobj) {
        snprintf(config->desc, sizeof(config->desc), "%s", cJSON_GetStringValue(vobj));       
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_EXPECT_RESPONSE_RULE);
    if (vobj) {
        config->expectRespon_rule = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_FINISH_RESPONSE_RULE);
    if (vobj) {
        config->finish_rule = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_FINISH_RESPONSE_TRYMAX);
    if (vobj) {
        config->finish_num_max = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_REQUEST_RESULT_CALL_ID);
    if (vobj) {
        config->rr_callid = cJSON_GetNumberValue(vobj);        
    } 
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_MAX_NUMBER_OF_BLOCK_LEN);
    if (vobj) {        
        config->maxNumberOfBlockLength = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_VARIABLE_BYTE); 
    if (vobj) {
        arrcount = cJSON_GetArraySize(vobj);
        int index = 0;
        for (index = 0; index < arrcount; index++) {
            cJSON *arrayItem = cJSON_GetArrayItem(vobj, index);
            if (arrayItem && config->variableByte) {
                ByteArrayAppendChar(config->variableByte, cJSON_GetNumberValue(arrayItem));                
            }            
        }         
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_EXPECT_RESPONSE_BYTE); 
    if (vobj) {
        arrcount = cJSON_GetArraySize(vobj);
        int index = 0;
        for (index = 0; index < arrcount; index++) {
            cJSON *arrayItem = cJSON_GetArrayItem(vobj, index);
            if (arrayItem && config->expectResponByte) {
                ByteArrayAppendChar(config->expectResponByte, cJSON_GetNumberValue(arrayItem));                
            }            
        }         
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_FINISH_RESPONSE_BYTE); 
    if (vobj) {
        arrcount = cJSON_GetArraySize(vobj);
        int index = 0;
        for (index = 0; index < arrcount; index++) {
            cJSON *arrayItem = cJSON_GetArrayItem(vobj, index);
            if (arrayItem && config->finishByte) {
                ByteArrayAppendChar(config->finishByte, cJSON_GetNumberValue(arrayItem));
            }
        }         
    }

    vobj = cJSON_GetObjectItem(root, C_KEY_SI_ID_34_RD);
    if (vobj) {
        cJSON *tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_ID_34_DFI);
        if (tobj) {
            config->service_34_rd.dataFormatIdentifier = cJSON_GetNumberValue(tobj);        
        }
        tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_ID_34_AALFI);
        if (tobj) {
            config->service_34_rd.addressAndLengthFormatIdentifier = cJSON_GetNumberValue(tobj);        
        }
        tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_ID_34_MA);
        if (tobj) {
            config->service_34_rd.memoryAddress = cJSON_GetNumberValue(tobj);        
        }        
        tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_ID_34_MS);
        if (tobj) {
            config->service_34_rd.memorySize = cJSON_GetNumberValue(tobj);        
        }
        tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_LOCAL_FILE_PATH);
        if (tobj) {
            snprintf(config->local_path, \
                sizeof(config->local_path), "%s", cJSON_GetStringValue(tobj));       
        } 
    }

    vobj = cJSON_GetObjectItem(root, C_KEY_SI_ID_38_FRT);
    if (vobj) {
        cJSON *tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_ID_38_MOO);
        if (tobj) {
            config->service_38_rft.modeOfOperation = cJSON_GetNumberValue(tobj);        
        }
        tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_ID_38_FPANL);
        if (tobj) {
            config->service_38_rft.filePathAndNameLength = cJSON_GetNumberValue(tobj);        
        }
        tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_ID_38_FPAN);
        if (tobj) {
            snprintf(config->service_38_rft.filePathAndName, \
                sizeof(config->service_38_rft.filePathAndName), "%s", cJSON_GetStringValue(tobj));       
        }
        tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_LOCAL_FILE_PATH);
        if (tobj) {
            snprintf(config->local_path, \
                sizeof(config->local_path), "%s", cJSON_GetStringValue(tobj));       
        } 
        tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_ID_38_DFI);
        if (tobj) {
            config->service_38_rft.dataFormatIdentifier = cJSON_GetNumberValue(tobj);        
        }
        tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_ID_38_FSPL);
        if (tobj) {
            config->service_38_rft.fileSizeParameterLength = cJSON_GetNumberValue(tobj);        
        }
        tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_ID_38_FSUC);
        if (tobj) {
            config->service_38_rft.fileSizeUnCompressed = cJSON_GetNumberValue(tobj);        
        }
        tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_ID_38_FSC);
        if (tobj) {
            config->service_38_rft.fileSizeCompressed = cJSON_GetNumberValue(tobj);        
        }
    }
    cJSON_Delete(root);
    
    return 0;
}

int dm_general_config_encode(unsigned char *buff, unsigned int size, udsc_general_config *config)
{
    int rlen = 0;

    if (config == 0) return -1;

    cJSON *root = cJSON_CreateObject();
    if (root == 0) return -1;

    cJSON_AddItemToObject(root, C_KEY_GC_IS_FAILABORT, cJSON_CreateBool(config->isFailAbort));
    cJSON_AddItemToObject(root, C_KEY_GC_TP_ENABLE, cJSON_CreateBool(config->tpEnable));
    cJSON_AddItemToObject(root, C_KEY_GC_TP_IS_REFRESH, cJSON_CreateBool(config->isTpRefresh));
    cJSON_AddItemToObject(root, C_KEY_GC_TP_INTERVAL, cJSON_CreateNumber(config->tpInterval));
    cJSON_AddItemToObject(root, C_KEY_GC_TP_TA, cJSON_CreateNumber(config->tpta));
    cJSON_AddItemToObject(root, C_KEY_GC_TP_SA, cJSON_CreateNumber(config->tpsa));
    const char *str = cJSON_PrintUnformatted(root);
    if (str) {
        rlen = snprintf((char *)buff, size, "%s", str);        
        if (rlen < size) {
            buff[rlen] = 0;
        }
        //log_d("%s \n", str);
        free((void *)str);
    }
    cJSON_Delete(root);

    return rlen;
}

int dm_general_config_decode(unsigned char *buff, unsigned int size, udsc_general_config *config)
{
    cJSON *vobj = 0;

    if (config == 0) return -1;

    cJSON *root = cJSON_Parse((const char *)buff);
    if (root == 0) return -2;
    
    vobj = cJSON_GetObjectItem(root, C_KEY_GC_IS_FAILABORT);
    if (vobj) {
        config->isFailAbort = cJSON_IsFalse(vobj) ? 0 : 1;        
    }    
    vobj = cJSON_GetObjectItem(root, C_KEY_GC_TP_ENABLE);
    if (vobj) {
        config->tpEnable = cJSON_IsFalse(vobj) ? 0 : 1;        
    }  
    vobj = cJSON_GetObjectItem(root, C_KEY_GC_TP_IS_REFRESH);
    if (vobj) {
        config->isTpRefresh = cJSON_IsFalse(vobj) ? 0 : 1;        
    }  
    vobj = cJSON_GetObjectItem(root, C_KEY_GC_TP_INTERVAL);
    if (vobj) {
        config->tpInterval = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_GC_TP_TA);
    if (vobj) {
        config->tpta = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_GC_TP_SA);
    if (vobj) {
        config->tpsa = cJSON_GetNumberValue(vobj);        
    }

    cJSON_Delete(root);

    return 0;
}

char *dm_ipc_command_str(unsigned int cmd)
{
    switch (cmd) {
        case DM_CAMMAND_SERVICE_INDICATION_REQUEST: 
             return "(Command request) <service indication>";
        case DM_CAMMAND_SERVICE_INDICATION_REPLY:   
             return "[Command reply] <service indication>";
        /* dm_api 发送诊断应答请求 */
        case DM_CAMMAND_SERVICE_RESPONSE_REQUEST:            
             return "(Command request>) <service rsponse>";
        case DM_CAMMAND_SERVICE_RESPONSE_REPLY:     
             return "[Command reply] <service rsponse>";
        /* dm_api 控制 ota_master 启动诊断脚本 */
        case DM_CAMMAND_START_SCRIPT_REQUEST:            
             return "(Command request) <all service start>";
        case DM_CAMMAND_START_SCRIPT_REPLY:          
             return "[Command reply] <all service start>";
        /* dm_api 控制 ota_master 停止诊断脚本 */
        case DM_CAMMAND_STOP_SCRIPT_REQUEST:           
             return "(Command request) <all service stop>";
        case DM_CAMMAND_STOP_SCRIPT_REPLY:           
             return "[Command reply] <all service stop>";
        /* ota_master 通知 dm_api 诊断任务执行结果 */
        case DM_CAMMAND_SCRIPT_RESULT_REQUEST:            
             return "(Command request) <all service result>";
        case DM_CAMMAND_SCRIPT_RESULT_REPLY:
             return "[Command reply] <all service result>";
        /* dm_api 配置诊断脚本 */
        case DM_CAMMAND_CONFIG_SCRIPT_REQUEST:            
             return "(Command request) <service config>";
        case DM_CAMMAND_CONFIG_SCRIPT_REPLY:
             return "[Command reply] <service config>";
        /* dm_api 让 ota_master直接读脚本文件 */
        case DM_CAMMAND_SCRIPT_PATH_REQUEST:            
             return "(Command request) <service config path>";
        case DM_CAMMAND_SCRIPT_PATH_REPLY:
             return "[Command reply] <service config path>";
        /* dm_api 让 ota_master 创建 uds客户端 */
        case DM_CAMMAND_UDSC_CREATE_REQUEST:            
             return "(Command request) <uds client create>";
        case DM_CAMMAND_UDSC_CREATE_REPLY:
             return "[Command reply] <uds client create>";
        /* dm_api 让 ota_master 销毁 uds客户端 */
        case DM_CAMMAND_UDSC_DESTORY_REQUEST:            
             return "(Command request) <uds client destory>";
        case DM_CAMMAND_UDSC_DESTORY_REPLY:
             return "[Command reply] <uds client destory>";
        /* dm_api 请求 ota_master 复位 */
        case DM_CAMMAND_DIAG_MASTER_RESET_REQUEST:            
             return "(Command request) <diag master reset>";
        case DM_CAMMAND_DIAG_MASTER_RESET_REPLY:
             return "[Command reply] <diag master reset>";
        /* dm_api 请求 ota_master 配置通用项 */
        case DM_CAMMAND_OTA_GENERAL_CONFIG_REQUEST:            
             return "(Command request) <diag master general config>";
        case DM_CAMMAND_OTA_GENERAL_CONFIG_REPLY:
             return "[Command reply] <diag master general config>";
        /* ota_master 通知 dm_api 诊断请求结果 */
        case DM_CAMMAND_SERVICE_REQUEST_RESULT_REQUEST:
             return "(Command request) <service request result>";
        case DM_CAMMAND_SERVICE_REQUEST_RESULT_REPLY:
             return "[Command reply] <service request result>";
        case DM_CAMMAND_SERVICE_SA_SEED_REQUEST:
             return "(Command request) <service request SA seed>";
        case DM_CAMMAND_SERVICE_SA_SEED_REPLY:
             return "[Command reply] <service request SA seed>";             
        case DM_CAMMAND_SERVICE_SA_KEY_REQUEST:
             return "(Command request) <service request SA key>";
        case DM_CAMMAND_SERVICE_SA_KEY_REPLY:
             return "[Command reply] <service request SA key>";
        case DM_CAMMAND_CONNECT_DIAG_MASTER_REQUEST:            
            return "(Command request) <connect diag master>";
        case DM_CAMMAND_CONNECT_DIAG_MASTER_REPLY:
            return "[Command reply] <connect diag master>";
        case DM_CAMMAND_OMAPI_KEEPALIVE_REQUEST:
            return "(Command request) <keepalive>";
        case DM_CAMMAND_OMAPI_KEEPALIVE_REPLY:
            return "[Command reply] <keepalive>";
        case DM_CAMMAND_DOIPC_CREATE_REQUEST:
            return "(Command request) <create doip client>";
        case DM_CAMMAND_DOIPC_CREATE_REPLY:
            return "[Command reply] <create doip client>";
        default:
            return "unknown";
    }
}

char *dm_command_rcode_str(int rcode)
{
    switch (rcode) {
        case DM_CAMMAND_ERR_NO:
            return "<IPC COMMAND RCODE> no error";
        case DM_CAMMAND_ERR_SERVICE_REQUEST:
            return "<IPC COMMAND RCODE> UDS request sent failed";
        case DM_CAMMAND_ERR_START_SCRIPT_FAILED:
            return "<IPC COMMAND RCODE> UDS client start failed";
        case DM_CAMMAND_ERR_NOT_FOUND_FILE:
            return "<IPC COMMAND RCODE> UDS services script file not found";
        case DM_CAMMAND_ERR_UNABLE_PARSE_FILE:
            return "<IPC COMMAND RCODE> UDS services script file not parse";
        case DM_CAMMAND_ERR_UNABLE_PARSE_CONFIG:
            return "<IPC COMMAND RCODE> UDS services config not parse";
        case DM_CAMMAND_ERR_UDSC_CREATE:
            return "<IPC COMMAND RCODE> UDS client create failed";
        case DM_CAMMAND_ERR_UDSC_INVALID:
            return "<IPC COMMAND RCODE> UDS client id invalid";
        case DM_CAMMAND_ERR_UDSC_RUNING:
            return "<IPC COMMAND RCODE> UDS client runing not destory";
        case DM_CAMMAND_ERR_UDS_RESPONSE_TIMEOUT:
            return "<IPC COMMAND RCODE> UDS service response timeout";
        case DM_CAMMAND_ERR_UDS_RESPONSE_UNEXPECT:
            return "<IPC COMMAND RCODE> UDS service response unexpect";
        case DM_CAMMAND_ERR_DIAG_MASTER_MAX:
            return "<IPC COMMAND RCODE> diag master out max";
        case DM_CAMMAND_ERR_OMAPI_UNKNOWN:
            return "<IPC COMMAND RCODE> diag master api unknown";
        case DM_CAMMAND_ERR_DIAG_MASTER_CREATE:        
            return "<IPC COMMAND RCODE> diag master create failed";
        case DM_CAMMAND_ERR_DOIPC_CREATE_FAILED:
            return "<IPC COMMAND RCODE> doip client create failed";
        default:
            return "<IPC COMMAND RCODE> unknown";
    }

    return "<IPC COMMAND RCODE> unknown";
}

#define BYTE_ARRAY_RESERVED_SIZE (32)

ByteArray *ByteArrayNew()
{
    ByteArray *arr = calloc(sizeof(ByteArray), 1);
    if (arr) {
        arr->size = 1;
        arr->data = calloc(arr->size, 1);
        if (arr->data) {
            arr->dlen = 0;
        }
    }
    
    return arr;
}

void ByteArrayDelete(ByteArray *arr)
{
    if (arr) {
        if (arr->data) { 
            free(arr->data);
        }
        free(arr);
    }
}

void ByteArrayClear(ByteArray *arr)
{
    arr->dlen = 0;
    memset(arr->data, 0, arr->size);
}

const unsigned char *ByteArrayConstData(ByteArray *arr)
{
    return arr->data;
}

int ByteArrayCount(ByteArray *arr)
{
    return arr->dlen;
}

void ByteArrayAppendChar(ByteArray *dest, unsigned char c)
{
    if (!(dest->dlen + 1 <= dest->size)) {
        dest->size = BYTE_ARRAY_RESERVED_SIZE + dest->dlen + 1;
        unsigned char *data = calloc(dest->size, 1);
        if (data) {
            memcpy(data, dest->data, dest->dlen);
            data[dest->dlen] = c;
            dest->dlen++;
            free(dest->data);
            dest->data = data;
        }
    }
    else {
        dest->data[dest->dlen] = c;
        dest->dlen++;
    }
}

void ByteArrayAppendArray(ByteArray *dest, ByteArray *src)
{
    if (!(dest->dlen + src->dlen <= dest->size)) {
        dest->size = BYTE_ARRAY_RESERVED_SIZE + dest->dlen + src->dlen;
        unsigned char *data = calloc(dest->size, 1);
        if (data) {
            memcpy(data, dest->data, dest->dlen);
            memcpy(data + dest->dlen, src->data, src->dlen);
            dest->dlen += src->dlen;
            free(dest->data);
            dest->data = data;
        }
    }
    else {
        memcpy(dest->data + dest->dlen, src->data, src->dlen);
        dest->dlen += src->dlen;
    }
}

void ByteArrayAppendNChar(ByteArray *dest, unsigned char *c, unsigned int count)
{
    if (!(dest->dlen + count <= dest->size)) {
        dest->size = BYTE_ARRAY_RESERVED_SIZE + dest->dlen + count;
        unsigned char *data = calloc(dest->size, 1);
        if (data) {
            memcpy(data, dest->data, dest->dlen);
            memcpy(data + dest->dlen, c, count);
            dest->dlen += count;
            free(dest->data);
            dest->data = data;
        }
    }
    else {
        memcpy(dest->data + dest->dlen, c, count);
        dest->dlen += count;
    }    
}

int ByteArrayEqual(ByteArray *arr1, ByteArray *arr2)
{
    if (arr1->dlen == arr2->dlen && \
        memcmp(arr1->data, arr2->data, arr2->dlen) == 0) {
        return 1;
    }

    return 0;
}

int ByteArrayCharEqual(ByteArray *arr1, unsigned char *c, unsigned int count)
{
    if (arr1->dlen == count && \
        memcmp(arr1->data, c, count) == 0) {
        return 1;
    }

    return 0;
}

char ByteArrayAt(ByteArray *arr, int pos)
{


}