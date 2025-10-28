#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    BATTERY_STATE_CRITICAL,
    BATTERY_STATE_LOW,
    BATTERY_STATE_MEDIUM,
    BATTERY_STATE_HIGH,
    BATTERY_STATE_CHARGING,
    BATTERY_STATE_FULLY_CHARGED,
} battery_state_t;

void power_manager_init(void);
void power_manager_check_on_boot(void);
void power_manager_prepare_for_sleep(void);
void power_manager_handle_wakeup(void);
void power_manager_periodic_task(void);
void power_manager_request_sleep(void);
void power_manager_shutdown(void);
void power_manager_wakeup_system(char reason);
battery_state_t power_manager_get_state(void);

#endif // POWER_MANAGER_H