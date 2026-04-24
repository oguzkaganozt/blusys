#include "sdkconfig.h"

void run_platform_smoke(void);
void run_platform_storage(void);
void run_platform_power_mgmt(void);
void run_platform_console(void);
void run_platform_framework_core(void);
void run_platform_concurrency(void);

void app_main(void)
{
#if CONFIG_PLATFORM_LAB_SCENARIO_SMOKE
    printf(">>> CONFIG_PLATFORM_LAB_SCENARIO_SMOKE is defined\n");
    fflush(stdout);
    run_platform_smoke();
#elif CONFIG_PLATFORM_LAB_SCENARIO_STORAGE
    run_platform_storage();
#elif CONFIG_PLATFORM_LAB_SCENARIO_POWER_MGMT
    run_platform_power_mgmt();
#elif CONFIG_PLATFORM_LAB_SCENARIO_CONSOLE
    run_platform_console();
#elif CONFIG_PLATFORM_LAB_SCENARIO_FRAMEWORK_CORE
    run_platform_framework_core();
#elif CONFIG_PLATFORM_LAB_SCENARIO_CONCURRENCY
    run_platform_concurrency();
#else
    printf(">>> NO SCENARIO DEFINED\n");
    fflush(stdout);
#endif
    printf(">>> app_main returning\n");
    fflush(stdout);
}
