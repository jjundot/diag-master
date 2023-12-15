#ifdef _WIN32
#include <windows.h>
#else /* #ifdef _WIN32 */

#endif /* #ifdef _WIN32 */
int main(int argc, char *argv[])
{
    diag_master_dms_start();

    while (1) {
#ifdef _WIN32
        Sleep(1000);
#else /* #ifdef _WIN32 */
        sleep(1000);
#endif /* #ifdef _WIN32 */
    }
    
    return 0;
}
