#ifndef __DM_UDSC_TYPES_H__
#define __DM_UDSC_TYPES_H__


#define __HAVE_UDS_DESC_INFO__ /* ÃèÊöÐÅÏ¢Êä³ö */

typedef unsigned int INT32U;
typedef unsigned short INT16U;
typedef unsigned char INT8U;

typedef signed int INT32;
typedef signed short INT16;
typedef signed char INT8;

typedef unsigned char BOOLEAN;
#ifdef _WIN32
#ifndef F_OK
#define F_OK (0)
#endif /* #ifndef F_OK */
typedef int ssize_t;
#endif /* _WIN32 */
#define true (1)
#define false (!true)

#ifndef NULL
#define NULL ((void *)0)
#endif /* NULL */

#ifndef MIN
#define MIN(x, y) ((x) > (y) ? (y) : (x))
#endif

#ifdef _WIN32

#else /* _WIN32 */
#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t)(&((TYPE *)0)->MEMBER))
#endif
#ifndef container_of
#define container_of(ptr, type, member) ({ \
            const typeof(*ptr) *__ptr = (ptr); \
            (type *) ((char*)__ptr - offsetof(type, member));})
#endif
#endif /* _WIN32 */

#define Y_UNUSED(v) (void)v

#define DM_MALLOC(s) malloc(s)
#define DM_FREE(p) free(p)
#define DM_CALLOC(s, c) calloc(s, c)

#ifdef __HAVE_UDS_DESC_INFO__
typedef struct _value_string {
    INT32U      value;
    const char *strptr;
} value_string;
#endif /* __HAVE_UDS_DESC_INFO__ */

#define UDS_SID_MASK    0xBF
#define UDS_REPLY_MASK  0x40

#define OBD_SERVICES_0x01    0x01
#define OBD_SERVICES_0x02    0x02
#define OBD_SERVICES_0x03    0x03
#define OBD_SERVICES_0x04    0x04
#define OBD_SERVICES_0x05    0x05
#define OBD_SERVICES_0x06    0x06
#define OBD_SERVICES_0x07    0x07
#define OBD_SERVICES_0x08    0x08
#define OBD_SERVICES_0x09    0x09
#define OBD_SERVICES_0x0A    0x0A
#define OBD_SERVICES_0x0B    0x0B
#define OBD_SERVICES_0x0C    0x0C
#define OBD_SERVICES_0x0D    0x0D
#define OBD_SERVICES_0x0E    0x0E
#define OBD_SERVICES_0x0F    0x0F

#define UDS_RESPONSE_CODES_GR       0x10
#define UDS_RESPONSE_CODES_SNS      0x11
#define UDS_RESPONSE_CODES_SFNS     0x12
#define UDS_RESPONSE_CODES_IMLOIF   0x13
#define UDS_RESPONSE_CODES_RTL      0x14
#define UDS_RESPONSE_CODES_BRR      0x21
#define UDS_RESPONSE_CODES_CNC      0x22
#define UDS_RESPONSE_CODES_RSE      0x24
#define UDS_RESPONSE_CODES_NRFSC    0x25
#define UDS_RESPONSE_CODES_FPEORA   0x26
#define UDS_RESPONSE_CODES_ROOR     0x31
#define UDS_RESPONSE_CODES_SAD      0x33
#define UDS_RESPONSE_CODES_AR       0x34
#define UDS_RESPONSE_CODES_IK       0x35
#define UDS_RESPONSE_CODES_ENOA     0x36
#define UDS_RESPONSE_CODES_RTDNE    0x37
#define UDS_RESPONSE_CODES_SDTR     0x38
#define UDS_RESPONSE_CODES_SDTNA    0x39
#define UDS_RESPONSE_CODES_SDTF     0x3A
#define UDS_RESPONSE_CODES_CVFITP   0x50
#define UDS_RESPONSE_CODES_CVFIS    0x51
#define UDS_RESPONSE_CODES_CVFICOT  0x52
#define UDS_RESPONSE_CODES_CVFIT    0x53
#define UDS_RESPONSE_CODES_CVFIF    0x54
#define UDS_RESPONSE_CODES_CVFIC    0x55
#define UDS_RESPONSE_CODES_CVFISD   0x56
#define UDS_RESPONSE_CODES_CVFICR   0x57
#define UDS_RESPONSE_CODES_OVF      0x58
#define UDS_RESPONSE_CODES_CCF      0x59
#define UDS_RESPONSE_CODES_SARF     0x5A
#define UDS_RESPONSE_CODES_SKCDF    0x5B
#define UDS_RESPONSE_CODES_CDUF     0x5C
#define UDS_RESPONSE_CODES_DAF      0x5D
#define UDS_RESPONSE_CODES_UDNA     0x70
#define UDS_RESPONSE_CODES_TDS      0x71
#define UDS_RESPONSE_CODES_GPF      0x72
#define UDS_RESPONSE_CODES_WBSC     0x73
#define UDS_RESPONSE_CODES_RCRRP    0x78
#define UDS_RESPONSE_CODES_SFNSIAS  0x7E
#define UDS_RESPONSE_CODES_SNSIAS   0x7F
#define UDS_RESPONSE_CODES_RPMTH    0x81
#define UDS_RESPONSE_CODES_RPMTL    0x82
#define UDS_RESPONSE_CODES_EIR      0x83
#define UDS_RESPONSE_CODES_EINR     0x84
#define UDS_RESPONSE_CODES_ERTTL    0x85
#define UDS_RESPONSE_CODES_TEMPTH   0x86
#define UDS_RESPONSE_CODES_TEMPTL   0x87
#define UDS_RESPONSE_CODES_VSTH     0x88
#define UDS_RESPONSE_CODES_VSTL     0x89
#define UDS_RESPONSE_CODES_TPTH     0x8a
#define UDS_RESPONSE_CODES_TPTL     0x8b
#define UDS_RESPONSE_CODES_TRNIN    0x8c
#define UDS_RESPONSE_CODES_TRNIG    0x8d
#define UDS_RESPONSE_CODES_BSNC     0x8f
#define UDS_RESPONSE_CODES_SLNIP    0x90
#define UDS_RESPONSE_CODES_TCCL     0x91
#define UDS_RESPONSE_CODES_VTH      0x92
#define UDS_RESPONSE_CODES_VTL      0x93
#define UDS_RESPONSE_CODES_RTNA     0x94

#define UDS_SUBFUNCTION_MASK                    0x7f
#define UDS_SUPPRESS_POS_RSP_MSG_IND_MASK       0x80

#define UDS_DSC_TYPES_DEFAULT_SESSION                   1
#define UDS_DSC_TYPES_PROGRAMMING_SESSION               2
#define UDS_DSC_TYPES_EXTENDED_DIAGNOSTIC_SESSION       3
#define UDS_DSC_TYPES_SAFETY_SYSTEM_DIAGNOSTIC_SESSION  4

#define UDS_ER_TYPES_HARD_RESET                   1
#define UDS_ER_TYPES_KEY_OFF_ON_RESET             2
#define UDS_ER_TYPES_SOFT_RESET                   3
#define UDS_ER_TYPES_ENABLE_RAPID_POWER_SHUTDOWN  4
#define UDS_ER_TYPES_DISABLE_RAPID_POWER_SHUTDOWN 5

#define UDS_ER_TYPE_ENABLE_RAPID_POWER_SHUTDOWN_INVALID 0xFF

#define UDS_RDTCI_TYPES_NUMBER_BY_STATUS_MASK     0x1
#define UDS_RDTCI_TYPES_BY_STATUS_MASK            0x2
#define UDS_RDTCI_TYPES_SNAPSHOT_IDENTIFICATION   0x3
#define UDS_RDTCI_TYPES_SNAPSHOT_RECORD_BY_DTC    0x4
#define UDS_RDTCI_TYPES_SNAPSHOT_RECORD_BY_RECORD 0x5
#define UDS_RDTCI_TYPES_EXTENDED_RECORD_BY_DTC    0x6
#define UDS_RDTCI_TYPES_NUM_DTC_BY_SEVERITY_MASK  0x7
#define UDS_RDTCI_TYPES_BY_SEVERITY_MASK          0x8
#define UDS_RDTCI_TYPES_SEVERITY_INFO_OF_DTC      0x9
#define UDS_RDTCI_TYPES_SUPPORTED_DTC             0xA
#define UDS_RDTCI_TYPES_FIRST_TEST_FAILED_DTC     0xB
#define UDS_RDTCI_TYPES_FIRST_CONFIRMED_DTC       0xC
#define UDS_RDTCI_TYPES_MOST_RECENT_TEST_FAILED   0xD
#define UDS_RDTCI_TYPES_MOST_RECENT_CONFIRMED_DTC 0xE
#define UDS_RDTCI_TYPES_OUTDATED_RMMDTCBSM        0xF
#define UDS_RDTCI_TYPES_OUTDATED_RMMDEDRBDN       0x10
#define UDS_RDTCI_TYPES_OUTDATED_RNOMMDTCBSM      0x11
#define UDS_RDTCI_TYPES_OUTDATED_RNOOEOBDDTCBSM   0x12
#define UDS_RDTCI_TYPES_OUTDATED_ROBDDTCBSM       0x13
#define UDS_RDTCI_TYPES_DTC_FAULT_DETECT_CTR      0x14
#define UDS_RDTCI_TYPES_DTC_WITH_PERM_STATUS      0x15
#define UDS_RDTCI_TYPES_DTC_EXT_DATA_REC_BY_NUM   0x16
#define UDS_RDTCI_TYPES_USER_MEM_DTC_BY_STATUS_M  0x17
#define UDS_RDTCI_TYPES_USER_MEM_DTC_REC_BY_DTC_N 0x18
#define UDS_RDTCI_TYPES_USER_MEM_DTC_EXT_REC_BY_N 0x19
#define UDS_RDTCI_TYPES_SUP_DTC_EXT_RECORD        0x1A
#define UDS_RDTCI_TYPES_WWH_OBD_DTC_BY_MASK_REC   0x42
#define UDS_RDTCI_TYPES_WWH_OBD_DTC_PERM_STATUS   0x55
#define UDS_RDTCI_TYPES_WWH_OBD_BY_GROUP_READY    0x56

#define UDS_RDTCI_DTC_STATUS_TEST_FAILED                      0x01
#define UDS_RDTCI_DTC_STATUS_TEST_FAILED_THIS_OPER_CYCLE      0x02
#define UDS_RDTCI_DTC_STATUS_PENDING_DTC                      0x04
#define UDS_RDTCI_DTC_STATUS_CONFIRMED_DTC                    0x08
#define UDS_RDTCI_DTC_STATUS_TEST_NOT_COMPL_SINCE_LAST_CLEAR  0x10
#define UDS_RDTCI_DTC_STATUS_TEST_FAILED_SINCE_LAST_CLEAR     0x20
#define UDS_RDTCI_DTC_STATUS_TEST_NOT_COMPL_THIS_OPER_CYCLE   0x40
#define UDS_RDTCI_DTC_STATUS_WARNING_INDICATOR_REQUESTED      0x80

#define UDS_RSDBI_DATA_TYPE_UNSIGNED_NUM                    0x00
#define UDS_RSDBI_DATA_TYPE_SIGNED_NUM                      0x01
#define UDS_RSDBI_DATA_TYPE_BITMAPPED_REPORTED_WO_MAP       0x02
#define UDS_RSDBI_DATA_TYPE_BITMAPPED_REPORTED_WITH_MAP     0x03
#define UDS_RSDBI_DATA_TYPE_BINARY_CODED_DECIMAL            0x04
#define UDS_RSDBI_DATA_TYPE_STATE_ENCODED_VARIABLE          0x05
#define UDS_RSDBI_DATA_TYPE_ASCII                           0x06
#define UDS_RSDBI_DATA_TYPE_SIGNED_FLOAT                    0x07
#define UDS_RSDBI_DATA_TYPE_PACKET                          0x08
#define UDS_RSDBI_DATA_TYPE_FORMULA                         0x09
#define UDS_RSDBI_DATA_TYPE_UNIT_FORMAT                     0x0a
#define UDS_RSDBI_DATA_TYPE_STATE_AND_CONNECTION_TYPE       0x0b

#define UDS_SA_TYPES_RESERVED                     0x00
#define UDS_SA_TYPES_REQUEST_SEED                 0x01
#define UDS_SA_TYPES_SEND_KEY                     0x02
#define UDS_SA_TYPES_REQUEST_SEED_ISO26021        0x03
#define UDS_SA_TYPES_SEND_KEY_ISO26021            0x04
#define UDS_SA_TYPES_SUPPLIER                     0xFE
#define UDS_SA_TYPES_UNCLEAR                      0xFF

#define UDS_CC_TYPES_ENABLE_RX_AND_TX                            0
#define UDS_CC_TYPES_ENABLE_RX_AND_DISABLE_TX                    1
#define UDS_CC_TYPES_DISABLE_RX_AND_ENABLE_TX                    2
#define UDS_CC_TYPES_DISABLE_RX_AND_TX                           3
#define UDS_CC_TYPES_ENABLE_RX_AND_DISABLE_TX_WITH_ENH_ADDR_INFO 4
#define UDS_CC_TYPES_ENABLE_RX_AND_TX_WITH_ENH_ADDR_INFO         5

#define UDS_CC_COMM_TYPE_COMM_TYPE_MASK             0x03
#define UDS_CC_COMM_TYPE_SUBNET_NUMBER_MASK         0xF0

#define UDS_ARS_TYPES_DEAUTHENTICATE                0x00
#define UDS_ARS_TYPES_VERIFY_CERT_UNIDIRECTIONAL    0x01
#define UDS_ARS_TYPES_VERIFY_CERT_BIDIRECTIONAL     0x02
#define UDS_ARS_TYPES_PROOF_OF_OWNERSHIP            0x03
#define UDS_ARS_TYPES_TRANSMIT_CERTIFICATE          0x04
#define UDS_ARS_TYPES_REQUEST_CHALLENGE_FOR_AUTH    0x05
#define UDS_ARS_TYPES_VERIFY_PROOF_OF_OWN_UNIDIR    0x06
#define UDS_ARS_TYPES_VERIFY_PROOF_OF_OWN_BIDIR     0x07
#define UDS_ARS_TYPES_AUTH_CONFIGURATION            0x08

#define UDS_ARS_AUTH_RET_REQUEST_ACCEPTED           0x00
#define UDS_ARS_AUTH_RET_GENERAL_REJECT             0x01
#define UDS_ARS_AUTH_RET_AUTH_CONFIG_APCE           0x02
#define UDS_ARS_AUTH_RET_AUTH_CONFIG_ACR_SYM        0x03
#define UDS_ARS_AUTH_RET_AUTH_CONFIG_ACR_ASYM       0x04
#define UDS_ARS_AUTH_RET_DEAUTH_SUCCESS             0x10
#define UDS_ARS_AUTH_RET_CERT_VER_OWN_VER_NEC       0x11
#define UDS_ARS_AUTH_RET_OWN_VER_AUTH_COMPL         0x12
#define UDS_ARS_AUTH_RET_CERT_VERIFIED              0x13

#define UDS_DDDI_TYPES_DEFINE_BY_IDENTIFIER         0x01
#define UDS_DDDI_TYPES_DEFINE_BY_MEM_ADDRESS        0x02
#define UDS_DDDI_TYPES_CLEAR_DYN_DEF_DATA_ID        0x03

#define UDS_IOCBI_PARAMETERS_RETURN_CONTROL_TO_ECU  0
#define UDS_IOCBI_PARAMETERS_RESET_TO_DEFAULT       1
#define UDS_IOCBI_PARAMETERS_FREEZE_CURRENT_STATE   2
#define UDS_IOCBI_PARAMETERS_SHORT_TERM_ADJUSTMENT  3

#define UDS_RC_TYPES_START   1
#define UDS_RC_TYPES_STOP    2
#define UDS_RC_TYPES_REQUEST 3

#define UDS_RD_COMPRESSION_METHOD_MASK          0xF0
#define UDS_RD_ENCRYPTING_METHOD_MASK           0x0F
#define UDS_RD_MEMORY_SIZE_LENGTH_MASK          0xF0
#define UDS_RD_MEMORY_ADDRESS_LENGTH_MASK       0x0F
#define UDS_RD_MAX_BLOCK_LEN_LEN_MASK           0xF0

#define UDS_RFT_MODE_ADD_FILE                   0x01
#define UDS_RFT_MODE_DELETE_FILE                0x02
#define UDS_RFT_MODE_REPLACE_FILE               0x03
#define UDS_RFT_MODE_READ_FILE                  0x04
#define UDS_RFT_MODE_READ_DIR                   0x05
#define UDS_RFT_MODE_RESUME_FILE                0x06

#define UDS_SDT_ADMIN_PARAM_REQ                 0x0001
#define UDS_SDT_ADMIN_PARAM_PRE_ESTABL_KEY      0x0008
#define UDS_SDT_ADMIN_PARAM_ENCRYPTED           0x0010
#define UDS_SDT_ADMIN_PARAM_SIGNED              0x0020
#define UDS_SDT_ADMIN_PARAM_SIGN_ON_RESP_REQ    0x0040

#define UDS_CDTCS_ACTIONS_ON  1
#define UDS_CDTCS_ACTIONS_OFF 2

#define UDS_LC_TYPES_VMTWFP 1
#define UDS_LC_TYPES_VMTWSP 2
#define UDS_LC_TYPES_TM     3

#define UDS_DID_BSIDID          0xF180
#define UDS_DID_ASIDID          0xF181
#define UDS_DID_ADIDID          0xF182
#define UDS_DID_BSFPDID         0xF183
#define UDS_DID_ASFPDID         0xF184
#define UDS_DID_ADFPDID         0xF185
#define UDS_DID_ADSDID          0xF186
#define UDS_DID_VMSPNDID        0xF187
#define UDS_DID_VMECUSNDID      0xF188
#define UDS_DID_VMECUSVNDID     0xF189
#define UDS_DID_SSIDDID         0xF18A
#define UDS_DID_ECUMDDID        0xF18B
#define UDS_DID_ECUSNDID        0xF18C
#define UDS_DID_SFUDID          0xF18D
#define UDS_DID_VMKAPNDID       0xF18E
#define UDS_DID_RXSWIN          0xF18F
#define UDS_DID_VINDID          0xF190
#define UDS_DID_VMECUHNDID      0xF191
#define UDS_DID_SSECUHWNDID     0xF192
#define UDS_DID_SSECUHWVNDID    0xF193
#define UDS_DID_SSECUSWNDID     0xF194
#define UDS_DID_SSECUSWVNDID    0xF195
#define UDS_DID_EROTANDID       0xF196
#define UDS_DID_SNOETDID        0xF197
#define UDS_DID_RSCOTSNDID      0xF198
#define UDS_DID_PDDID           0xF199
#define UDS_DID_CRSCOCESNDID    0xF19A
#define UDS_DID_CDDID           0xF19B
#define UDS_DID_CESWNDID        0xF19D
#define UDS_DID_EIDDID          0xF19D
#define UDS_DID_ODXFDID         0xF19E
#define UDS_DID_EDID            0xF19F
#define UDS_DID_ADDID_FA00      0xFA00
#define UDS_DID_ADDID_FA01      0xFA01
#define UDS_DID_ADDID_FA02      0xFA02
#define UDS_DID_ADDID_FA03      0xFA03
#define UDS_DID_ADDID_FA04      0xFA04
#define UDS_DID_ADDID_FA05      0xFA05
#define UDS_DID_ADDID_FA06      0xFA06
#define UDS_DID_ADDID_FA07      0xFA07
#define UDS_DID_ADDID_FA08      0xFA08
#define UDS_DID_ADDID_FA09      0xFA09
#define UDS_DID_ADDID_FA0A      0xFA0A
#define UDS_DID_ADDID_FA0B      0xFA0B
#define UDS_DID_ADDID_FA0C      0xFA0C
#define UDS_DID_ADDID_FA0D      0xFA0D
#define UDS_DID_ADDID_FA0E      0xFA0E
#define UDS_DID_ADDID_FA0F      0xFA0F
#define UDS_DID_NOEDRD          0xFA10
#define UDS_DID_EDRI            0xFA11
#define UDS_DID_EDRDAI          0xFA12
#define UDS_DID_UDSVDID         0xFF00
#define UDS_DID_RESRVDCPADLC    0xFF01

#define UDS_RID_EXSPLRI_        0xE200
#define UDS_RID_DLRI_           0xE201
#define UDS_RID_EM_             0xFF00
#define UDS_RID_CPD_            0xFF01
#define UDS_RID_FF02            0xFF02


#define UDS_SERVICES_MIN     0x10
#define UDS_SERVICES_DSC     0x10
#define UDS_SERVICES_ER      0x11
#define UDS_SERVICES_CDTCI   0x14
#define UDS_SERVICES_RDTCI   0x19
#define UDS_SERVICES_RDBI    0x22
#define UDS_SERVICES_RMBA    0x23
#define UDS_SERVICES_RSDBI   0x24
#define UDS_SERVICES_SA      0x27
#define UDS_SERVICES_CC      0x28
#define UDS_SERVICES_ARS     0x29
#define UDS_SERVICES_RDBPI   0x2A
#define UDS_SERVICES_DDDI    0x2C
#define UDS_SERVICES_WDBI    0x2E
#define UDS_SERVICES_IOCBI   0x2F
#define UDS_SERVICES_RC      0x31
#define UDS_SERVICES_RD      0x34
#define UDS_SERVICES_RU      0x35
#define UDS_SERVICES_TD      0x36
#define UDS_SERVICES_RTE     0x37
#define UDS_SERVICES_RFT     0x38
#define UDS_SERVICES_WMBA    0x3D
#define UDS_SERVICES_TP      0x3E
#define UDS_SERVICES_ERR     0x3F
#define UDS_SERVICES_SDT     0x84
#define UDS_SERVICES_CDTCS   0x85
#define UDS_SERVICES_ROE     0x86
#define UDS_SERVICES_LC      0x87

#ifdef __HAVE_UDS_DESC_INFO__
const char *dm_desc_uds_services(INT32U       value);
const char *dm_desc_uds_response_codes(INT32U        value);
const char *dm_desc_uds_dsc_types(INT32U       value);
const char *dm_desc_uds_er_types(INT32U       value);
const char *dm_desc_uds_cdtci_group_of_dtc(INT32U value);
const char *dm_desc_uds_rdtci_types(INT32U value);
const char *dm_desc_uds_rdtci_format_id_types(INT32U value);
const char *dm_desc_uds_rsdbi_data_types(INT32U value);
const char *dm_desc_uds_cc_types(INT32U value);
const char *dm_desc_uds_cc_comm_types(INT32U value);
const char *dm_desc_uds_cc_subnet_number_types(INT32U value);
const char *dm_desc_uds_ars_types(INT32U value);
const char *dm_desc_uds_ars_auth_ret_types(INT32U value);
const char *dm_desc_uds_rdbpi_transmission_mode(INT32U value);
const char *dm_desc_uds_dddi_types(INT32U value);
const char *dm_desc_uds_iocbi_parameters(INT32U value);
const char *dm_desc_uds_rc_types(INT32U value);
const char *dm_desc_uds_rft_mode_types(INT32U value);
const char *dm_desc_uds_cdtcs_types(INT32U value);
const char *dm_desc_uds_lc_types(INT32U value);
const char *dm_desc_uds_standard_did_types(INT32U value);
const char *dm_desc_uds_did_resrvdcpadlc_types(INT32U value);
const char *dm_desc_uds_standard_rid_types(INT32U value);
#endif /* __HAVE_UDS_DESC_INFO__ */

#endif /* __DM_UDSC_TYPES_H__ */
