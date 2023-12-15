#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <pthread.h>

#include "config.h"

#ifdef __HAVE_LIBEV__
#include "ev.h"
#endif /* __HAVE_LIBEV__ */

#include "dm_udsc_types.h"
#include "dm_common.h"       
#include "dm_doip_client.h"

#define DOIP_PORT                                  13400
#define DOIP_TLS_PORT                               3496

#define DOIP_GENERIC_NACK                          0x0000
#define DOIP_VEHICLE_IDENTIFICATION_REQ            0x0001
#define DOIP_VEHICLE_IDENTIFICATION_REQ_EID        0x0002
#define DOIP_VEHICLE_IDENTIFICATION_REQ_VIN        0x0003
#define DOIP_VEHICLE_ANNOUNCEMENT_MESSAGE          0x0004
#define DOIP_ROUTING_ACTIVATION_REQUEST            0x0005
#define DOIP_ROUTING_ACTIVATION_RESPONSE           0x0006
#define DOIP_ALIVE_CHECK_REQUEST                   0x0007
#define DOIP_ALIVE_CHECK_RESPONSE                  0x0008
#define DOIP_ENTITY_STATUS_REQUEST                 0x4001
#define DOIP_ENTITY_STATUS_RESPONSE                0x4002
#define DOIP_POWER_INFORMATION_REQUEST             0x4003
#define DOIP_POWER_INFORMATION_RESPONSE            0x4004
#define DOIP_DIAGNOSTIC_MESSAGE                    0x8001
#define DOIP_DIAGNOSTIC_MESSAGE_ACK                0x8002
#define DOIP_DIAGNOSTIC_MESSAGE_NACK               0x8003


/* Header */
#define DOIP_VERSION_OFFSET                        0
#define DOIP_VERSION_LEN                           1
#define DOIP_INV_VERSION_OFFSET                    (DOIP_VERSION_OFFSET + DOIP_VERSION_LEN)
#define DOIP_INV_VERSION_LEN                       1
#define DOIP_TYPE_OFFSET                           (DOIP_INV_VERSION_OFFSET + DOIP_INV_VERSION_LEN)
#define DOIP_TYPE_LEN                              2
#define DOIP_LENGTH_OFFSET                         (DOIP_TYPE_OFFSET + DOIP_TYPE_LEN)
#define DOIP_LENGTH_LEN                            4
#define DOIP_HEADER_LEN                            (DOIP_LENGTH_OFFSET + DOIP_LENGTH_LEN)

#define RESERVED_VER                               0x00
#define ISO13400_2010                              0x01
#define ISO13400_2012                              0x02
#define ISO13400_2019                              0x03
#define ISO13400_2019_AMD1                         0x04
#define DEFAULT_VALUE                              0xFF


/* Generic NACK */
#define DOIP_GENERIC_NACK_OFFSET                   DOIP_HEADER_LEN
#define DOIP_GENERIC_NACK_LEN                      1


/* Common */
#define DOIP_COMMON_VIN_LEN                        17
#define DOIP_COMMON_EID_LEN                        6


/*  Vehicle identifcation request */
#define DOIP_VEHICLE_IDENTIFICATION_EID_OFFSET     DOIP_HEADER_LEN
#define DOIP_VEHICLE_IDENTIFICATION_VIN_OFFSET     DOIP_HEADER_LEN


/* Routing activation request */
#define DOIP_ROUTING_ACTIVATION_REQ_SRC_OFFSET     DOIP_HEADER_LEN
#define DOIP_ROUTING_ACTIVATION_REQ_SRC_LEN        2
#define DOIP_ROUTING_ACTIVATION_REQ_TYPE_OFFSET    (DOIP_ROUTING_ACTIVATION_REQ_SRC_OFFSET + DOIP_ROUTING_ACTIVATION_REQ_SRC_LEN)
#define DOIP_ROUTING_ACTIVATION_REQ_TYPE_LEN_V1    2
#define DOIP_ROUTING_ACTIVATION_REQ_TYPE_LEN_V2    1
#define DOIP_ROUTING_ACTIVATION_REQ_ISO_OFFSET_V1  (DOIP_ROUTING_ACTIVATION_REQ_TYPE_OFFSET + DOIP_ROUTING_ACTIVATION_REQ_TYPE_LEN_V1)
#define DOIP_ROUTING_ACTIVATION_REQ_ISO_OFFSET_V2  (DOIP_ROUTING_ACTIVATION_REQ_TYPE_OFFSET + DOIP_ROUTING_ACTIVATION_REQ_TYPE_LEN_V2)
#define DOIP_ROUTING_ACTIVATION_REQ_ISO_LEN        4
#define DOIP_ROUTING_ACTIVATION_REQ_OEM_OFFSET_V1  (DOIP_ROUTING_ACTIVATION_REQ_ISO_OFFSET_V1 + DOIP_ROUTING_ACTIVATION_REQ_ISO_LEN)
#define DOIP_ROUTING_ACTIVATION_REQ_OEM_OFFSET_V2  (DOIP_ROUTING_ACTIVATION_REQ_ISO_OFFSET_V2 + DOIP_ROUTING_ACTIVATION_REQ_ISO_LEN)
#define DOIP_ROUTING_ACTIVATION_REQ_OEM_LEN        4


/* Routing activation response */
#define DOIP_ROUTING_ACTIVATION_RES_TESTER_OFFSET  DOIP_HEADER_LEN
#define DOIP_ROUTING_ACTIVATION_RES_TESTER_LEN     2
#define DOIP_ROUTING_ACTIVATION_RES_ENTITY_OFFSET  (DOIP_ROUTING_ACTIVATION_RES_TESTER_OFFSET + DOIP_ROUTING_ACTIVATION_RES_TESTER_LEN)
#define DOIP_ROUTING_ACTIVATION_RES_ENTITY_LEN     2
#define DOIP_ROUTING_ACTIVATION_RES_CODE_OFFSET    (DOIP_ROUTING_ACTIVATION_RES_ENTITY_OFFSET + DOIP_ROUTING_ACTIVATION_RES_ENTITY_LEN)
#define DOIP_ROUTING_ACTIVATION_RES_CODE_LEN       1
#define DOIP_ROUTING_ACTIVATION_RES_ISO_OFFSET     (DOIP_ROUTING_ACTIVATION_RES_CODE_OFFSET + DOIP_ROUTING_ACTIVATION_RES_CODE_LEN)
#define DOIP_ROUTING_ACTIVATION_RES_ISO_LEN        4
#define DOIP_ROUTING_ACTIVATION_RES_OEM_OFFSET     (DOIP_ROUTING_ACTIVATION_RES_ISO_OFFSET + DOIP_ROUTING_ACTIVATION_RES_ISO_LEN)
#define DOIP_ROUTING_ACTIVATION_RES_OEM_LEN        4


/* Vehicle announcement message */
#define DOIP_VEHICLE_ANNOUNCEMENT_VIN_OFFSET       DOIP_HEADER_LEN
#define DOIP_VEHICLE_ANNOUNCEMENT_ADDRESS_OFFSET   (DOIP_VEHICLE_ANNOUNCEMENT_VIN_OFFSET + DOIP_COMMON_VIN_LEN)
#define DOIP_VEHICLE_ANNOUNCEMENT_ADDRESS_LEN      2
#define DOIP_VEHICLE_ANNOUNCEMENT_EID_OFFSET       (DOIP_VEHICLE_ANNOUNCEMENT_ADDRESS_OFFSET + DOIP_VEHICLE_ANNOUNCEMENT_ADDRESS_LEN)
#define DOIP_VEHICLE_ANNOUNCEMENT_GID_OFFSET       (DOIP_VEHICLE_ANNOUNCEMENT_EID_OFFSET + DOIP_COMMON_EID_LEN)
#define DOIP_VEHICLE_ANNOUNCEMENT_GID_LEN          6
#define DOIP_VEHICLE_ANNOUNCEMENT_ACTION_OFFSET    (DOIP_VEHICLE_ANNOUNCEMENT_GID_OFFSET + DOIP_VEHICLE_ANNOUNCEMENT_GID_LEN)
#define DOIP_VEHICLE_ANNOUNCEMENT_ACTION_LEN       1
#define DOIP_VEHICLE_ANNOUNCEMENT_SYNC_OFFSET      (DOIP_VEHICLE_ANNOUNCEMENT_ACTION_OFFSET + DOIP_VEHICLE_ANNOUNCEMENT_ACTION_LEN)
#define DOIP_VEHICLE_ANNOUNCEMENT_SYNC_LEN         1


/* Alive check response */
#define DOIP_ALIVE_CHECK_RESPONSE_SOURCE_OFFSET    DOIP_HEADER_LEN
#define DOIP_ALIVE_CHECK_RESPONSE_SOURCE_LEN       2


/* Entity status response */
#define DOIP_ENTITY_STATUS_RESPONSE_NODE_OFFSET    DOIP_HEADER_LEN
#define DOIP_ENTITY_STATUS_RESPONSE_NODE_LEN       1
#define DOIP_ENTITY_STATUS_RESPONSE_MCTS_OFFSET    (DOIP_ENTITY_STATUS_RESPONSE_NODE_OFFSET + DOIP_ENTITY_STATUS_RESPONSE_NODE_LEN)
#define DOIP_ENTITY_STATUS_RESPONSE_MCTS_LEN       1
#define DOIP_ENTITY_STATUS_RESPONSE_NCTS_OFFSET    (DOIP_ENTITY_STATUS_RESPONSE_MCTS_OFFSET + DOIP_ENTITY_STATUS_RESPONSE_MCTS_LEN)
#define DOIP_ENTITY_STATUS_RESPONSE_NCTS_LEN       1
#define DOIP_ENTITY_STATUS_RESPONSE_MDS_OFFSET     (DOIP_ENTITY_STATUS_RESPONSE_NCTS_OFFSET + DOIP_ENTITY_STATUS_RESPONSE_NCTS_LEN)
#define DOIP_ENTITY_STATUS_RESPONSE_MDS_LEN        4


/* Diagnostic power mode information response */
#define DOIP_POWER_MODE_OFFSET                     DOIP_HEADER_LEN
#define DOIP_POWER_MODE_LEN                        1


/* Common */
#define DOIP_DIAG_COMMON_SOURCE_OFFSET             DOIP_HEADER_LEN
#define DOIP_DIAG_COMMON_SOURCE_LEN                2
#define DOIP_DIAG_COMMON_TARGET_OFFSET             (DOIP_DIAG_COMMON_SOURCE_OFFSET + DOIP_DIAG_COMMON_SOURCE_LEN)
#define DOIP_DIAG_COMMON_TARGET_LEN                2


/* Diagnostic message */
#define DOIP_DIAG_MESSAGE_DATA_OFFSET              (DOIP_DIAG_COMMON_TARGET_OFFSET + DOIP_DIAG_COMMON_TARGET_LEN)


/* Diagnostic message ACK */
#define DOIP_DIAG_MESSAGE_ACK_CODE_OFFSET          (DOIP_DIAG_COMMON_TARGET_OFFSET + DOIP_DIAG_COMMON_TARGET_LEN)
#define DOIP_DIAG_MESSAGE_ACK_CODE_LEN             1
#define DOIP_DIAG_MESSAGE_ACK_PREVIOUS_OFFSET      (DOIP_DIAG_MESSAGE_ACK_CODE_OFFSET + DOIP_DIAG_MESSAGE_ACK_CODE_LEN)


/* Diagnostic message NACK */
#define DOIP_DIAG_MESSAGE_NACK_CODE_OFFSET         (DOIP_DIAG_COMMON_TARGET_OFFSET + DOIP_DIAG_COMMON_TARGET_LEN)
#define DOIP_DIAG_MESSAGE_NACK_CODE_LEN            1
#define DOIP_DIAG_MESSAGE_NACK_PREVIOUS_OFFSET     (DOIP_DIAG_MESSAGE_NACK_CODE_OFFSET + DOIP_DIAG_MESSAGE_NACK_CODE_LEN)



/*
 * Enums
 */

/* Header */
/* Protocol version */
static const value_string doip_versions[] = {
    { RESERVED_VER,         "Reserved" },
    { ISO13400_2010,        "DoIP ISO/DIS 13400-2:2010" },
    { ISO13400_2012,        "DoIP ISO 13400-2:2012" },
    { ISO13400_2019,        "DoIP ISO 13400-2:2019" },
    { ISO13400_2019_AMD1,   "DoIP ISO 13400-2:2019 Amd1 (experimental)" },
    { DEFAULT_VALUE,        "Default value for vehicle identification request messages" },
    { 0, 0 }
};

const char *dm_desc_doip_versions(INT32U value)
{
    int i = 0;

    for (i = 0; doip_versions[i].strptr; i++) {
        if (doip_versions[i].value == value) {
            return doip_versions[i].strptr;
        }
    }

    return "Unknown";
}

/* Payload type */
static const value_string doip_payloads[] = {
    { DOIP_GENERIC_NACK,                    "Generic DoIP header NACK" },
    { DOIP_VEHICLE_IDENTIFICATION_REQ,      "Vehicle identification request" },
    { DOIP_VEHICLE_IDENTIFICATION_REQ_EID,  "Vehicle identification request with EID" },
    { DOIP_VEHICLE_IDENTIFICATION_REQ_VIN,  "Vehicle identification request with VIN" },
    { DOIP_VEHICLE_ANNOUNCEMENT_MESSAGE,     "Vehicle announcement message/vehicle identification response message" },
    { DOIP_ROUTING_ACTIVATION_REQUEST, "Routing activation request" },
    { DOIP_ROUTING_ACTIVATION_RESPONSE, "Routing activation response" },
    { DOIP_ALIVE_CHECK_REQUEST, "Alive check request" },
    { DOIP_ALIVE_CHECK_RESPONSE, "Alive check response" },
    { DOIP_ENTITY_STATUS_REQUEST, "DoIP entity status request" },
    { DOIP_ENTITY_STATUS_RESPONSE, "DoIP entity status response" },
    { DOIP_POWER_INFORMATION_REQUEST, "Diagnostic power mode information request" },
    { DOIP_POWER_INFORMATION_RESPONSE, "Diagnostic power mode information response" },
    { DOIP_DIAGNOSTIC_MESSAGE, "Diagnostic message" },
    { DOIP_DIAGNOSTIC_MESSAGE_ACK, "Diagnostic message ACK" },
    { DOIP_DIAGNOSTIC_MESSAGE_NACK, "Diagnostic message NACK" },
    { 0, 0 }
};
    
const char *dm_desc_doip_payloads(INT32U value)
{
    int i = 0;

    for (i = 0; doip_payloads[i].strptr; i++) {
        if (doip_payloads[i].value == value) {
            return doip_payloads[i].strptr;
        }
    }

    return "Unknown";
}

/* Generic NACK */
static const value_string nack_codes[] = {
    { 0x00, "Incorrect pattern format" },
    { 0x01, "Unknown payload type" },
    { 0x02, "Message too large" },
    { 0x03, "Out of memory" },
    { 0x04, "Invalid payload length" },
    { 0, 0 }
};

const char *dm_desc_nack_codes(INT32U value)
{
    int i = 0;

    for (i = 0; nack_codes[i].strptr; i++) {
        if (nack_codes[i].value == value) {
            return nack_codes[i].strptr;
        }
    }

    return "Unknown";
}

/* Routing activation request */
static const value_string activation_types[] = {
    { 0x00, "Default" },
    { 0x01, "WWH-OBD" },
    { 0xE0, "Central security" },
    { 0, 0 }
};
    
const char *dm_desc_activation_types(INT32U value)
{
    int i = 0;

    for (i = 0; activation_types[i].strptr; i++) {
        if (activation_types[i].value == value) {
            return activation_types[i].strptr;
        }
    }

    return "Unknown";
}

/* Routing activation response */
static const value_string activation_codes[] = {
    { 0x00, "Routing activation denied due to unknown source address." },
    { 0x01, "Routing activation denied because all concurrently supported TCP_DATA sockets are registered and active." },
    { 0x02, "Routing activation denied because an SA different from the table connection entry was received on the already activated TCP_DATA socket." },
    { 0x03, "Routing activation denied because the SA is already registered and active on a different TCP_DATA socket." },
    { 0x04, "Routing activation denied due to missing authentication." },
    { 0x05, "Routing activation denied due to rejected confirmation." },
    { 0x06, "Routing activation denied due to unsupported routing activation type." },
    { 0x07, "Routing activation denied due to request for encrypted connection via TLS." },
    { 0x08, "Reserved by ISO 13400." },
    { 0x09, "Reserved by ISO 13400." },
    { 0x0A, "Reserved by ISO 13400." },
    { 0x0B, "Reserved by ISO 13400." },
    { 0x0C, "Reserved by ISO 13400." },
    { 0x0D, "Reserved by ISO 13400." },
    { 0x0E, "Reserved by ISO 13400." },
    { 0x0F, "Reserved by ISO 13400." },
    { 0x10, "Routing successfully activated." },
    { 0x11, "Routing will be activated; confirmation required." },
    { 0, 0 }
};

const char *dm_desc_activation_codes(INT32U value)
{
    int i = 0;

    for (i = 0; activation_codes[i].strptr; i++) {
        if (activation_codes[i].value == value) {
            return activation_codes[i].strptr;
        }
    }

    return "Unknown";
}


/* Vehicle announcement message */
/* Action code */
static const value_string action_codes[] = {
    { 0x00, "No further action required" },
    { 0x01, "Reserved by ISO 13400" },
    { 0x02, "Reserved by ISO 13400" },
    { 0x03, "Reserved by ISO 13400" },
    { 0x04, "Reserved by ISO 13400" },
    { 0x05, "Reserved by ISO 13400" },
    { 0x06, "Reserved by ISO 13400" },
    { 0x07, "Reserved by ISO 13400" },
    { 0x08, "Reserved by ISO 13400" },
    { 0x09, "Reserved by ISO 13400" },
    { 0x0A, "Reserved by ISO 13400" },
    { 0x0B, "Reserved by ISO 13400" },
    { 0x0C, "Reserved by ISO 13400" },
    { 0x0D, "Reserved by ISO 13400" },
    { 0x0E, "Reserved by ISO 13400" },
    { 0x0F, "Reserved by ISO 13400" },
    { 0x10, "Routing activation required to initiate central security" },
    { 0, 0 }
};

const char *dm_desc_action_codes(INT32U value)
{
    int i = 0;

    for (i = 0; action_codes[i].strptr; i++) {
        if (action_codes[i].value == value) {
            return action_codes[i].strptr;
        }
    }

    return "Unknown";
}

/* Sync status */
static const value_string sync_status[] = {
    { 0x00, "VIN and/or GID are synchronized" },
    { 0x01, "Reserved by ISO 13400" },
    { 0x02, "Reserved by ISO 13400" },
    { 0x03, "Reserved by ISO 13400" },
    { 0x04, "Reserved by ISO 13400" },
    { 0x05, "Reserved by ISO 13400" },
    { 0x06, "Reserved by ISO 13400" },
    { 0x07, "Reserved by ISO 13400" },
    { 0x08, "Reserved by ISO 13400" },
    { 0x09, "Reserved by ISO 13400" },
    { 0x0A, "Reserved by ISO 13400" },
    { 0x0B, "Reserved by ISO 13400" },
    { 0x0C, "Reserved by ISO 13400" },
    { 0x0D, "Reserved by ISO 13400" },
    { 0x0E, "Reserved by ISO 13400" },
    { 0x0F, "Reserved by ISO 13400" },
    { 0x10, "Incomplete: VIN and GID are NOT synchronized" },
    { 0, 0 }
};

const char *dm_desc_sync_status(INT32U value)
{
    int i = 0;

    for (i = 0; action_codes[i].strptr; i++) {
        if (action_codes[i].value == value) {
            return action_codes[i].strptr;
        }
    }

    return "Unknown";
}

/* Entity status response */
/* Node type */
static const value_string node_types[] = {
    { 0x00, "DoIP gateway" },
    { 0x01, "DoIp node" },
    { 0, 0 }
};

const char *dm_desc_node_types(INT32U value)
{
    int i = 0;

    for (i = 0; node_types[i].strptr; i++) {
        if (node_types[i].value == value) {
            return node_types[i].strptr;
        }
    }

    return "Unknown";
}

/* Diagnostic power mode information response */
/* Power mode */
static const value_string power_modes[] = {
    { 0x00, "not ready" },
    { 0x01, "ready" },
    { 0x02, "not supported" },
    { 0, 0 }
};

const char *dm_desc_power_modes(INT32U value)
{
    int i = 0;

    for (i = 0; power_modes[i].strptr; i++) {
        if (power_modes[i].value == value) {
            return power_modes[i].strptr;
        }
    }

    return "Unknown";
}

/* Diagnostic message ACK */
static const value_string diag_ack_codes[] = {
    { 0x00, "ACK" },
    { 0, 0 }
};

const char *dm_desc_diag_ack_codes(INT32U value)
{
    int i = 0;

    for (i = 0; diag_ack_codes[i].strptr; i++) {
        if (diag_ack_codes[i].value == value) {
            return diag_ack_codes[i].strptr;
        }
    }

    return "Unknown";
}

/* Diagnostic message NACK */
static const value_string diag_nack_codes[] = {\
    { 0x00, "Reserved by ISO 13400" },
    { 0x01, "Reserved by ISO 13400" },
    { 0x02, "Invalid source address" },
    { 0x03, "Unknown target address" },
    { 0x04, "Diagnostic message too large" },
    { 0x05, "Out of memory" },
    { 0x06, "Target unreachable" },
    { 0x07, "Unknown network" },
    { 0x08, "Transport protocol error" },
    { 0, 0 }
};

const char *dm_desc_diag_nack_codes(INT32U value)
{
    int i = 0;

    for (i = 0; diag_nack_codes[i].strptr; i++) {
        if (diag_nack_codes[i].value == value) {
            return diag_nack_codes[i].strptr;
        }
    }

    return "Unknown";
}

static int doipc_udp_create(unsigned int ip, short port)
{
    int sockfd = -1;
    int flag = 0;
    
#ifdef __linux__    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
#endif /* __linux__ */
    if (!(sockfd > 0)) {
        return sockfd;
    }
    flag = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flag | O_NONBLOCK);
       
    return sockfd;
}

static int doipc_tcp_create(unsigned int ip, short port)
{
    int sockfd = -1;
    int flag = 0;

#ifdef __linux__    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
#endif /* __linux__ */
    if (!(sockfd > 0)) {
        return sockfd;
    }
    flag = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flag | O_NONBLOCK);
       
    return sockfd;
}

static int doipc_is_active(doip_client_t *doipc)
{
    return doipc->isactive;
}

int doipc_tcp_send(doip_client_t *doipc, INT8U *msg, INT32U size)
{
    int bytesSent = -1;
    fd_set writefds;
    struct timeval timeout;
    int sret = 0;

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    FD_ZERO(&writefds);
    FD_SET(doipc->tcp_sfd, &writefds);
    if ((sret = select(doipc->tcp_sfd + 1, NULL, &writefds, NULL, &timeout)) > 0 &&\
        FD_ISSET(doipc->tcp_sfd, &writefds)) {
        bytesSent = send(doipc->tcp_sfd, msg, size, 0);
        if (bytesSent < 0) {
            log_e("(%d)Send error: %s(errno: %d) \n", doipc->tcp_sfd, strerror(errno), errno);
        }
        else {
            return bytesSent;
        }
    }
    else {
        log_e("(%d)Select return %d: %s(errno: %d) \n", doipc->tcp_sfd, sret, strerror(errno), errno);
    }
    
    return bytesSent;
}

int doipc_header_encode(INT8U *buf, INT32U buf_size, INT8U ver, INT16U type, INT32U len)
{
    if (buf_size < DOIP_HEADER_LEN) {
        return -1;
    }
    buf[DOIP_VERSION_OFFSET] = ver;
    buf[DOIP_INV_VERSION_OFFSET] = ~ver;
    buf[DOIP_TYPE_OFFSET] = ((type >> 8) & 0xff);
    buf[DOIP_TYPE_OFFSET + 1] = ((type >> 0) & 0xff);
    buf[DOIP_LENGTH_OFFSET] = ((len >> 24) & 0xff);
    buf[DOIP_LENGTH_OFFSET + 1] = ((len >> 16) & 0xff);
    buf[DOIP_LENGTH_OFFSET + 2] = ((len >> 8) & 0xff);
    buf[DOIP_LENGTH_OFFSET + 3] = ((len >> 0) & 0xff);

    return 0;
}

int doipc_header_decode(INT8U *buf, INT32U buf_size, INT8U *ver, INT16U *type, INT32U *len)
{
    if (buf_size < DOIP_HEADER_LEN) {
        return -1;
    }
    *ver = buf[DOIP_VERSION_OFFSET];
    *type = (*type & ~(0xff << 8)) | ((buf[DOIP_TYPE_OFFSET] & 0xff) << 8);
    *type = (*type & ~(0xff << 0)) | ((buf[DOIP_TYPE_OFFSET + 1] & 0xff) << 0);
    *len = (*len & ~(0xff << 24)) | ((buf[DOIP_LENGTH_OFFSET] & 0xff) << 24);
    *len = (*len & ~(0xff << 16)) | ((buf[DOIP_LENGTH_OFFSET + 1] & 0xff) << 16);
    *len = (*len & ~(0xff << 8)) | ((buf[DOIP_LENGTH_OFFSET + 2] & 0xff) << 8);
    *len = (*len & ~(0xff << 0)) | ((buf[DOIP_LENGTH_OFFSET + 3] & 0xff) << 0);

    return 0;
}

static int doipc_tcp_connect_check(doip_client_t *doipc)
{
    fd_set writefds;
    fd_set readfds;
    struct timeval timeout;
    int sret = 0;

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    FD_ZERO(&writefds);
    FD_SET(doipc->tcp_sfd, &writefds);
    FD_ZERO(&readfds);
    FD_SET(doipc->tcp_sfd, &readfds);
    if ((sret = select(doipc->tcp_sfd + 1, &readfds, &writefds, NULL, &timeout)) > 0) {
        int error = 0;
        socklen_t len = sizeof(error);
    
        if (FD_ISSET(doipc->tcp_sfd, &writefds) &&\
            FD_ISSET(doipc->tcp_sfd, &readfds)) {
            if (getsockopt(doipc->tcp_sfd, SOL_SOCKET, SO_ERROR, (void *)&error, &len) < 0 || \
                error != 0) {
                return 0;
            }
            else {                
                return 1;
            }
        }
        else if (FD_ISSET(doipc->tcp_sfd, &writefds)) {            
            return 1;
        }
        else if (FD_ISSET(doipc->tcp_sfd, &readfds)) {        
            return 0;
        }
    }

    return 0;
}

static void doipc_tcp_connect_process_handler(doip_client_t *doipc)
{
    if (doipc_tcp_connect_check(doipc)) {
        log_d("doip tcp connect success \n");
        doipc->con_stat = DOIPC_TCP_CONNECT_SUCCESS;
        doipc_routing_activation_request(doipc);   
    }
    else {        
        log_d("doip tcp connect failed \n");
        dm_doipc_disconnect_server(doipc); 
    }
    ev_io_stop(doipc->loop, &doipc->tcpsend_watcher);

    return ;
}

static void doipc_ev_io_tcp_write_handler(struct ev_loop* loop, ev_io* w, int e)
{
    doip_client_t* doipc = container_of(w, doip_client_t, tcpsend_watcher);

    /* 检查是否连接上 */
    if (doipc->con_stat == DOIPC_TCP_CONNECT_PROCESS) {
        doipc_tcp_connect_process_handler(doipc);
        return ;
    }

    return ;
}

static void doipc_routing_activation_response_handler(doip_client_t *doipc)
{
    unsigned short tester = 0, entity = 0;
    unsigned char rcode = 0;

    tester |= (doipc->rxbuf[DOIP_ROUTING_ACTIVATION_RES_TESTER_OFFSET] & 0xff)<< 8;
    tester |= (doipc->rxbuf[DOIP_ROUTING_ACTIVATION_RES_TESTER_OFFSET + 1] & 0xff) << 0;
    entity |= (doipc->rxbuf[DOIP_ROUTING_ACTIVATION_RES_ENTITY_OFFSET] & 0xff) << 8;
    entity |= (doipc->rxbuf[DOIP_ROUTING_ACTIVATION_RES_ENTITY_OFFSET + 1] & 0xff) << 0;
    rcode = doipc->rxbuf[DOIP_ROUTING_ACTIVATION_RES_CODE_OFFSET];
    log_d("tester => %04x \n", tester);
    log_d("entity => %04x \n", entity);
    log_d("rcode  => %02x \n", rcode);
    log_d("%s \n", dm_desc_activation_codes(rcode));    
    if (rcode == 0x10) {
        doipc->isactive = 1;
    }
    else {
        doipc->isactive = 0;
    }

    return ;
}

static void doipc_alive_check_request_handler(doip_client_t *doipc)
{
    if (!doipc_is_active(doipc)) {
        return ;
    }

    /* encode 8byte fix header */
    doipc_header_encode(doipc->txbuf, doipc->txlen, doipc->config.ver, DOIP_ALIVE_CHECK_RESPONSE, \
                                DOIP_ALIVE_CHECK_RESPONSE_SOURCE_LEN);
    /* encode 2byte source addr */
    doipc->txbuf[DOIP_DIAG_COMMON_SOURCE_OFFSET] = ((doipc->config.sa_addr >> 8) & 0xff);
    doipc->txbuf[DOIP_DIAG_COMMON_SOURCE_OFFSET + 1] = ((doipc->config.sa_addr >> 0) & 0xff);
    doipc_tcp_send(doipc, doipc->txbuf, DOIP_HEADER_LEN + DOIP_ALIVE_CHECK_RESPONSE_SOURCE_LEN);

    return;
}

static void doipc_ev_io_tcp_read_handler(struct ev_loop* loop, ev_io* w, int e)
{
    int bytesRecv = -1;
    doip_client_t* doipc = container_of(w, doip_client_t, tcprecv_watcher);
    INT8U ver = 0; INT16U type = 0; INT32U len = 0;

    /* 检查是否连接上 */
    if (doipc->con_stat == DOIPC_TCP_CONNECT_PROCESS) {
        doipc_tcp_connect_process_handler(doipc);
        return ;
    }

    bytesRecv = recv(doipc->tcp_sfd, doipc->rxbuf, doipc->rxlen, 0);
    if (!(bytesRecv > 0)) {
        dm_doipc_disconnect_server(doipc);
        return ;
    }
    log_hex_d("DOIP Response", doipc->rxbuf, bytesRecv);
    if (doipc_header_decode(doipc->rxbuf, bytesRecv, &ver, &type, &len) < 0) {
        log_e("DOIP procotol format error \n");
        return ;
    }
    switch (type) {
        case DOIP_ROUTING_ACTIVATION_RESPONSE:
            doipc_routing_activation_response_handler(doipc);
            break;
        case DOIP_ALIVE_CHECK_REQUEST:
            doipc_alive_check_request_handler(doipc);
            break;
        default:

            break;
    }

    return ;
}

static int doipc_event_loop_set(doip_client_t *doipc, struct ev_loop *loop)
{
    if (doipc == 0 || loop == 0) {
        return -1;
    }
    
    doipc->loop = loop;
    
    return 0;
}

int doipc_routing_activation_request(doip_client_t *doipc)
{
    if (doipc_is_active(doipc)) {
        return -1;
    }

    if (doipc->txlen < DOIP_ROUTING_ACTIVATION_REQ_OEM_OFFSET_V2) {
        return -2;
    }

    /* encode 8byte fix header */
    doipc_header_encode(doipc->txbuf, doipc->txlen, doipc->config.ver, DOIP_ROUTING_ACTIVATION_REQUEST, \
                                DOIP_ROUTING_ACTIVATION_REQ_OEM_OFFSET_V2 - DOIP_HEADER_LEN);
    /* encode 2byte source addr */
    doipc->txbuf[DOIP_ROUTING_ACTIVATION_REQ_SRC_OFFSET] = ((doipc->config.sa_addr >> 8) & 0xff);
    doipc->txbuf[DOIP_ROUTING_ACTIVATION_REQ_SRC_OFFSET + 1] = ((doipc->config.sa_addr >> 0) & 0xff);
    /* encode 1byte req type */
    doipc->txbuf[DOIP_ROUTING_ACTIVATION_REQ_TYPE_OFFSET] = 0;
    /* encode 4byte iso */
    doipc->txbuf[DOIP_ROUTING_ACTIVATION_REQ_ISO_OFFSET_V2] = ((0 >> 24) & 0xff);
    doipc->txbuf[DOIP_ROUTING_ACTIVATION_REQ_ISO_OFFSET_V2 + 1] = ((0 >> 16) & 0xff);
    doipc->txbuf[DOIP_ROUTING_ACTIVATION_REQ_ISO_OFFSET_V2 + 2] = ((0 >> 8) & 0xff);
    doipc->txbuf[DOIP_ROUTING_ACTIVATION_REQ_ISO_OFFSET_V2 + 3] = ((0 >> 0) & 0xff);
    log_hex_d("DOIP rout active request:", doipc->txbuf, DOIP_ROUTING_ACTIVATION_REQ_OEM_OFFSET_V2);
    
    return doipc_tcp_send(doipc, doipc->txbuf, DOIP_ROUTING_ACTIVATION_REQ_OEM_OFFSET_V2);

}

int dm_doipc_diagnostic_request(doip_client_t *doipc, INT16U sa, INT16U ta, const INT8U *msg, INT32U len, INT32U timeout)
{
    if (!doipc_is_active(doipc)) {
        return -1;
    }

    if (len > (doipc->txlen - DOIP_DIAG_COMMON_TARGET_OFFSET)) {
        return -2;
    }

    /* encode 8byte fix header */
    doipc_header_encode(doipc->txbuf, doipc->txlen, doipc->config.ver, DOIP_DIAGNOSTIC_MESSAGE, \
                                len + DOIP_DIAG_COMMON_TARGET_OFFSET - DOIP_HEADER_LEN);
    /* encode 2byte source addr */
    doipc->txbuf[DOIP_DIAG_COMMON_SOURCE_OFFSET] = ((doipc->config.sa_addr >> 8) & 0xff);
    doipc->txbuf[DOIP_DIAG_COMMON_SOURCE_OFFSET + 1] = ((doipc->config.sa_addr >> 0) & 0xff);
    /* encode 2byte target addr */
    doipc->txbuf[DOIP_DIAG_COMMON_TARGET_OFFSET] = ((ta >> 8) & 0xff);
    doipc->txbuf[DOIP_DIAG_COMMON_TARGET_OFFSET + 1] = ((ta >> 0) & 0xff);
    /* encode nbyte uds diag msg */
    memcpy(doipc->txbuf + DOIP_DIAG_COMMON_TARGET_OFFSET, msg, len);
    log_hex_d("DOIP Diag request:", doipc->txbuf, len + DOIP_DIAG_COMMON_TARGET_OFFSET);
    
    return doipc_tcp_send(doipc, doipc->txbuf, len + DOIP_DIAG_COMMON_TARGET_OFFSET);
}

int dm_doipc_disconnect_server(doip_client_t *doipc)
{
    log_d("doip client disconnect \n");
    doipc->isactive = 0;
    doipc->con_stat = DOIPC_TCP_DISCONNECT;
    ev_io_stop(doipc->loop, &doipc->tcprecv_watcher);
    ev_io_stop(doipc->loop, &doipc->tcpsend_watcher);
    if (doipc->tcp_sfd > 0) {
        close(doipc->tcp_sfd);
        doipc->tcp_sfd = -1;
    }

    if (doipc->udp_sfd > 0) {
        close(doipc->udp_sfd);
        doipc->udp_sfd = -1;
    }

    return 0;
}

int dm_doipc_connect_active_server(doip_client_t *doipc, doipc_config_t *config)
{
    int ret = 0;
    struct sockaddr_in toAddr;

    memcpy(&doipc->config, config, sizeof(*config));
    doipc->config.ver = ISO13400_2012;

    /* 断开后重新连接激活 */
    dm_doipc_disconnect_server(doipc);

    /* 创建TCP */
    doipc->tcp_sfd = doipc_tcp_create(doipc->config.src_ip, doipc->config.src_port);
    if (doipc->tcp_sfd < 0) {
        log_e("TCP Socket create error \n");
        return -1;
    }
    /* 创建一个UDP的socket */
    doipc->udp_sfd = doipc_udp_create(doipc->config.src_ip, doipc->config.src_port);
    if (doipc->udp_sfd < 0) {
        log_e("UDP Socket create error \n");
        return -2;
    }

    /* 开始连接server */
    toAddr.sin_family = AF_INET;
    toAddr.sin_addr.s_addr = htonl(doipc->config.dst_ip);
    toAddr.sin_port = htons(doipc->config.dst_port);
    ret = connect(doipc->tcp_sfd, (struct sockaddr *)&toAddr, sizeof(struct sockaddr));
    if (ret == -1 && errno == EINPROGRESS) {
        /* 标记一下正在连接中 */
        doipc->con_stat = DOIPC_TCP_CONNECT_PROCESS; 
        /* 非阻塞的SOCKET需要另行判断 */
        ev_io_init(&doipc->tcprecv_watcher, doipc_ev_io_tcp_read_handler, doipc->tcp_sfd, EV_READ);
        ev_io_start(doipc->loop, &doipc->tcprecv_watcher);
        ev_io_init(&doipc->tcpsend_watcher, doipc_ev_io_tcp_write_handler, doipc->tcp_sfd, EV_WRITE);
        ev_io_start(doipc->loop, &doipc->tcpsend_watcher);
    }
    else if (ret == 0) {        
        log_w("TCP connect success \n");
        doipc->con_stat = DOIPC_TCP_CONNECT_SUCCESS; 
        ev_io_init(&doipc->tcprecv_watcher, doipc_ev_io_tcp_read_handler, doipc->tcp_sfd, EV_READ);
        ev_io_start(doipc->loop, &doipc->tcprecv_watcher);
    }
    else {
        /* 连接失败 */
        log_w("TCP connect failed \n");
        doipc->con_stat = DOIPC_TCP_DISCONNECT;
        dm_doipc_disconnect_server(doipc);
    }

    return 0;
}

doip_client_t *dm_doipc_create(struct ev_loop *loop)
{
    doip_client_t *doipc = malloc(sizeof(*doipc));
    if (doipc == 0) {
        return 0;
    }
    memset(doipc, 0, sizeof(*doipc));
    /* 初始化变量 */
    doipc->tcp_sfd = -1;
    doipc->udp_sfd = -1;
    doipc->con_stat = DOIPC_TCP_DISCONNECT;
    doipc->isactive = 0;
    
    doipc->rxlen = RXTX_BUFF_LEN_DEF;
    doipc->rxbuf = malloc(doipc->rxlen);
    if (doipc->rxbuf == 0) {
        log_e("malloc rxbuff error \n");
        goto CREATE_FAILED;
    }
    memset(doipc->rxbuf, 0, doipc->rxlen);

    doipc->txlen = RXTX_BUFF_LEN_DEF;
    doipc->txbuf = malloc(doipc->txlen);
    if (doipc->txbuf == 0) {
        log_e("malloc txbuff error \n");
        goto CREATE_FAILED;
    }
    memset(doipc->txbuf, 0, doipc->txlen);
    
    doipc_event_loop_set(doipc, loop);

    return doipc;
CREATE_FAILED:
    dm_doipc_destory(doipc);

    return 0;
}

int dm_doipc_destory(doip_client_t *doipc)
{
    if (doipc == 0) {
        return -1;
    }
    dm_doipc_disconnect_server(doipc);
    if (doipc->rxbuf) {
        free(doipc->rxbuf);
        doipc->rxbuf = 0;
        doipc->rxlen = 0;
    }
    if (doipc->txbuf) {
        free(doipc->txbuf);
        doipc->txbuf = 0;
        doipc->txlen = 0;
    }

    free(doipc);

    return 0;
}

