#include <stdio.h>
//#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#ifdef _WIN32

#else /* _WIN32 */
#include <unistd.h>
#endif /* _WIN32 */
#include <sys/stat.h>
#include <errno.h>

#include "config.h"
#ifdef __HAVE_LIBEV__
#include "ev.h"
#endif /* __HAVE_LIBEV__ */
#ifdef __HAVE_LIBUV__
#include "uv.h"
#endif /* __HAVE_LIBUV__ */
#include "dm_udsc_types.h"
#include "dm_common.h"       
#include "dm_uds_client.h"  

#ifdef __HAVE_LIBUV__
static void udsc_service_handler_callback(uv_timer_t* w);
static void udsc_response_timeout_callback(uv_timer_t* w);
static void udsc_tester_present_callback(uv_timer_t* w);
#endif /* __HAVE_LIBUV__ */

static BOOLEAN udsc_service_item_index_valid(uds_client *udsc, INT32 index)
{
    if (index >= 0 && \
        index < udsc->service_cnt && \
        index < udsc->service_size) {
        return true;
    }
    // log_w("UDSC Service item index %d is invalid \n", index);
    return false;
}

static BOOLEAN udsc_service_have_sub(INT8U sid)
{
    if (sid == UDS_SERVICES_DSC || \
        sid == UDS_SERVICES_ER || \
        sid == UDS_SERVICES_SA || \
        sid == UDS_SERVICES_CC || \
        sid == UDS_SERVICES_CDTCS || \
        sid == UDS_SERVICES_ROE || \
        sid == UDS_SERVICES_LC || \
        sid == UDS_SERVICES_RDTCI || \
        sid == UDS_SERVICES_DDDI || \
        sid == UDS_SERVICES_RC) {
        return true;
    }
    return false;
}

static BOOLEAN udsc_service_isactive(uds_client *udsc)
{
    return !!udsc_service_item_index_valid(udsc, udsc->sindex);
}

/*  */
static service_item *udsc_active_service_item(uds_client *udsc)
{
    /* ���ﲻӦ�÷��ؿ�ָ�룬���ؿ�ָ���ǲ������ */
    service_item *item = udsc->service_items[udsc->sindex];
    assert(item);
    return item;
}

/*  */
static void udsc_next_service_item(uds_client *udsc)
{
    if (udsc_service_isactive(udsc)) {
        udsc->sindex++;
    }
}

static BOOLEAN  udsc_services_stop(uds_client *udsc)
{
    log_d("UDSC %d Stop \n", udsc->id);
    udsc->sindex = -1;    
#ifdef __HAVE_LIBEV__
    ev_timer_stop(udsc->loop, &udsc->testerPresent_watcher);
    ev_timer_stop(udsc->loop, &udsc->request_watcher);
    ev_timer_stop(udsc->loop, &udsc->response_watcher);
#endif /* __HAVE_LIBEV__ */
#ifdef __HAVE_LIBUV__
    uv_timer_stop(&udsc->testerPresent_watcher);
    uv_timer_stop(&udsc->request_watcher);
    uv_timer_stop(&udsc->response_watcher);
#endif /* __HAVE_LIBUV__ */
    if (udsc->common_var.td_buff) {
        DM_FREE(udsc->common_var.td_buff);
        udsc->common_var.td_buff = NULL;
    }

    if (udsc->common_var.filefd) {
        fclose(udsc->common_var.filefd);
        udsc->common_var.filefd = NULL;
    }

    return true;
}

static void udsc_service_tester_present_refresh(uds_client *udsc)
{
#ifdef __HAVE_LIBEV__
    ev_timer_stop(udsc->loop, &udsc->testerPresent_watcher);
    ev_timer_set(&udsc->testerPresent_watcher, udsc->tpInterval * 0.001, udsc->tpInterval * 0.001);
    ev_timer_start(udsc->loop, &udsc->testerPresent_watcher);
#endif /* __HAVE_LIBEV__ */  
#ifdef __HAVE_LIBUV__
    uv_timer_stop(&udsc->testerPresent_watcher);
    uv_timer_start(&udsc->testerPresent_watcher, udsc_tester_present_callback, udsc->tpInterval, udsc->tpInterval);
#endif /* __HAVE_LIBUV__ */  
}

static void udsc_service_tester_present_start(uds_client *udsc)
{
#ifdef __HAVE_LIBEV__
    ev_timer_stop(udsc->loop, &udsc->testerPresent_watcher);
    ev_timer_set(&udsc->testerPresent_watcher, udsc->tpInterval * 0.001, udsc->tpInterval * 0.001);
    ev_timer_start(udsc->loop, &udsc->testerPresent_watcher);
#endif /* __HAVE_LIBEV__ */  
#ifdef __HAVE_LIBUV__
    uv_timer_stop(&udsc->testerPresent_watcher);
    uv_timer_start(&udsc->testerPresent_watcher, udsc_tester_present_callback, udsc->tpInterval, udsc->tpInterval);
#endif /* __HAVE_LIBUV__ */  
}

static void udsc_service_request_start(uds_client *udsc)
{
    service_item *item = udsc_active_service_item(udsc);  
    
    log_d("UDSC Service %s Pre-request delay %d ms start \n", item->desc ? item->desc : " ", item->delay);
#ifdef __HAVE_LIBEV__
    ev_timer_stop(udsc->loop, &udsc->request_watcher);
    ev_timer_set(&udsc->request_watcher, item->delay * 0.001, item->delay * 0.001);
    ev_timer_start(udsc->loop, &udsc->request_watcher);
#endif /* __HAVE_LIBEV__ */  
#ifdef __HAVE_LIBUV__
    uv_timer_stop(&udsc->request_watcher);
    uv_timer_start(&udsc->request_watcher, udsc_service_handler_callback, item->delay, item->delay);
#endif /* __HAVE_LIBUV__ */ 
}

static void udsc_service_response_start(uds_client *udsc)
{
    INT32 timeout = 0;
    service_item *item = udsc_active_service_item(udsc);
    
    if (item->timeout == 0) {
        timeout = udsc->common_var.p2_server_max > 0 ? udsc->common_var.p2_server_max : item->timeout;
    }
    else {
        timeout = item->timeout;
    }    
    log_d("UDSC Service %s Response timeout %d ms start \n", item->desc ? item->desc : " ", timeout);
#ifdef __HAVE_LIBEV__
    ev_timer_stop(udsc->loop, &udsc->response_watcher);
    ev_timer_set(&udsc->response_watcher, timeout * 0.001, timeout * 0.001);
    ev_timer_start(udsc->loop, &udsc->response_watcher);
#endif /* __HAVE_LIBEV__ */   
#ifdef __HAVE_LIBUV__
    uv_timer_stop(&udsc->response_watcher);
    uv_timer_start(&udsc->response_watcher, udsc_response_timeout_callback, timeout, timeout);
#endif /* __HAVE_LIBUV__ */  
}

static void udsc_service_response_stop(uds_client *udsc)
{
    INT32 timeout = 0;
    service_item *item = udsc_active_service_item(udsc);

    timeout = udsc->common_var.p2_server_max > 0 ? udsc->common_var.p2_server_max : item->timeout;
    log_d("UDSC Service %s Response timeout %d ms stop \n", item->desc ? item->desc : " ", timeout);
#ifdef __HAVE_LIBEV__
    ev_timer_stop(udsc->loop, &udsc->response_watcher);
#endif /* __HAVE_LIBEV__ */ 
#ifdef __HAVE_LIBUV__
    uv_timer_stop(&udsc->response_watcher);
#endif /* __HAVE_LIBUV__ */
}

static void udsc_service_response_refresh(uds_client *udsc)
{
    INT32 timeout = 0;
    service_item *item = udsc_active_service_item(udsc);

    timeout = udsc->common_var.p2_e_server_max > 0 ? udsc->common_var.p2_e_server_max : item->timeout;
    timeout *= 10;
    log_d("UDSC Service %s Response timeout %d ms refresh \n", item->desc ? item->desc : " ", timeout);
#ifdef __HAVE_LIBEV__
    ev_timer_stop(udsc->loop, &udsc->response_watcher);
    ev_timer_set(&udsc->response_watcher, timeout * 0.001, timeout * 0.001);
    ev_timer_start(udsc->loop, &udsc->response_watcher);
#endif /* __HAVE_LIBEV__ */   
#ifdef __HAVE_LIBUV__
    uv_timer_stop(&udsc->response_watcher);
    uv_timer_start(&udsc->response_watcher, udsc_response_timeout_callback, timeout, timeout);
#endif /* __HAVE_LIBUV__ */   
}

static void udsc_service_request_preprocessor(uds_client *udsc)
{
    size_t rsize = 0;
    service_item *item = udsc_active_service_item(udsc);
    
    if (item->sid == UDS_SERVICES_SA) {
        if (item->sub % 2 == 0) {
            ByteArrayAppendArray(item->request_byte, udsc->common_var.key_byte);
        }
    }
    else if (item->sid == UDS_SERVICES_TD) {
        if (item->td_36.maxNumberOfBlockLength > 0) {
            udsc->common_var.maxNumberOfBlockLength = \
            udsc->common_var.maxNumberOfBlockLength > item->td_36.maxNumberOfBlockLength ? \
            item->td_36.maxNumberOfBlockLength : udsc->common_var.maxNumberOfBlockLength;
        }
        if (udsc->common_var.td_buff == NULL ||\
            udsc->common_var.filefd == NULL) {
            udsc->common_var.fileTransferTotalSize = 0; /* 36�������ļ����ֽ��� */
            udsc->common_var.fileSize = 0; /* �ļ���С */
            log_e("The file is not open. The data in the file cannot be read. \n");
            return ;
        }
        log_d("fileTransferCount: %d \n", udsc->common_var.fileTransferCount);    
        log_d("fileTransferTotalSize: %d \n", udsc->common_var.fileTransferTotalSize);    
        log_d("fileSize: %d \n", udsc->common_var.fileSize);    
        log_d("maxNumberOfBlockLength: %d \n", udsc->common_var.maxNumberOfBlockLength);
            
        rsize = fread(udsc->common_var.td_buff, 1, \
            udsc->common_var.maxNumberOfBlockLength - 2, udsc->common_var.filefd);
        if (rsize > 0) {
            ByteArrayClear(item->request_byte);
            ByteArrayAppendChar(item->request_byte, UDS_SERVICES_TD);
            ByteArrayAppendChar(item->request_byte, udsc->common_var.fileTransferCount);
            ByteArrayAppendNChar(item->request_byte, udsc->common_var.td_buff, rsize);
        }
        else {
            log_e("Failed to read the file. The planned read %d bytes was actually read %d bytes \n", \
                udsc->common_var.maxNumberOfBlockLength - 2, (int)rsize);
        }
    }
}
/* ����ǰ�������� */
static void udsc_service_request_handler(uds_client *udsc)
{
    INT32 sentbyte = -1;
    BOOLEAN isfail = false;
    service_item *item = NULL;

    /* û���������Ҫ����ֱ�ӷ��� */
    if (!udsc_service_isactive(udsc)) {
        log_w("The service request has been stopped \n");
        return ;
    }
    log_d("UDS Service request start \n");
    /* ˢ����������߷��Ͷ�ʱ�� */
    if (udsc->tpEnable && udsc->isTpRefresh) {
        udsc_service_tester_present_refresh(udsc);
    }
    do {
        isfail = false; /* ���ñ�־λ */
        item = udsc_active_service_item(udsc);
        /* �жϵ�ǰ������Ƿ���Ҫִ�� */
        if (item->isenable) {
            log_d("Execute the diagnostic service item %s \n", item->desc ? item->desc : " ");
            /* Ԥ�ȴ�������������� */
            udsc_service_request_preprocessor(udsc);
            /* �ҵ���Ҫִ�е���Ϸ������ʼִ�������Ϸ����� */   
            if (udsc->sent_callback) {
                sentbyte = udsc->sent_callback(udsc->id, udsc->sent_arg, \
                    ByteArrayConstData(item->request_byte), ByteArrayCount(item->request_byte), \
                    item->sa, item->ta, item->timeout);
            }          
            /* ��Ϸ���������ʧ�� */
            if (sentbyte != ByteArrayCount(item->request_byte)) {
                log_e("Sent UDS Service request fail %d \n", sentbyte);
                const unsigned char *msg = ByteArrayConstData(item->request_byte);
                int msglen = ByteArrayCount(item->request_byte);
                log_hex_e("UDS Service request", msg, msglen);
                isfail = true;
            }
            if (isfail == false) {
                /* ���ͳɹ�����Ӧ��ʱ��ʱ�� */
               udsc_service_response_start(udsc);
            }
            if (isfail == false || udsc->isFailAbort) {
                /* ��Ϸ���ִ�гɹ����߱����ִ��ʧ�ܼ��������˳�������һ������� */
                goto REQUEST_FINISH;
            }
        }
        udsc_next_service_item(udsc); /* ������һ��������Ƿ����ִ�� */
    } while (udsc_service_isactive(udsc));

REQUEST_FINISH:
    if (isfail == false) { /* �����ִ�гɹ� */
        if (!udsc_service_isactive(udsc)) {/* ���е�������Ѿ������� */
            log_d("All diagnostic service items are executed finish \n");
            if (udsc->finish_callback) {
                /* �������������ִ�н��� */
                udsc->finish_callback(udsc, UDSC_NORMAL_FINISH, udsc->finish_arg, 0, 0, 0, 0);
            }            
            udsc_services_stop(udsc); /* ֹͣ�������������ִ�� */
        }
    }
    else {
        /* ��Ϸ���ִ��ʧ�� */  
        log_d("diagnostic service items are executed error \n");
        if (udsc->isFailAbort) {
            log_d("diagnostic service items are executed error finish \n");
            if (udsc->finish_callback) {                
                item = udsc_active_service_item(udsc);
                udsc->finish_callback(udsc, UDSC_SENT_ERROR_FINISH, udsc->finish_arg, \
                    ByteArrayConstData(item->request_byte), ByteArrayCount(item->request_byte), 0, 0);
            }            
            udsc_services_stop(udsc); /* ֹͣ�������������ִ�� */
        }
    }
    
    return ;
}

static void udsc_service_27_sa_response_handler(uds_client *udsc, INT8U *data, INT32U size)
{
    int byteoffset = 0;
    service_item *item = udsc_active_service_item(udsc);
    INT8U seed[SA_KEY_SEED_SIZE] = {0};    
    INT8U key[SA_KEY_SEED_SIZE] = {0};
    unsigned short seed_size = 0;
    unsigned short key_size = sizeof(key);

    ; byteoffset++;
    INT8U sub = data[byteoffset]; byteoffset++;
    if (sub % 2) { /* ������������ */
        memcpy(seed, &data[byteoffset], MIN(sizeof(seed), size - byteoffset));
        log_hex_d("Request seed ", seed, seed_size);
        ByteArrayClear(udsc->common_var.key_byte);
        if (item->ac_27.key_callback) {
            item->ac_27.key_callback(seed, sizeof(seed), sub, (const char *)udsc->id, key, &key_size);
            if (key_size <= sizeof(key)) {
                log_hex_d("Request key ", key, key_size);
                ByteArrayAppendNChar(udsc->common_var.key_byte, key, key_size);
            }
        }
        if (udsc->saseed_callback) {
            /* ����ΪOTA MASTERר�����õĻص�������OTA MASTER�����ӷ��͸�OTA MASTER API��������Key */
            udsc->saseed_callback(udsc->saseed_arg, udsc->id, sub, seed, seed_size);
        }
    }
}

static void udsc_service_36_td_set(uds_client *udsc)
{
    struct stat file_info;
    service_item *item = udsc_active_service_item(udsc);

    if (item->td_36.local_path == 0) {
        log_e("item->td_36.local_path is null \n");
        return ;
    }

    if (udsc->common_var.maxNumberOfBlockLength == 0) {
        log_e("maxNumberOfBlockLength value zero error \n");
        udsc->response_meet_expect = false;
        return ;
    }
    if (udsc->common_var.td_buff) {
        DM_FREE(udsc->common_var.td_buff);
        udsc->common_var.td_buff = NULL;
    }
        
    udsc->common_var.td_buff = DM_MALLOC(udsc->common_var.maxNumberOfBlockLength);
    if (udsc->common_var.td_buff == NULL) {
        log_e("Malloc size %d failed \n", udsc->common_var.maxNumberOfBlockLength);
        udsc->response_meet_expect = false;
        return ;
    }
    
    /* �ļ��Ѿ��򿪹ر��ļ� */
    if (udsc->common_var.filefd) {
        fclose(udsc->common_var.filefd);
        udsc->common_var.filefd = NULL;
    }
    /* �ļ����ڴ��ļ� */
#ifdef _WIN32
    errno_t err = fopen_s(&udsc->common_var.filefd, "file.txt", "r");
#else /* #ifdef _WIN32 */
    udsc->common_var.filefd = fopen(item->td_36.local_path, "rb");
#endif /* #ifdef _WIN32 */  
    if (!udsc->common_var.filefd) {
        log_e("Failed to open the transfer file(%s) \n", item->td_36.local_path);
        udsc->response_meet_expect = false;
        return ;
    }
    log_d("Open file => %s success \n", item->td_36.local_path);
    
    udsc->common_var.fileTransferCount = 1; /* ���к�1��ʼ */
    udsc->common_var.fileTransferTotalSize = 0;
    if (stat(item->td_36.local_path, &file_info) != 0) {
        log_e("Failed to obtain the file(%s) size \n", item->td_36.local_path);
        udsc->response_meet_expect = false;
        return ;
    }
    udsc->common_var.fileSize = file_info.st_size;
    log_d("fileTransferCount: %d \n", udsc->common_var.fileTransferCount);    
    log_d("fileTransferTotalSize: %d \n", udsc->common_var.fileTransferTotalSize);    
    log_d("fileSize: %d \n", udsc->common_var.fileSize);    
    log_d("maxNumberOfBlockLength: %d \n", udsc->common_var.maxNumberOfBlockLength);
}

static void udsc_service_38_rft_response_handler(uds_client *udsc, INT8U *data, INT32U size)
{
    int byteoffset = 0;
    int nbyte = 0;

    ; byteoffset++;
    INT8U modeOfOperation = data[byteoffset]; byteoffset++;
    if (modeOfOperation == UDS_RFT_MODE_ADD_FILE || \
        modeOfOperation == UDS_RFT_MODE_REPLACE_FILE) {
        byteoffset++;
        INT8U lengthFormatIdentifier = data[byteoffset]; byteoffset++;
        for (nbyte = 0; nbyte < lengthFormatIdentifier; nbyte++) {
            INT8U byte = 0;
    
            byte = data[byteoffset]; byteoffset++;
            udsc->common_var.maxNumberOfBlockLength |=  byte << ((lengthFormatIdentifier - nbyte - 1) * 8);
        }
        INT8U dataFormatIdentifier = data[byteoffset]; byteoffset++;
        Y_UNUSED(dataFormatIdentifier);
    }
    log_d("maxNumberOfBlockLength => %d \n", udsc->common_var.maxNumberOfBlockLength);
    udsc_service_36_td_set(udsc);
}

static void udsc_service_34_rd_response_handler(uds_client *udsc, INT8U *data, INT32U size)
{
    int byteoffset = 0;
    int nbyte = 0;

    ; byteoffset++; /* ������SID */

    INT8U lengthFormatIdentifier = data[byteoffset] >> 4 & 0x0f; byteoffset++;
    for (nbyte = 0; nbyte < lengthFormatIdentifier; nbyte++) {
        INT8U byte = 0;

        byte = data[byteoffset]; byteoffset++;
        udsc->common_var.maxNumberOfBlockLength |=  byte << ((lengthFormatIdentifier - nbyte - 1) * 8);
    }
    log_d("maxNumberOfBlockLength => %d \n", udsc->common_var.maxNumberOfBlockLength);
    udsc_service_36_td_set(udsc);
}

static void udsc_service_36_td_response_handler(uds_client *udsc, INT8U *data, INT32U size)
{
    udsc->common_var.fileTransferCount++;
    udsc->common_var.fileTransferTotalSize += (udsc->common_var.maxNumberOfBlockLength - 2);
    log_d("fileTransferTotalSize => %d fileSize => %d \n", \
            udsc->common_var.fileTransferTotalSize, udsc->common_var.fileSize);
    if (udsc->common_var.fileTransferTotalSize >= udsc->common_var.fileSize) {
        log_d("The file has been transferred fileTransferTotalSize => %d fileSize => %d \n", \
            udsc->common_var.fileTransferTotalSize, udsc->common_var.fileSize);
        /* �ļ��Ѿ��򿪹ر��ļ� */
        if (udsc->common_var.filefd) {
            fclose(udsc->common_var.filefd);
            udsc->common_var.filefd = NULL;
        }
    }
    else {
        /* �ļ�����δ��ɣ����������ļ� */
        udsc->iskeep = true;
    }
}

static void udsc_service_37_rte_response_handler(uds_client *udsc, INT8U *data, INT32U size)
{
    /* �ļ��Ѿ��򿪹ر��ļ� */
    if (udsc->common_var.filefd) {
        fclose(udsc->common_var.filefd);
        udsc->common_var.filefd = NULL;
    }
}

static void udsc_service_10_dsc_response_handler(uds_client *udsc, INT8U *data, INT32U size)
{
    udsc->common_var.p2_server_max = data[2] << 8 | data[3];
    udsc->common_var.p2_e_server_max = data[4] << 8 | data[5];
    log_d("p2_server_max => %d \n", udsc->common_var.p2_server_max);
    log_d("p2_e_server_max => %d \n", udsc->common_var.p2_e_server_max);
}

static void udsc_service_response_handler(uds_client *udsc, service_response_stat stat, INT32U sa, INT32U ta, INT8U *data, INT32U size)
{
    INT8U rsid = 0;

    if (!data) {
        return ;
    }

    /* ��������Ӧ����������Ӧ */
    rsid = data[0] == 0x7f ? data[1] : data[0] & (~UDS_REPLY_MASK);
    log_d("Service (%s) response \n", dm_desc_uds_services(rsid));
    switch (rsid) {
        case UDS_SERVICES_SA:
            udsc_service_27_sa_response_handler(udsc, data, size);
            return ;
        case UDS_SERVICES_RFT:
            udsc_service_38_rft_response_handler(udsc, data, size);    
            return ;
        case UDS_SERVICES_RD:
            udsc_service_34_rd_response_handler(udsc, data, size);  
            return ;
        case UDS_SERVICES_TD:
            udsc_service_36_td_response_handler(udsc, data, size);
            return ;
        case UDS_SERVICES_RTE:
            udsc_service_37_rte_response_handler(udsc, data, size);
            return ;
        case UDS_SERVICES_DSC:
            udsc_service_10_dsc_response_handler(udsc, data, size);            
            return ;
        default:

            break;
    }
}

static void udsc_service_match_response_expect(uds_client *udsc, service_response_stat stat, INT32U sa, INT32U ta, INT8U *data, INT32U size)
{
    service_item *item = udsc_active_service_item(udsc);

    if (stat == SERVICE_RESPONSE_NORMAL && \
        (item->response_rule == NO_RESPONSE_EXPECT || \
         item->issuppress == true)) {            
        udsc->response_meet_expect = false;
        log_e("Expected response %d, the actual response timed out. \n", item->response_rule);
        return ;
    }

    if (item->response_rule == NOT_SET_RESPONSE_EXPECT) {

    }
    else if (item->response_rule == POSITIVE_RESPONSE_EXPECT) {    
        if (data == NULL || size == 0) {
            udsc->response_meet_expect = false;
            log_w("Response matching error \n");
            return ;
        }
        if ((data[0] & (~UDS_REPLY_MASK)) != item->sid) {
            udsc->response_meet_expect = false;
            log_d("Expected response Positive response, actual negative response \n");
            return ;   
        } 
    }
    else if (item->response_rule == NEGATIVE_RESPONSE_EXPECT) {    
        if (data == NULL || size == 0) {
            udsc->response_meet_expect = false;
            log_w("Response matching error \n");
            return ;
        }    
        if (data[0] != 0x7f) {
            udsc->response_meet_expect = false;
            log_d("Expected response negative response, actual positive response \n");
            return ;   
        } 
    }
    else if (item->response_rule == MATCH_RESPONSE_EXPECT) {
        if (data == NULL || size == 0) {
            udsc->response_meet_expect = false;
            log_w("Response matching error \n");
            return ;
        }
    
        if (!ByteArrayCharEqual(item->expect_byte, data, size)) {
           udsc->response_meet_expect = false;
           log_hex_d("expect response", item->expect_byte->data, item->expect_byte->dlen);
           log_hex_d("actual response", data, size);
           return ;
        }
    }
    else if (item->response_rule == NO_RESPONSE_EXPECT) {
        if (data && size > 0) {
            udsc->response_meet_expect = false;
            log_d("Expected no response \n");
            log_hex_d("actual response", data, size);
            return ;
        }
    }
}

static void udsc_service_match_finish_rule(uds_client *udsc, service_response_stat stat, INT32U sa, INT32U ta, INT8U *data, INT32U size)
{
    service_item *item = udsc_active_service_item(udsc);

    if (item->finish_rule == FINISH_DEFAULT_SETTING) {
        /* ignore */
    }
    else if (item->finish_rule == FINISH_EQUAL_TO) {
        /* ����Ԥ�ڽ���Ӧ��ͽ��� */
        if (ByteArrayCharEqual(item->finish_byte, data, size) == 0) {
            /* û������Ԥ�ڽ�����Ӧ */
            udsc->iskeep = true;
        }
        item->finish_try_num++;
        if (item->finish_try_num > item->finish_num_max) {
            udsc->iskeep = false;
        }
    }
    else if (item->finish_rule == FINISH_UN_EQUAL_TO) {        
        /* û������Ԥ�ڽ���Ӧ��ͽ��� */
        if (ByteArrayCharEqual(item->finish_byte, data, size) == 1) {        
            /* ������Ԥ�ڽ�����Ӧ */
            udsc->iskeep = true;
        }        
        item->finish_try_num++;
        if (item->finish_try_num > item->finish_num_max) {
            udsc->iskeep = false;
        }
    }
}

static void udsc_service_response_finish(uds_client *udsc, service_response_stat stat, INT32U sa, INT32U ta, INT8U *data, INT32U size)
{
    INT32U fcode = UDSC_NORMAL_FINISH;

    /* û���������Ҫ����ֱ�ӷ��� */
    if (!udsc_service_isactive(udsc)) {        
        log_w("The service request has been stopped \n");
        return ;
    }

    if (stat == SERVICE_RESPONSE_NORMAL) {
        log_hex_d("Recv response: ", data, size);
    }
    else {
        log_d("Recv response timeout \n");
    }

    /* ���һ���Ƿ������Ӧ������ */
    for ( ; stat == SERVICE_RESPONSE_NORMAL; ) {
        service_item *item = udsc_active_service_item(udsc);
        if(data[0] == 0x7f && data[1] == item->sid) {
            break; /* for */
        }
        if (data[0] == (item->sid | UDS_REPLY_MASK)) {            
            break; /* for */
        }        
        log_w("The response does not meet current service item expectations \n");        
        log_w("data[0] = %02x data[1] = %02x item->sid = %02x \n", data[0], data[1], item->sid);
        return ;
    }                  
    /* �յ�NRC 78hˢ��Ӧ��ʱ��ʱ�� */
    if (size == 3) {
        if (data[0] == 0x7f && \
            data[2] == UDS_RESPONSE_CODES_RCRRP) {
            udsc_service_response_refresh(udsc);
            return ;
        }
    }
    
    udsc_service_response_stop(udsc); /* �Ѿ��յ�Ӧ��ֹͣӦ��ʱ��ʱ�� */
    
    udsc->response_meet_expect = true; /* �ȱ��Ӧ���Ƿ���Ҫ��� */
    udsc->iskeep = false; /* Ԥ�ȱ�ǲ���ִ�е�ǰ������ */

    /* ���Ӧ���Ƿ���Ͻ������� */
    udsc_service_match_finish_rule(udsc, stat, sa, ta, data, size);

    /* ���Ӧ���Ƿ����Ԥ��Ӧ�� */
    udsc_service_match_response_expect(udsc, stat, sa, ta, data, size);
    if (udsc->response_meet_expect == false && \
        udsc->isFailAbort) {
        /* Ӧ������ն�ִ��  */
        goto RESPONSE_FINISH;
    }
        
    /* �����յ���Ӧ���ݣ�������Ӧ���� */
    if (stat == SERVICE_RESPONSE_NORMAL) {
        udsc_service_response_handler(udsc, stat, sa, ta, data, size);
    }
    if (udsc->response_meet_expect == false && \
        udsc->isFailAbort) {
        /* Ӧ������ն�ִ��  */
        goto RESPONSE_FINISH;
    }

    if (udsc->iskeep) {
        /* ����ִ�е�ǰ����� */
        service_item *item = udsc_active_service_item(udsc);
        item->finish_try_num++;
        /* ������ʱ��ʱ������ʼִ����Ϸ����� */
        udsc_service_request_start(udsc);
        udsc->iskeep = false; /* ���ñ�־ */
    }
    else {
        /* ������һ��ʹ�ܵ���Ϸ����� */
        udsc_next_service_item(udsc);
        while (udsc_service_isactive(udsc)) {
            service_item *item = udsc_active_service_item(udsc);
            /* �жϵ�ǰ������Ƿ���Ҫִ�� */
            if (item->isenable) {
                /* ������ʱ��ʱ������ʼִ����Ϸ����� */
                udsc_service_request_start(udsc);
                break; /* break while */
            }
            udsc_next_service_item(udsc); /*  */
        }
        /* ���е�������Ѿ������� */
        if (!udsc_service_isactive(udsc)) {
            log_d("All diagnostic service items are executed finish \n");
            if (udsc->finish_callback) {
                /* �������������ִ�н����ص����������� */
                udsc->finish_callback(udsc, fcode, udsc->finish_arg, 0, 0, 0, 0);
            }            
            udsc_services_stop(udsc); /* ֹͣ�������������ִ�� */
        }
    }
    
RESPONSE_FINISH:
    if (udsc->response_meet_expect == false) {
        /* Ӧ�𲻷���Ԥ�� */
        log_d("The service response is not as expected \n");
        if (udsc->isFailAbort) {
            log_d("diagnostic service items are executed error finish \n");
            if (udsc->finish_callback) {
                if (stat == SERVICE_RESPONSE_NORMAL) {
                    fcode = UDSC_UNEXPECT_RESPONSE_FINISH; /* ���յ��������������Ԥ�����õ�ֵ */
                } else {
                    fcode = UDSC_TIMEOUT_RESPONSE_FINISH; /* ���Ӧ��ʱ */
                }
                log_d("UDS services finish callback \n");
                service_item *item = udsc_active_service_item(udsc);
                udsc->finish_callback(udsc, fcode, udsc->finish_arg, \
                    ByteArrayConstData(item->request_byte), ByteArrayCount(item->request_byte), data, size);
            }            
            udsc_services_stop(udsc); /* ֹͣ�������������ִ�� */
        }
    }
}

void dm_udsc_service_response_finish(uds_client *udsc, service_response_stat stat, INT32U sa, INT32U ta, INT8U *data, INT32U size)
{
    udsc_service_response_finish(udsc, stat, sa, ta, data, size);
}

static void udsc_service_item_destory(service_item **item)
{
    if ((*item)) {
        if ((*item)->desc) {
            DM_FREE((*item)->desc);
            (*item)->desc = NULL;
        }
        ByteArrayDelete((*item)->request_byte);
        ByteArrayDelete((*item)->expect_byte);
        ByteArrayDelete((*item)->finish_byte);
        ByteArrayDelete((*item)->variable_byte);
        if ((*item)->td_36.local_path) {
            DM_FREE((*item)->td_36.local_path);
        }
        DM_FREE((*item));
        (*item) = NULL;
    }
}

#ifdef __HAVE_LIBEV__
static void udsc_service_handler_callback(struct ev_loop *loop, ev_timer *w, int revents)
{
#ifdef _WIN32
    uds_client* udsc = (uds_client*)((char*)w - offsetof(uds_client, request_watcher));
#else /*_WIN32 */
    uds_client *udsc = container_of(w, uds_client, request_watcher); 
#endif /* _WIN32 */
    ev_timer_stop(loop, w);
    udsc_service_request_handler(udsc);    
}
static void udsc_response_timeout_callback(struct ev_loop *loop, ev_timer *w, int revents)
{
#ifdef _WIN32
    uds_client* udsc = (uds_client*)((char*)w - offsetof(uds_client, response_watcher));
#else /*_WIN32 */
    uds_client* udsc = container_of(w, uds_client, response_watcher);
#endif /* _WIN32 */
    ev_timer_stop(loop, w);
    INT8U data[8] = {0};
    udsc_service_response_finish(udsc, SERVICE_RESPONSE_TIMEOUT, 0, 0, data, 0);   
}
static void udsc_tester_present_callback(struct ev_loop *loop, ev_timer *w, int revents)
{
#ifdef _WIN32
    uds_client* udsc = (uds_client*)((char*)w - offsetof(uds_client, testerPresent_watcher));
#else /*_WIN32 */
    uds_client* udsc = container_of(w, uds_client, testerPresent_watcher);
#endif /* _WIN32 */
    if (udsc->sent_callback) {
        udsc->sent_callback(udsc->id, udsc->sent_arg, (INT8U *)"\x3e\x80", 2, udsc->tpsa, udsc->tpta, 0);
    }
}

#endif /* __HAVE_LIBEV__ */

#ifdef __HAVE_LIBUV__
static void udsc_service_handler_callback(uv_timer_t* w)
{
#ifdef _WIN32
    uds_client* udsc = (uds_client*)((char*)w - offsetof(uds_client, request_watcher));
#else /*_WIN32 */
    uds_client* udsc = container_of(w, uds_client, request_watcher);
#endif /* _WIN32 */
    uv_timer_stop(w);
    udsc_service_request_handler(udsc);
}
static void udsc_response_timeout_callback(uv_timer_t* w)
{
#ifdef _WIN32
    uds_client* udsc = (uds_client*)((char*)w - offsetof(uds_client, response_watcher));
#else /*_WIN32 */
    uds_client* udsc = container_of(w, uds_client, response_watcher);
#endif /* _WIN32 */
    uv_timer_stop(w);
    INT8U data[8] = { 0 };
    udsc_service_response_finish(udsc, SERVICE_RESPONSE_TIMEOUT, 0, 0, data, 0);
}
static void udsc_tester_present_callback(uv_timer_t* w)
{
#ifdef _WIN32
    uds_client* udsc = (uds_client*)((char*)w - offsetof(uds_client, testerPresent_watcher));
#else /*_WIN32 */
    uds_client* udsc = container_of(w, uds_client, testerPresent_watcher);
#endif /* _WIN32 */
    if (udsc->sent_callback) {
        udsc->sent_callback(udsc->id, udsc->sent_arg, (INT8U*)"\x3e\x80", 2, udsc->tpsa, udsc->tpta, 0);
    }
}

#endif /* __HAVE_LIBUV__ */

void udsc_reset(uds_client *udsc)
{
    int ii = 0;

    for (ii = 0; ii < udsc->service_cnt; ii++) {
        service_item *item = udsc->service_items[ii];
        if (item) {
            udsc_service_item_destory(&item);
            udsc->service_items[ii] = 0;
        }
    }
    udsc->cmd_valid_time = IPC_CAMMAND_VALID_TIME;

    udsc->isFailAbort = false; /* ������������������ʱ����ִֹ�з���������� */
    udsc->response_meet_expect = false; /* Ӧ�����Ԥ��Ҫ�� */
    udsc->sindex = -1; /* ��ǰ���д������Ϸ��������� service_items[sindex] */
    udsc->iskeep = false; /* �Ƿ����ִ�е�ǰ��Ϸ����Ĭ�ϲ�����֧�ֵ�ǰ������й�����ȷ�� */
    udsc->service_cnt = 0; /* ��Ϸ������� */
    udsc->sent_arg = NULL;
    udsc->sent_callback = NULL; /* �����Ϣ���ͺ���������Ҫ������������ʱ����� */
    udsc->finish_arg = NULL;
    udsc->finish_callback = NULL; /* ��Ͽͻ��˽������� */

    udsc->common_var.fileTransferCount = 0;
    udsc->common_var.fileTransferTotalSize = 0;
    udsc->common_var.fileSize = 0;
    if (udsc->common_var.td_buff) {
        DM_FREE(udsc->common_var.td_buff);
        udsc->common_var.td_buff = NULL;
    }

    if (udsc->common_var.filefd) {
        fclose(udsc->common_var.filefd);
        udsc->common_var.filefd = NULL;
    }
    udsc->common_var.maxNumberOfBlockLength = 0;
    ByteArrayClear(udsc->common_var.key_byte);
    ByteArrayClear(udsc->common_var.seed_byte);

    udsc->tpEnable = false; /* ʹ����������������� */
    udsc->isTpRefresh = false; /* �Ƿ�UDS����ˢ�¶�ʱ�� */
    udsc->tpInterval = 0; /* ���ͼ�� unit ms */
    udsc->tpta = 0; /* ���Ŀ�ĵ�ַ */
    udsc->tpsa = 0; /* ���Դ��ַ */
    
    udsc->isidle = true;
}

#ifdef __HAVE_LIBUV__
void dm_udsc_ev_loop_set(uds_client *udsc, uv_loop_t *loop)
{
    udsc->loop = loop;
    uv_timer_init(loop, &udsc->request_watcher);
    uv_timer_init(loop, &udsc->response_watcher);
    uv_timer_init(loop, &udsc->testerPresent_watcher);
}
#else /* __HAVE_LIBUV__ */
void dm_udsc_ev_loop_set(uds_client* udsc, struct ev_loop* loop)
{
    udsc->loop = loop;
}
#endif /* __HAVE_LIBUV__ */
/* ��������ʼ��uds�ͻ��� */
uds_client *dm_udsc_create()
{
    uds_client *udsc = DM_MALLOC(sizeof(*udsc));
    if (udsc == NULL) {
        return NULL;
    }
    memset(udsc, 0, sizeof(*udsc));

#ifdef __HAVE_LIBEV__
    udsc->loop = EV_DEFAULT; /* ʹ���¼�ѭ��Ĭ������ */
    /* �����������ʱ�� */
    ev_timer_init(&udsc->request_watcher, udsc_service_handler_callback, 0.0, 0.0);
    /* ����Ӧ��ʱ��ʱ�� */
    ev_timer_init(&udsc->response_watcher, udsc_response_timeout_callback, 0.0, 0.0);    
    /* ����������� */
    ev_timer_init(&udsc->testerPresent_watcher, udsc_tester_present_callback, 0.0, 0.0);
#endif /* __HAVE_LIBEV__ */  
    udsc->sindex = -1; /* ��ǰ��Ҫִ�е������С��0��ʾû����Ҫִ�� */
    udsc->isFailAbort = false; /* Ĭ����Ϸ����������ֹ�������� */
    udsc->service_cnt = 0; /* ��Ϸ����б������ܺ� */
    /* ������Ϸ����б����� */
    udsc->service_size = SERVICE_ITEM_SIZE_MAX; 
    udsc->service_items = DM_CALLOC(sizeof(service_item *), udsc->service_size + 1 /* Ԥ��һ�������Զ����ʹ�� */);
    if (udsc->service_items == 0) {
        log_e("calloc(sizeof(service_item *), udsc->service_size) error. \n");
        DM_FREE(udsc);
        return NULL;
    }

    udsc->common_var.seed_byte = ByteArrayNew();
    udsc->common_var.key_byte = ByteArrayNew();

    udsc->common_var.filefd = NULL;
    udsc->common_var.td_buff = NULL;

    udsc_reset(udsc);

    return udsc;
}

/* ��UDS�ͻ��˸�ԭ�ɸմ���ʱ���״̬ */
BOOLEAN dm_udsc_reset(uds_client *udsc) 
{
    if (udsc_service_isactive(udsc)) return false;
    
    udsc_reset(udsc);

    return true;
}

/* ����uds�ͻ��� */
BOOLEAN dm_udsc_destory(uds_client *udsc)
{
    if (udsc_service_isactive(udsc)) {
        return false;
    }
    
    udsc_reset(udsc);

    DM_FREE(udsc->service_items);
    udsc->service_items = NULL;
    
    if (udsc->common_var.td_buff) {
        DM_FREE(udsc->common_var.td_buff);
        udsc->common_var.td_buff = NULL;
    }

    if (udsc->common_var.filefd) {
        fclose(udsc->common_var.filefd);
        udsc->common_var.filefd = NULL;
    }
    
    ByteArrayDelete(udsc->common_var.seed_byte);
    ByteArrayDelete(udsc->common_var.key_byte);

    DM_FREE(udsc);

    return true;
}

/*
    ��Ϸ������Ƿ���ִ����
*/
BOOLEAN dm_udsc_service_isactive(uds_client *udsc)
{
    if (udsc == NULL) {
        log_e("The fatal error parameter cannot be null. \n");
        return false;
    }

    return udsc_service_isactive(udsc);
}

/*
    ��ʼִ����Ϸ�����, ��������һ��ʹ�ܵ���Ϸ�����󷵻�true
*/
BOOLEAN dm_udsc_services_start(uds_client *udsc)
{
    BOOLEAN start_succ = false;

    if (udsc_service_isactive(udsc)) {
        return start_succ;
    }
    udsc->sindex = 0;
    while (udsc->sindex < udsc->service_cnt) {
        service_item *item = udsc->service_items[udsc->sindex];
        /* �жϵ�ǰ������Ƿ���Ҫִ�� */
        if (item && item->isenable) {
            /* ������ʱ��ʱ������ʼִ����Ϸ����� */
            udsc_service_request_start(udsc);
            start_succ = true;
            break;
        }
        udsc->sindex++;
    }
    /* ��Ҫ������������������� */
    if (start_succ && udsc->tpEnable) {        
        udsc_service_tester_present_start(udsc);
    }

    return start_succ;
}

/*
    ִֹͣ����Ϸ�����
*/
BOOLEAN dm_udsc_services_stop(uds_client *udsc)
{
    return udsc_services_stop(udsc);
}

/* �����б��еķ�����ʧ���Ƿ���ֹ�������� */
void dm_udsc_service_fail_abort(uds_client *udsc, BOOLEAN b)
{
    udsc->isFailAbort = b;
}

/*
    ����һ���µ���Ϸ�����������������������ڵ�����
*/
service_item *dm_udsc_service_item_add(uds_client *udsc, char *desc)
{
    service_item *nitem = NULL;

    if (!(udsc->service_cnt < udsc->service_size)) {
        /* �������б������޷�������� */
        log_w("The capacity of the service item has reached the maximum value => %d \n", udsc->service_size);
        return NULL;
    }
    nitem = DM_CALLOC(sizeof(service_item), 1);
    if (nitem == NULL) {
        log_e("calloc(sizeof(service_item), 1) error. \n");
        return NULL;
    }
    memset(nitem, 0, sizeof(*nitem));
    /* ���汣�� */
    udsc->service_items[udsc->service_cnt] = nitem;
    udsc->service_cnt++;
    nitem->isenable = true;
    nitem->tatype = PHYSICAL_ADDRESS;
    nitem->response_rule = NOT_SET_RESPONSE_EXPECT;
    nitem->finish_rule = FINISH_DEFAULT_SETTING;
    nitem->request_byte = ByteArrayNew();
    nitem->expect_byte = ByteArrayNew();
    nitem->finish_byte = ByteArrayNew();
    nitem->variable_byte = ByteArrayNew();
    
    if (desc) {
        /* ���������ַ���s */
        nitem->desc = DM_MALLOC(strlen(desc) + 1);
        if (nitem->desc) {
#ifdef _WIN32
            strcpy_s(nitem->desc, strlen(desc) + 1, desc);
#else /* #ifdef _WIN32 */
            strcpy(nitem->desc, desc);
#endif /* #ifdef _WIN32 */
            nitem->desc[strlen(desc)] = '\0';
        }
    }
    log_d("Add current service item count => %d \n", udsc->service_cnt);
    
    return nitem;
}

/*
    ɾ������Ϸ�����ᱻɾ����item������ʹ����
*/
void dm_udsc_service_item_del(uds_client *udsc, service_item *item)
{
    int ii = 0;
    BOOLEAN isfound = false;

    if (item == NULL) return ;

    for (ii = 0; udsc_service_item_index_valid(udsc, ii); ii++) {
        if (!isfound) {
            if (udsc->service_items[ii] == item) {
                isfound = true;
                log_d("Del item %d => %s \n", ii, item->desc);
                udsc_service_item_destory(&item);
                udsc->service_items[ii] = udsc->service_items[ii + 1];
            }
        }
        else {
            log_d("move item %d  \n", ii);
            udsc->service_items[ii] = udsc->service_items[ii + 1];
        }
    }
    if (isfound) {
        udsc->service_cnt--;
    }
    log_d("Del current service item count => %d \n", udsc->service_cnt);
 }

BOOLEAN dm_udsc_loop_stop(uds_client *udsc)
{
    udsc_services_stop(udsc);
    
    return true;
}

/* 
    ��ʼuds�ͻ��˵��߳�ѭ���������һ�ξͿ��� 
*/
BOOLEAN dm_udsc_thread_loop_start(uds_client *udsc)
{
    return true;
}

/* 
    ��ʼuds�ͻ��˵��¼�ѭ��������Ҫѭ�����õ�������������� 
*/
BOOLEAN dm_udsc_event_loop_start(uds_client *udsc)
{

    return true;
}

void dm_udsc_request_sent_callback_set(uds_client *udsc, udsc_request_sent_callback sent_callback, void *arg)
{
    udsc->sent_callback = sent_callback;
    udsc->sent_arg = arg;
}

void dm_udsc_services_finish_callback_set(uds_client *udsc, udsc_services_finish_callback finish_callback, void *arg)
{
    udsc->finish_callback = finish_callback;
    udsc->finish_arg = arg;
}

BOOLEAN dm_udsc_service_request_build(service_item *sitem)
{
    int n = 0;

    ByteArrayClear(sitem->request_byte);

    /* append sid */
    ByteArrayAppendChar(sitem->request_byte, sitem->sid);
    if (udsc_service_have_sub(sitem->sid)) {
        /* append subfunction */
        if (sitem->issuppress) {
            ByteArrayAppendChar(sitem->request_byte, sitem->sub | UDS_SUPPRESS_POS_RSP_MSG_IND_MASK);
        }
        else {
            ByteArrayAppendChar(sitem->request_byte, sitem->sub);
        }
    }

    /* append did */
    if (sitem->sid == UDS_SERVICES_RDBI || \
        sitem->sid == UDS_SERVICES_WDBI || \
        sitem->sid == UDS_SERVICES_RC) {
        ByteArrayAppendChar(sitem->request_byte, sitem->did >> 8 & 0xff);
        ByteArrayAppendChar(sitem->request_byte, sitem->did & 0xff);
    }
    else if (sitem->sid == UDS_SERVICES_RFT) {
        ByteArrayAppendChar(sitem->request_byte, sitem->rft_38.modeOfOperation);
        ByteArrayAppendChar(sitem->request_byte, sitem->rft_38.filePathAndNameLength >> 8 & 0xff);
        ByteArrayAppendChar(sitem->request_byte, sitem->rft_38.filePathAndNameLength & 0xff);
        ByteArrayAppendNChar(sitem->request_byte, (unsigned char *)sitem->rft_38.filePathAndName, strlen(sitem->rft_38.filePathAndName));
        ByteArrayAppendChar(sitem->request_byte, sitem->rft_38.dataFormatIdentifier);
        ByteArrayAppendChar(sitem->request_byte, sitem->rft_38.fileSizeParameterLength);
        for (n = sitem->rft_38.fileSizeParameterLength - 1; n >= 0; n--) {
            ByteArrayAppendChar(sitem->request_byte, (sitem->rft_38.fileSizeUnCompressed >> (8 * n)) & 0xff);
        }
        for (n = sitem->rft_38.fileSizeParameterLength - 1; n >= 0; n--) {
            ByteArrayAppendChar(sitem->request_byte, (sitem->rft_38.fileSizeCompressed >> (8 * n)) & 0xff);
        }
    }
    else if (sitem->sid == UDS_SERVICES_RD) {
        ByteArrayAppendChar(sitem->request_byte, sitem->rd_34.dataFormatIdentifier);
        ByteArrayAppendChar(sitem->request_byte, sitem->rd_34.addressAndLengthFormatIdentifier);
        for (n = (sitem->rd_34.addressAndLengthFormatIdentifier >> 4 & 0x0f) - 1; n >= 0; n--) {
            ByteArrayAppendChar(sitem->request_byte, (sitem->rd_34.memoryAddress >> (8 * n)) & 0xff);
        }
        for (n = (sitem->rd_34.addressAndLengthFormatIdentifier & 0x0f) - 1; n >= 0; n--) {
            ByteArrayAppendChar(sitem->request_byte, (sitem->rd_34.memorySize >> (8 * n)) & 0xff);
        }
    }

    if (sitem->vb_callback) {
        sitem->vb_callback(sitem, sitem->variable_byte, sitem->vb_callback_arg);
    }
    
    /* append variable byte */
    ByteArrayAppendArray(sitem->request_byte, sitem->variable_byte);

    return true;
}

void dm_udsc_service_variable_byte_callback_set(service_item *sitem, service_variable_byte_callback vb_callback, void *arg)
{  
    sitem->vb_callback = vb_callback;
    sitem->vb_callback_arg = arg;
}

void dm_udsc_service_response_callback_set(service_item *sitem, service_response_callback response_callback, void *arg)
{   
    sitem->response_callback = response_callback;
    sitem->response_callback_arg = arg;
}

service_item *dm_udsc_curr_service_item(uds_client *udsc)
{
    if (!udsc_service_isactive(udsc)) {
        log_w("The UDS client is not started \n");
        return NULL;
    }

    return udsc_active_service_item(udsc);
}

void dm_udsc_service_sid_set(service_item *sitem, INT8U sid)
{
    sitem->sid = sid;
}

INT8U dm_udsc_service_sid_get(service_item *sitem)
{
    return sitem->sid;    
}

void dm_udsc_service_sub_set(service_item *sitem, INT8U sub)
{
    sitem->sub = sub;
}

INT8U dm_udsc_service_sub_get(service_item *sitem)
{
    return sitem->sub;
}

void dm_udsc_service_did_set(service_item *sitem, INT16U did)
{
    sitem->did = did;
}

INT16U dm_udsc_service_did_get(service_item *sitem)
{
    return sitem->did;
}

void dm_udsc_service_timeout_set(service_item *sitem, INT32 timeout)
{
    sitem->timeout = timeout;
}

void dm_udsc_service_delay_set(service_item *sitem, INT32 delay)
{
    sitem->delay = delay;
}

void dm_udsc_service_suppress_set(service_item *sitem, BOOLEAN b)
{
    sitem->issuppress = b;
}

void dm_udsc_service_enable_set(service_item *sitem, BOOLEAN b)
{
    sitem->isenable = b;
}

void dm_udsc_service_expect_response_set(service_item *sitem, serviceResponseExpect rule, INT8U *data, INT32U size)
{
    sitem->response_rule = rule;
    if (data && size > 0) {
        ByteArrayAppendNChar(sitem->expect_byte, data, size);
    }
}

void dm_udsc_service_key_set(service_item *sitem, DMKey key_callback)
{
    sitem->ac_27.key_callback = key_callback;
}

void dm_udsc_service_key_generate(uds_client *udsc, INT8U *key, INT16U key_size)
{
    ByteArrayAppendNChar(udsc->common_var.key_byte, key, key_size);
}

void dm_udsc_service_saseed_callback_set(uds_client *udsc, service_sa_seed_callback call, void *arg)
{
    udsc->saseed_callback = call;
    udsc->saseed_arg = arg;
}

void dm_udsc_doip_channel_bind(uds_client *udsc, void *doip_channel)
{
    udsc->doipc_channel = doip_channel;
}

void dm_udsc_doip_channel_unbind(uds_client *udsc)
{
    udsc->doipc_channel = 0;
}

void *dm_udsc_doip_channel(uds_client *udsc)
{
    return udsc->doipc_channel;
}
