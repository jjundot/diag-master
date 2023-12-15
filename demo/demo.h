#ifndef __DEMO_H__
#define __DEMO_H__

#ifdef _WIN32 
#define demo_printf(fmt, ...) do {\
            printf(fmt, ## __VA_ARGS__); \
        } while (0)
#else 
#define demo_printf(fmt, ...) do {\
            struct timeb tTimeB; \
            ftime(&tTimeB); \
            struct tm* tTM = localtime(&tTimeB.time); \
            printf("[%04d/%02d/%02d %02d:%02d:%02d.%03d] ", tTM->tm_year + 1900, \
            tTM->tm_mon + 1, tTM->tm_mday, tTM->tm_hour, tTM->tm_min, tTM->tm_sec, \
            tTimeB.millitm); \
            printf("[%s:%d]", __FILE__, __LINE__); \
            printf(fmt, ## __VA_ARGS__); \
        } while (0)
#endif
#define demo_hex_printf(describe, msg, msglen) do { \
        if ((msglen) > 10240) { \
            demo_printf("Messages that are too long are not dumped. \n"); \
            break; \
        } \
        int nn = 0, pp = 0; \
        char hexstr[1024] = {0}; \
        for (; nn < (msglen); nn++, pp++) { \
            if (pp >= 300) {\
                demo_printf("%s(%d-%d/%d):%s \n", describe, nn - pp + 1, nn, (msglen), hexstr);\
                pp = 0;\
                memset(hexstr, 0, sizeof(hexstr)); \
            }\
            snprintf(hexstr + (pp * 3), sizeof(hexstr) - (pp * 3), "%02X ", (unsigned char)msg[nn]); \
        } \
        demo_printf("%s(%d-%d/%d):%s \n", describe, nn - pp + 1, nn, (msglen), hexstr);\
    } while(0)
   
            
#endif /* __DEMO_H__ */
