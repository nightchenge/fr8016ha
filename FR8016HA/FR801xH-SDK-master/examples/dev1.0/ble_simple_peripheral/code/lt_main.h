#ifndef _LT_MAIN_H_
#define _LT_MAIN_H_

void main_app_init(void);


void main_app_stop_timers(void);

/**
 * @brief 重新启动应用层定时器（用于从睡眠中唤醒）
 */
void main_app_restart_timers(void);

#endif