#include <string.h>
#include <stdlib.h>
#include "dm_udsc_types.h"


#ifdef __HAVE_UDS_DESC_INFO__
/*
 * Enums
 */

/* Services */
static const value_string uds_services[]= {
    {OBD_SERVICES_0x01,  "OBD - Request Current Powertrain Diagnostic Data"},
    {OBD_SERVICES_0x02,  "OBD - Request Powertrain Freeze Frame Data"},
    {OBD_SERVICES_0x03,  "OBD - Request Emission-Related Diagnostic Trouble Codes"},
    {OBD_SERVICES_0x04,  "OBD - Clear/Reset Emission-Related Diagnostic Information"},
    {OBD_SERVICES_0x05,  "OBD - Request Oxygen Sensor Monitoring Test Results"},
    {OBD_SERVICES_0x06,  "OBD - Request On-Board Monitoring Test Results for Specific Monitored Systems"},
    {OBD_SERVICES_0x07,  "OBD - Request Emission-Related Diagnostic Trouble Codes Detected During Current or Last Completed Driving Cycle"},
    {OBD_SERVICES_0x08,  "OBD - Request Control of On-Board System, Test or Component"},
    {OBD_SERVICES_0x09,  "OBD - Request Vehicle Information"},
    {OBD_SERVICES_0x0A,  "OBD - Request Emission-Related Diagnostic Trouble Codes with Permanent Status"},
    {OBD_SERVICES_0x0B,  "OBD - Unknown Service"},
    {OBD_SERVICES_0x0C,  "OBD - Unknown Service"},
    {OBD_SERVICES_0x0D,  "OBD - Unknown Service"},
    {OBD_SERVICES_0x0E,  "OBD - Unknown Service"},
    {OBD_SERVICES_0x0F,  "OBD - Unknown Service"},

    {UDS_SERVICES_DSC,   "Diagnostic Session Control"},
    {UDS_SERVICES_ER,    "ECU Reset"},
    {UDS_SERVICES_CDTCI, "Clear Diagnostic Information"},
    {UDS_SERVICES_RDTCI, "Read DTC Information"},
    {UDS_SERVICES_RDBI,  "Read Data By Identifier"},
    {UDS_SERVICES_RMBA,  "Read Memory By Address"},
    {UDS_SERVICES_RSDBI, "Read Scaling Data By Identifier"},
    {UDS_SERVICES_SA,    "Security Access"},
    {UDS_SERVICES_CC,    "Communication Control"},
    {UDS_SERVICES_ARS,   "Authentication"},
    {UDS_SERVICES_RDBPI, "Read Data By Periodic Identifier"},
    {UDS_SERVICES_DDDI,  "Dynamically Define Data Identifier"},
    {UDS_SERVICES_WDBI,  "Write Data By Identifier"},
    {UDS_SERVICES_IOCBI, "Input Output Control By Identifier"},
    {UDS_SERVICES_RC,    "Routine Control"},
    {UDS_SERVICES_RD,    "Request Download"},
    {UDS_SERVICES_RU,    "Request Upload"},
    {UDS_SERVICES_TD,    "Transfer Data"},
    {UDS_SERVICES_RTE,   "Request Transfer Exit"},
    {UDS_SERVICES_RFT,   "Request File Transfer"},
    {UDS_SERVICES_WMBA,  "Write Memory By Address"},
    {UDS_SERVICES_TP,    "Tester Present"},
    {UDS_SERVICES_ERR,   "Error"},
    {UDS_SERVICES_SDT,   "Secured Data Transmission"},
    {UDS_SERVICES_CDTCS, "Control DTC Setting"},
    {UDS_SERVICES_ROE,   "Response On Event"},
    {UDS_SERVICES_LC,    "Link Control"},
    {0, NULL}
};

const char *dm_desc_uds_services(INT32U      value)
{
    int i = 0;

    for (i = 0; uds_services[i].strptr; i++) {
        if (uds_services[i].value == value) {
            return uds_services[i].strptr;
        }
    }

    return "Unknown";
}


/* Response code */
static const value_string uds_response_codes[]= {
    {UDS_RESPONSE_CODES_GR,      "General reject"},
    {UDS_RESPONSE_CODES_SNS,     "Service not supported"},
    {UDS_RESPONSE_CODES_SFNS,    "SubFunction Not Supported"},
    {UDS_RESPONSE_CODES_IMLOIF,  "Incorrect Message Length or Invalid Format"},
    {UDS_RESPONSE_CODES_RTL,     "Response too long"},
    {UDS_RESPONSE_CODES_BRR,     "Busy repeat request"},
    {UDS_RESPONSE_CODES_CNC,     "Conditions Not Correct"},
    {UDS_RESPONSE_CODES_RSE,     "Request Sequence Error"},
    {UDS_RESPONSE_CODES_NRFSC,   "No response from sub-net component"},
    {UDS_RESPONSE_CODES_FPEORA,  "Failure prevents execution of requested action"},
    {UDS_RESPONSE_CODES_ROOR,    "Request Out of Range"},
    {UDS_RESPONSE_CODES_SAD,     "Security Access Denied"},
    {UDS_RESPONSE_CODES_AR,      "Authentication Required"},
    {UDS_RESPONSE_CODES_IK,      "Invalid Key"},
    {UDS_RESPONSE_CODES_ENOA,    "Exceeded Number Of Attempts"},
    {UDS_RESPONSE_CODES_RTDNE,   "Required Time Delay Not Expired"},
    {UDS_RESPONSE_CODES_SDTR,    "Secure Data Transmission Required"},
    {UDS_RESPONSE_CODES_SDTNA,   "Secure Data Transmission Not Allowed"},
    {UDS_RESPONSE_CODES_SDTF,    "Secure Data Verification Failed"},
    {UDS_RESPONSE_CODES_CVFITP,  "Certificate Verification Failed: Invalid Time Period"},
    {UDS_RESPONSE_CODES_CVFIS,   "Certificate Verification Failed: Invalid Signature"},
    {UDS_RESPONSE_CODES_CVFICOT, "Certificate Verification Failed: Invalid Chain of Trust"},
    {UDS_RESPONSE_CODES_CVFIT,   "Certificate Verification Failed: Invalid Type"},
    {UDS_RESPONSE_CODES_CVFIF,   "Certificate Verification Failed: Invalid Format"},
    {UDS_RESPONSE_CODES_CVFIC,   "Certificate Verification Failed: Invalid Content"},
    {UDS_RESPONSE_CODES_CVFISD,  "Certificate Verification Failed: Invalid Scope"},
    {UDS_RESPONSE_CODES_CVFICR,  "Certificate Verification Failed: Invalid Certificate (revoked)"},
    {UDS_RESPONSE_CODES_OVF,     "Ownership Verification Failed"},
    {UDS_RESPONSE_CODES_CCF,     "Challenge Calculation Failed"},
    {UDS_RESPONSE_CODES_SARF,    "Setting Access Rights Failed"},
    {UDS_RESPONSE_CODES_SKCDF,   "Session Key Creation/Derivation Failed"},
    {UDS_RESPONSE_CODES_CDUF,    "Configuration Data Usage Failed"},
    {UDS_RESPONSE_CODES_DAF,     "DeAuthentication Failed"},
    {UDS_RESPONSE_CODES_UDNA,    "Upload/Download not accepted"},
    {UDS_RESPONSE_CODES_TDS,     "Transfer data suspended"},
    {UDS_RESPONSE_CODES_GPF,     "General Programming Failure"},
    {UDS_RESPONSE_CODES_WBSC,    "Wrong Block Sequence Counter"},
    {UDS_RESPONSE_CODES_RCRRP,   "Request correctly received, but response is pending"},
    {UDS_RESPONSE_CODES_SFNSIAS, "Subfunction not supported in active session"},
    {UDS_RESPONSE_CODES_SNSIAS,  "Service not supported in active session"},
    {UDS_RESPONSE_CODES_RPMTH,   "RPM Too High"},
    {UDS_RESPONSE_CODES_RPMTL,   "RPM Too Low"},
    {UDS_RESPONSE_CODES_EIR,     "Engine Is Running"},
    {UDS_RESPONSE_CODES_EINR,    "Engine Is Not Running"},
    {UDS_RESPONSE_CODES_ERTTL,   "Run Time Too Low"},
    {UDS_RESPONSE_CODES_TEMPTH,  "Temperature Too High"},
    {UDS_RESPONSE_CODES_TEMPTL,  "Temperature Too Low"},
    {UDS_RESPONSE_CODES_VSTH,    "Vehicle Speed Too High"},
    {UDS_RESPONSE_CODES_VSTL,    "Vehicle Speed Too Low"},
    {UDS_RESPONSE_CODES_TPTH,    "Throttle/Pedal Too High"},
    {UDS_RESPONSE_CODES_TPTL,    "Throttle/Pedal Too Low"},
    {UDS_RESPONSE_CODES_TRNIN,   "Transmission Range Not In Neutral"},
    {UDS_RESPONSE_CODES_TRNIG,   "Transmission Range Not In Gear"},
    {UDS_RESPONSE_CODES_BSNC,    "Brake Switch(es) Not Closed"},
    {UDS_RESPONSE_CODES_SLNIP,   "Shifter/Lever Not in Park"},
    {UDS_RESPONSE_CODES_TCCL,    "Torque Converter Clutch Locked"},
    {UDS_RESPONSE_CODES_VTH,     "Voltage Too High"},
    {UDS_RESPONSE_CODES_VTL,     "Voltage Too Low"},
    {UDS_RESPONSE_CODES_RTNA,    "Resource Temporarily Not Available"},
    {0, NULL}
};

const char *dm_desc_uds_response_codes(INT32U        value)
{
    int i = 0;

    for (i = 0; uds_response_codes[i].strptr; i++) {
        if (uds_response_codes[i].value == value) {
            return uds_response_codes[i].strptr;
        }
    }

    return "Unknown";
}

/* DSC */
static const value_string uds_dsc_types[] = {
    {0,                                             "Reserved"},
    {UDS_DSC_TYPES_DEFAULT_SESSION,                 "Default Session"},
    {UDS_DSC_TYPES_PROGRAMMING_SESSION,             "Programming Session"},
    {UDS_DSC_TYPES_EXTENDED_DIAGNOSTIC_SESSION,     "Extended Diagnostic Session"},
    {UDS_DSC_TYPES_SAFETY_SYSTEM_DIAGNOSTIC_SESSION, "Safety System Diagnostic Session"},
    {0, NULL}
};

const char *dm_desc_uds_dsc_types(INT32U       value)
{
    int i = 0;

    for (i = 0; uds_dsc_types[i].strptr; i++) {
        if (uds_dsc_types[i].value == value) {
            return uds_dsc_types[i].strptr;
        }
    }

    return "Unknown";
}

/* ER */
static const value_string uds_er_types[] = {
    {0,                                         "Reserved"},
    {UDS_ER_TYPES_HARD_RESET,                   "Hard Reset"},
    {UDS_ER_TYPES_KEY_OFF_ON_RESET,             "Key Off On Reset"},
    {UDS_ER_TYPES_SOFT_RESET,                   "Soft Reset"},
    {UDS_ER_TYPES_ENABLE_RAPID_POWER_SHUTDOWN,  "Enable Rapid Power Shutdown"},
    {UDS_ER_TYPES_DISABLE_RAPID_POWER_SHUTDOWN, "Disable Rapid Power Shutdown"},
    {0, NULL}
};
const char *dm_desc_uds_er_types(INT32U       value)
{
    int i = 0;

    for (i = 0; uds_er_types[i].strptr; i++) {
        if (uds_er_types[i].value == value) {
            return uds_er_types[i].strptr;
        }
    }

    return "Unknown";
}

/* CDTCI */
static const value_string uds_cdtci_group_of_dtc[] = {
        {0xffff33, "Emissions-system group"},
        {0xffffd0, "Safety-system group"},
        {0xfffffe, "VOBD system"},
        {0xffffff, "All Groups (all DTCs)"},
        {0, NULL}
};
const char *dm_desc_uds_cdtci_group_of_dtc(INT32U value)
{
    int i = 0;

    for (i = 0; uds_cdtci_group_of_dtc[i].strptr; i++) {
        if (uds_cdtci_group_of_dtc[i].value == value) {
            return uds_cdtci_group_of_dtc[i].strptr;
        }
    }

    return "Unknown";
}


/* RDTCI */
static const value_string uds_rdtci_types[] = {
    {UDS_RDTCI_TYPES_NUMBER_BY_STATUS_MASK,     "Report Number of DTC by Status Mask"},
    {UDS_RDTCI_TYPES_BY_STATUS_MASK,            "Report DTC by Status Mask"},
    {UDS_RDTCI_TYPES_SNAPSHOT_IDENTIFICATION,   "Report DTC Snapshot Identification"},
    {UDS_RDTCI_TYPES_SNAPSHOT_RECORD_BY_DTC,    "Report DTC Snapshot Record by DTC Number"},
    {UDS_RDTCI_TYPES_SNAPSHOT_RECORD_BY_RECORD, "Report DTC Snapshot Record by Record Number"},
    {UDS_RDTCI_TYPES_EXTENDED_RECORD_BY_DTC,    "Report DTC Extended Data Record by DTC Number"},
    {UDS_RDTCI_TYPES_NUM_DTC_BY_SEVERITY_MASK,  "Report Number of DTC By Severity Mask"},
    {UDS_RDTCI_TYPES_BY_SEVERITY_MASK,          "Report DTC by Severity Mask"},
    {UDS_RDTCI_TYPES_SEVERITY_INFO_OF_DTC,      "Report Severity Information of DTC"},
    {UDS_RDTCI_TYPES_SUPPORTED_DTC,             "Report Supported DTC"},
    {UDS_RDTCI_TYPES_FIRST_TEST_FAILED_DTC,     "Report First Test Failed DTC"},
    {UDS_RDTCI_TYPES_FIRST_CONFIRMED_DTC,       "Report First Confirmed DTC"},
    {UDS_RDTCI_TYPES_MOST_RECENT_TEST_FAILED,   "Report Most Recent Test Failed DTC"},
    {UDS_RDTCI_TYPES_MOST_RECENT_CONFIRMED_DTC, "Report Most Recent Confirmed DTC"},
    {UDS_RDTCI_TYPES_OUTDATED_RMMDTCBSM,        "Report Mirror Memory DTC By Status Mask (outdated 2013 revision)"},
    {UDS_RDTCI_TYPES_OUTDATED_RMMDEDRBDN,       "Report Mirror Memory DTC Ext Data Record by DTC Number (outdated 2013 revision)"},
    {UDS_RDTCI_TYPES_OUTDATED_RNOMMDTCBSM,      "Report Number of Mirror Memory DTC by Status Mask (outdated 2013 revision)"},
    {UDS_RDTCI_TYPES_OUTDATED_RNOOEOBDDTCBSM,   "Report Number of Emissions OBD DTC by Status Mask (outdated 2013 revision)"},
    {UDS_RDTCI_TYPES_OUTDATED_ROBDDTCBSM,       "Report Emissions OBD DTC By Status Mask (outdated 2013 revision)"},
    {UDS_RDTCI_TYPES_DTC_FAULT_DETECT_CTR,      "Report DTC Fault Detection Counter"},
    {UDS_RDTCI_TYPES_DTC_WITH_PERM_STATUS,      "Report DTC with Permanent Status"},
    {UDS_RDTCI_TYPES_DTC_EXT_DATA_REC_BY_NUM,   "Report DTC Extended Data Record by Record Number"},
    {UDS_RDTCI_TYPES_USER_MEM_DTC_BY_STATUS_M,  "Report User Defined Memory DTC By Status Mask"},
    {UDS_RDTCI_TYPES_USER_MEM_DTC_REC_BY_DTC_N, "Report User Defined Memory DTC Snapshot Record By DTC Number"},
    {UDS_RDTCI_TYPES_USER_MEM_DTC_EXT_REC_BY_N, "Report User Defined Memory DTC Extended Data Record by DTC Number"},
    {UDS_RDTCI_TYPES_SUP_DTC_EXT_RECORD,        "Report List of DTCs Supporting Specific Extended Data Record"},
    {UDS_RDTCI_TYPES_WWH_OBD_DTC_BY_MASK_REC,   "Report WWH-OBD DTC By Mask Record"},
    {UDS_RDTCI_TYPES_WWH_OBD_DTC_PERM_STATUS,   "Report WWH-OBD DTC With Permanent Status"},
    {UDS_RDTCI_TYPES_WWH_OBD_BY_GROUP_READY,    "Report WWH-OBD DTC By Readiness Group Identifier"},
    {0, NULL}
};
const char *dm_desc_uds_rdtci_types(INT32U value)
{
    int i = 0;

    for (i = 0; uds_rdtci_types[i].strptr; i++) {
        if (uds_rdtci_types[i].value == value) {
            return uds_rdtci_types[i].strptr;
        }
    }

    return "Unknown";
}


static const value_string uds_rdtci_format_id_types[] = {
    {0x00, "SAE J2012-DA DTC Format 00"},
    {0x01, "ISO 14229-1 DTC Format"},
    {0x02, "SAE J1939-73 DTC Format"},
    {0x03, "ISO 11992-4 DTC Format"},
    {0x04, "SAE J2012-DA DTC Format 04"},
    {0, NULL}
};
const char *dm_desc_uds_rdtci_format_id_types(INT32U value)
{
    int i = 0;

    for (i = 0; uds_rdtci_format_id_types[i].strptr; i++) {
        if (uds_rdtci_format_id_types[i].value == value) {
            return uds_rdtci_format_id_types[i].strptr;
        }
    }

    return "Unknown";
}

/* RSDBI */
static const value_string uds_rsdbi_data_types[] = {
    {UDS_RSDBI_DATA_TYPE_UNSIGNED_NUM,                  "Unsigned Numeric"},
    {UDS_RSDBI_DATA_TYPE_SIGNED_NUM,                    "Signed Numeric"},
    {UDS_RSDBI_DATA_TYPE_BITMAPPED_REPORTED_WO_MAP,     "Bit Mapped Reported Without Mask"},
    {UDS_RSDBI_DATA_TYPE_BITMAPPED_REPORTED_WITH_MAP,   "Bit Mapped Reported With Mask"},
    {UDS_RSDBI_DATA_TYPE_BINARY_CODED_DECIMAL,          "Binary Coded Decimal"},
    {UDS_RSDBI_DATA_TYPE_STATE_ENCODED_VARIABLE,        "State Encoded Variable"},
    {UDS_RSDBI_DATA_TYPE_ASCII,                         "ASCII"},
    {UDS_RSDBI_DATA_TYPE_SIGNED_FLOAT,                  "Signed Floating Point"},
    {UDS_RSDBI_DATA_TYPE_PACKET,                        "Packet"},
    {UDS_RSDBI_DATA_TYPE_FORMULA,                       "Formula"},
    {UDS_RSDBI_DATA_TYPE_UNIT_FORMAT,                   "Unit/Format"},
    {UDS_RSDBI_DATA_TYPE_STATE_AND_CONNECTION_TYPE,     "State And Connection Type"},
    {0, NULL}
};
const char *dm_desc_uds_rsdbi_data_types(INT32U value)
{
    int i = 0;

    for (i = 0; uds_rsdbi_data_types[i].strptr; i++) {
        if (uds_rsdbi_data_types[i].value == value) {
            return uds_rsdbi_data_types[i].strptr;
        }
    }

    return "Unknown";
}

/* CC */
static const value_string uds_cc_types[] = {
    {UDS_CC_TYPES_ENABLE_RX_AND_TX,                             "Enable RX and TX"},
    {UDS_CC_TYPES_ENABLE_RX_AND_DISABLE_TX,                     "Enable RX and Disable TX"},
    {UDS_CC_TYPES_DISABLE_RX_AND_ENABLE_TX,                     "Disable RX and Enable TX"},
    {UDS_CC_TYPES_DISABLE_RX_AND_TX,                            "Disable RX and TX"},
    {UDS_CC_TYPES_ENABLE_RX_AND_DISABLE_TX_WITH_ENH_ADDR_INFO,  "Enable RX and Disable TX with Enhanced Address Information"},
    {UDS_CC_TYPES_ENABLE_RX_AND_TX_WITH_ENH_ADDR_INFO,          "Enable RX and TX with Enhanced Address Information"},
    {0, NULL}
};
const char *dm_desc_uds_cc_types(INT32U value)
{
    int i = 0;

    for (i = 0; uds_cc_types[i].strptr; i++) {
        if (uds_cc_types[i].value == value) {
            return uds_cc_types[i].strptr;
        }
    }

    return "Unknown";
}


static const value_string uds_cc_comm_types[] = {
    {0, "Reserved"},
    {1, "Normal Communication Messages"},
    {2, "Network Management Communication Messages"},
    {3, "Network Management and Normal Communication Messages"},
    {0, NULL}
};
const char *dm_desc_uds_cc_comm_types(INT32U value)
{
    int i = 0;

    for (i = 0; uds_cc_comm_types[i].strptr; i++) {
        if (uds_cc_comm_types[i].value == value) {
            return uds_cc_comm_types[i].strptr;
        }
    }

    return "Unknown";
}

static const value_string uds_cc_subnet_number_types[] = {
    {0x0, "Disable/Enable specified Communication Type "},
    /* 0x1 .. 0xe specific subnets numbers */
    {0xf, "Disable/Enable network which request is received on"},
    {0, NULL}
};
const char *dm_desc_uds_cc_subnet_number_types(INT32U value)
{
    int i = 0;

    for (i = 0; uds_cc_subnet_number_types[i].strptr; i++) {
        if (uds_cc_subnet_number_types[i].value == value) {
            return uds_cc_subnet_number_types[i].strptr;
        }
    }

    return "Unknown";
}


/* ARS */
static const value_string uds_ars_types[] = {
    {UDS_ARS_TYPES_DEAUTHENTICATE,              "DeAuthenticate"},
    {UDS_ARS_TYPES_VERIFY_CERT_UNIDIRECTIONAL,  "Verify Certificate Unidirectional"},
    {UDS_ARS_TYPES_VERIFY_CERT_BIDIRECTIONAL,   "Verify Certificate Bidirectional"},
    {UDS_ARS_TYPES_PROOF_OF_OWNERSHIP,          "Proof of Ownership"},
    {UDS_ARS_TYPES_TRANSMIT_CERTIFICATE,        "Transmit Certificate"},
    {UDS_ARS_TYPES_REQUEST_CHALLENGE_FOR_AUTH,  "Request Challenge for Authentication"},
    {UDS_ARS_TYPES_VERIFY_PROOF_OF_OWN_UNIDIR,  "Verify Proof of Ownership Unidirectional"},
    {UDS_ARS_TYPES_VERIFY_PROOF_OF_OWN_BIDIR,   "Verify Proof of Ownership Bidirectional"},
    {UDS_ARS_TYPES_AUTH_CONFIGURATION,          "Authentication Configuration"},
    {0, NULL}
};
const char *dm_desc_uds_ars_types(INT32U value)
{
    int i = 0;

    for (i = 0; uds_ars_types[i].strptr; i++) {
        if (uds_ars_types[i].value == value) {
            return uds_ars_types[i].strptr;
        }
    }

    return "Unknown";
}

static const value_string uds_ars_auth_ret_types[] = {
    {UDS_ARS_AUTH_RET_REQUEST_ACCEPTED,         "Request Accepted"},
    {UDS_ARS_AUTH_RET_GENERAL_REJECT,           "General Reject"},
    {UDS_ARS_AUTH_RET_AUTH_CONFIG_APCE,         "Authentication Configuration APCE"},
    {UDS_ARS_AUTH_RET_AUTH_CONFIG_ACR_SYM,      "Authentication Configuration ACR with asymmetric cryptography"},
    {UDS_ARS_AUTH_RET_AUTH_CONFIG_ACR_ASYM,     "Authentication Configuration ACR with symmetric cryptography"},
    {UDS_ARS_AUTH_RET_DEAUTH_SUCCESS,           "DeAuthentication successful "},
    {UDS_ARS_AUTH_RET_CERT_VER_OWN_VER_NEC,     "Certificate Verified, Ownership Verification Necessary"},
    {UDS_ARS_AUTH_RET_OWN_VER_AUTH_COMPL,       "Ownership Verified, Authentication Complete "},
    {UDS_ARS_AUTH_RET_CERT_VERIFIED,            "Certificate Verified"},
    {0, NULL}
};
const char *dm_desc_uds_ars_auth_ret_types(INT32U value)
{
    int i = 0;

    for (i = 0; uds_ars_auth_ret_types[i].strptr; i++) {
        if (uds_ars_auth_ret_types[i].value == value) {
            return uds_ars_auth_ret_types[i].strptr;
        }
    }

    return "Unknown";
}

/* RDBPI */
static const value_string uds_rdbpi_transmission_mode[] = {
    {0, "Reserved"},
    {1, "Send at Slow Rate"},
    {2, "Send at Medium Rate"},
    {3, "Send at Fast Rate"},
    {4, "Stop Sending"},
    {0, NULL}
};
const char *dm_desc_uds_rdbpi_transmission_mode(INT32U value)
{
    int i = 0;

    for (i = 0; uds_rdbpi_transmission_mode[i].strptr; i++) {
        if (uds_rdbpi_transmission_mode[i].value == value) {
            return uds_rdbpi_transmission_mode[i].strptr;
        }
    }

    return "Unknown";
}
    

/* DDDI */
static const value_string uds_dddi_types[] = {
    {UDS_DDDI_TYPES_DEFINE_BY_IDENTIFIER,   "Define by Identifier"},
    {UDS_DDDI_TYPES_DEFINE_BY_MEM_ADDRESS,  "Define by Memory Address"},
    {UDS_DDDI_TYPES_CLEAR_DYN_DEF_DATA_ID,  "Clear Dynamically Defined Data Identifier"},
    {0, NULL}
};
const char *dm_desc_uds_dddi_types(INT32U value)
{
    int i = 0;

    for (i = 0; uds_dddi_types[i].strptr; i++) {
        if (uds_dddi_types[i].value == value) {
            return uds_dddi_types[i].strptr;
        }
    }

    return "Unknown";
}

/* IOCBI */
static const value_string uds_iocbi_parameters[] = {
    {UDS_IOCBI_PARAMETERS_RETURN_CONTROL_TO_ECU, "Return Control To ECU"},
    {UDS_IOCBI_PARAMETERS_RESET_TO_DEFAULT,      "Reset To Default"},
    {UDS_IOCBI_PARAMETERS_FREEZE_CURRENT_STATE,  "Freeze Current State"},
    {UDS_IOCBI_PARAMETERS_SHORT_TERM_ADJUSTMENT, "Short Term Adjustment"},
    {0, NULL}
};
const char *dm_desc_uds_iocbi_parameters(INT32U value)
{
    int i = 0;

    for (i = 0; uds_iocbi_parameters[i].strptr; i++) {
        if (uds_iocbi_parameters[i].value == value) {
            return uds_iocbi_parameters[i].strptr;
        }
    }

    return "Unknown";
}

/* RC */
static const value_string uds_rc_types[] = {
    {0,                    "Reserved"},
    {UDS_RC_TYPES_START,   "Start routine"},
    {UDS_RC_TYPES_STOP,    "Stop routine"},
    {UDS_RC_TYPES_REQUEST, "Request routine result"},
    {0, NULL}
};
const char *dm_desc_uds_rc_types(INT32U value)
{
    int i = 0;

    for (i = 0; uds_rc_types[i].strptr; i++) {
        if (uds_rc_types[i].value == value) {
            return uds_rc_types[i].strptr;
        }
    }

    return "Unknown";
}

/* RFT */
static const value_string uds_rft_mode_types[] = {
    {0,                             "Reserved"},
    {UDS_RFT_MODE_ADD_FILE,         "Add File"},
    {UDS_RFT_MODE_DELETE_FILE,      "Delete File"},
    {UDS_RFT_MODE_REPLACE_FILE,     "Replace File"},
    {UDS_RFT_MODE_READ_FILE,        "Read File"},
    {UDS_RFT_MODE_READ_DIR,         "Read Dir"},
    {UDS_RFT_MODE_RESUME_FILE,      "Resume File"},
    {0, NULL}
};
const char *dm_desc_uds_rft_mode_types(INT32U value)
{
    int i = 0;

    for (i = 0; uds_rft_mode_types[i].strptr; i++) {
        if (uds_rft_mode_types[i].value == value) {
            return uds_rft_mode_types[i].strptr;
        }
    }

    return "Unknown";
}

/* CDTCS */
static const value_string uds_cdtcs_types[] = {
    {0,                     "Reserved"},
    {UDS_CDTCS_ACTIONS_ON,  "On"},
    {UDS_CDTCS_ACTIONS_OFF, "Off"},
    {0, NULL}
};
const char *dm_desc_uds_cdtcs_types(INT32U value)
{
    int i = 0;

    for (i = 0; uds_cdtcs_types[i].strptr; i++) {
        if (uds_cdtcs_types[i].value == value) {
            return uds_cdtcs_types[i].strptr;
        }
    }

    return "Unknown";
}

/* LC */
static const value_string uds_lc_types[] = {
    {0x00,                  "Reserved"},
    {UDS_LC_TYPES_VMTWFP,   "Verify Mode Transition with fixed Parameter"},
    {UDS_LC_TYPES_VMTWSP,   "Verify Mode Transition with specific Parameter"},
    {UDS_LC_TYPES_TM,       "Transition Mode"},
    {0, NULL}
};
const char *dm_desc_uds_lc_types(INT32U value)
{
    int i = 0;

    for (i = 0; uds_lc_types[i].strptr; i++) {
        if (uds_lc_types[i].value == value) {
            return uds_lc_types[i].strptr;
        }
    }

    return "Unknown";
}

/* DIDS */
static const value_string uds_standard_did_types[] = {
    {UDS_DID_BSIDID,        "BootSoftwareIdentificationDataIdentifier"},
    {UDS_DID_ASIDID,        "applicationSoftwareIdentificationDataIdentifier"},
    {UDS_DID_ADIDID,        "applicationDataIdentificationDataIdentifier"},
    {UDS_DID_BSFPDID,       "bootSoftwareFingerprintDataIdentifier"},
    {UDS_DID_ASFPDID,       "applicationSoftwareFingerprintDataIdentifier"},
    {UDS_DID_ADFPDID,       "applicationDataFingerprintDataIdentifier"},
    {UDS_DID_ADSDID,        "ActiveDiagnosticSessionDataIdentifier"},
    {UDS_DID_VMSPNDID,      "vehicleManufacturerSparePartNumberDataIdentifier"},
    {UDS_DID_VMECUSNDID,    "vehicleManufacturerECUSoftwareNumberDataIdentifier"},
    {UDS_DID_VMECUSVNDID,   "vehicleManufacturerECUSoftwareVersionNumberDataIdentifier"},
    {UDS_DID_SSIDDID,       "systemSupplierIdentifierDataIdentifier"},
    {UDS_DID_ECUMDDID,      "ECUManufacturingDateDataIdentifier (year/month/day)"},
    {UDS_DID_ECUSNDID,      "ECUSerialNumberDataIdentifier"},
    {UDS_DID_SFUDID,        "supportedFunctionalUnitsDataIdentifier"},
    {UDS_DID_VMKAPNDID,     "VehicleManufacturerKitAssemblyPartNumberDataIdentifier"},
    {UDS_DID_RXSWIN,        "RegulationXSoftwareIdentificationNumbers (RxSWIN)"},
    {UDS_DID_VINDID,        "VINDataIdentifier"},
    {UDS_DID_VMECUHNDID,    "vehicleManufacturerECUHardwareNumberDataIdentifier"},
    {UDS_DID_SSECUHWNDID,   "systemSupplierECUHardwareNumberDataIdentifier"},
    {UDS_DID_SSECUHWVNDID,  "systemSupplierECUHardwareVersionNumberDataIdentifier"},
    {UDS_DID_SSECUSWNDID,   "systemSupplierECUSoftwareNumberDataIdentifier"},
    {UDS_DID_SSECUSWVNDID,  "systemSupplierECUSoftwareVersionNumberDataIdentifier"},
    {UDS_DID_EROTANDID,     "exhaustRegulationOrTypeApprovalNumberDataIdentifier"},
    {UDS_DID_SNOETDID,      "systemNameOrEngineTypeDataIdentifier"},
    {UDS_DID_RSCOTSNDID,    "repairShopCodeOrTesterSerialNumberDataIdentifier"},
    {UDS_DID_PDDID,         "programmingDateDataIdentifier (year/month/day)"},
    {UDS_DID_CRSCOCESNDID,  "calibrationRepairShopCodeOrCalibrationEquipmentSerialNumberDataIdentifier"},
    {UDS_DID_CDDID,         "calibrationDateDataIdentifier (year/month/day)"},
    {UDS_DID_CESWNDID,      "calibrationEquipmentSoftwareNumberDataIdentifier"},
    {UDS_DID_EIDDID,        "ECUInstallationDateDataIdentifier (year/month/day)"},
    {UDS_DID_ODXFDID,       "ODXFileDataIdentifier"},
    {UDS_DID_EDID,          "EntityDataIdentifier"},
    {UDS_DID_ADDID_FA00,    "AirbagDeployment: Number of PCUs (ISO 26021-2)"},
    {UDS_DID_ADDID_FA01,    "AirbagDeployment: Deployment Method Version (ISO 26021-2)"},
    {UDS_DID_ADDID_FA02,    "AirbagDeployment: Address Information of PCU (ISO 26021-2)"},
    {UDS_DID_ADDID_FA03,    "AirbagDeployment (ISO 26021-2)"},
    {UDS_DID_ADDID_FA04,    "AirbagDeployment (ISO 26021-2)"},
    {UDS_DID_ADDID_FA05,    "AirbagDeployment (ISO 26021-2)"},
    {UDS_DID_ADDID_FA06,    "AirbagDeployment: Deployment Loop Table of PCU (ISO 26021-2)"},
    {UDS_DID_ADDID_FA07,    "AirbagDeployment: Dismantler Info (ISO 26021-2)"},
    {UDS_DID_ADDID_FA08,    "AirbagDeployment (ISO 26021-2)"},
    {UDS_DID_ADDID_FA09,    "AirbagDeployment (ISO 26021-2)"},
    {UDS_DID_ADDID_FA0A,    "AirbagDeployment (ISO 26021-2)"},
    {UDS_DID_ADDID_FA0B,    "AirbagDeployment (ISO 26021-2)"},
    {UDS_DID_ADDID_FA0C,    "AirbagDeployment (ISO 26021-2)"},
    {UDS_DID_ADDID_FA0D,    "AirbagDeployment (ISO 26021-2)"},
    {UDS_DID_ADDID_FA0E,    "AirbagDeployment (ISO 26021-2)"},
    {UDS_DID_ADDID_FA0F,    "AirbagDeployment (ISO 26021-2)"},
    {UDS_DID_NOEDRD,        "NumberOfEDRDevices"},
    {UDS_DID_EDRI,          "EDRIdentification"},
    {UDS_DID_EDRDAI,        "EDRDeviceAddressInformation"},
    {UDS_DID_UDSVDID,       "UDSVersionDataIdentifier"},
    {UDS_DID_RESRVDCPADLC,  "ReservedForISO15765-5 (CAN, CAN-FD, CAN+CAN-FD, ...)"},
    {0, NULL}
};
const char *dm_desc_uds_standard_did_types(INT32U value)
{
    int i = 0;

    for (i = 0; uds_standard_did_types[i].strptr; i++) {
        if (uds_standard_did_types[i].value == value) {
            return uds_standard_did_types[i].strptr;
        }
    }

    return "Unknown";
}

/* ReservedForISO15765 */
static const value_string uds_did_resrvdcpadlc_types[] = {
    {0, "CAN Classic Only"},
    {1, "CAN FD only"},
    {2, "CAN Classic and CAN FD"},
    {0, NULL}
};
const char *dm_desc_uds_did_resrvdcpadlc_types(INT32U value)
{
    int i = 0;

    for (i = 0; uds_did_resrvdcpadlc_types[i].strptr; i++) {
        if (uds_did_resrvdcpadlc_types[i].value == value) {
            return uds_did_resrvdcpadlc_types[i].strptr;
        }
    }

    return "Unknown";
}

/* RIDS */
static const value_string uds_standard_rid_types[] = {
    {UDS_RID_EXSPLRI_,      "Execute SPL"},
    {UDS_RID_DLRI_,         "DeployLoopRoutineID"},
    {UDS_RID_EM_,           "eraseMemory"},
    {UDS_RID_CPD_,          "checkProgrammingDependencies"},
    {UDS_RID_FF02,          "eraseMirrorMemoryDTCs (deprecated)"},
    {0, NULL}
};
const char *dm_desc_uds_standard_rid_types(INT32U value)
{
    int i = 0;

    for (i = 0; uds_standard_rid_types[i].strptr; i++) {
        if (uds_standard_rid_types[i].value == value) {
            return uds_standard_rid_types[i].strptr;
        }
    }

    return "Unknown";
}
#else /* __HAVE_UDS_DESC_INFO__ */
const char *dm_desc_uds_services(INT32U      value)
{
    return "NULL";
}

const char *dm_desc_uds_response_codes(INT32U        value)
{
    return "NULL";
}

const char *dm_desc_uds_dsc_types(INT32U       value)
{
    return "NULL";
}

const char *dm_desc_uds_er_types(INT32U       value)
{
    return "NULL";
}

const char *dm_desc_uds_cdtci_group_of_dtc(INT32U value)
{
    return "NULL";
}

const char *dm_desc_uds_rdtci_types(INT32U value)
{
    return "NULL";
}

const char *dm_desc_uds_rdtci_format_id_types(INT32U value)
{
    return "NULL";
}

const char *dm_desc_uds_rsdbi_data_types(INT32U value)
{
    return "NULL";
}

const char *dm_desc_uds_cc_types(INT32U value)
{
    return "NULL";
}

const char *dm_desc_uds_cc_comm_types(INT32U value)
{
    return "NULL";
}

const char *dm_desc_uds_cc_subnet_number_types(INT32U value)
{
    return "NULL";
}

const char *dm_desc_uds_ars_types(INT32U value)
{
    return "NULL";
}

const char *dm_desc_uds_ars_auth_ret_types(INT32U value)
{
    return "NULL";
}

const char *dm_desc_uds_rdbpi_transmission_mode(INT32U value)
{
    return "NULL";
}

const char *dm_desc_uds_dddi_types(INT32U value)
{
    return "NULL";
}

const char *dm_desc_uds_iocbi_parameters(INT32U value)
{
    return "NULL";
}

const char *dm_desc_uds_rc_types(INT32U value)
{
    return "NULL";
}

const char *dm_desc_uds_rft_mode_types(INT32U value)
{
    return "NULL";
}

const char *dm_desc_uds_cdtcs_types(INT32U value)
{
    return "NULL";
}

const char *dm_desc_uds_lc_types(INT32U value)
{
    return "NULL";
}

const char *dm_desc_uds_standard_did_types(INT32U value)
{
    return "NULL";
}

const char *dm_desc_uds_did_resrvdcpadlc_types(INT32U value)
{
    return "NULL";
}

const char *dm_desc_uds_standard_rid_types(INT32U value)
{
    return "NULL";
}

#endif /* __HAVE_UDS_DESC_INFO__ */

#if 0
static int mfcnt = 0;
void* udsc_malloc(yuint32 size)
{
    void *p = malloc(size);

    mfcnt += (yuint32)p % 1000 + (yuint32)p % 1001;
    udsc_printf("Malloc adderss:(%x) total:(%d) \n", (int)p, mfcnt);    
    return p;
}
void udsc_free(void* p)
{
    if (p) {
        mfcnt -= (yuint32)p % 1000 + (yuint32)p % 1001;
        udsc_printf("Free adderss:(%x) total:(%d) \n", (int)p, mfcnt);    
        free(p);
    }
}

void* udsc_calloc(yuint32 size, yuint32 cnt)
{
    void *p = calloc(size, cnt);

    mfcnt += (yuint32)p % 1000 + (yuint32)p % 1001;
    udsc_printf("Calloc adderss:(%x) total:(%d) \n", (int)p, mfcnt);    
    return p;
}
#endif
