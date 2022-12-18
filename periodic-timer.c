/**
 * @file periodic_timer.c
 * @author liuqi (liuqi1@kylinos.cn)
 * @brief 
 * @version 0.1
 * @date 2022-12-14
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "kyrg.h"

#include <linux/kernel.h>
#include <linux/timer.h>

static void timer_handler(struct timer_list *t);

DEFINE_TIMER(timer, timer_handler);
static unsigned long time_msecs_period; // ms
static void (*callback)(void);

static void timer_handler(struct timer_list *t)
{
	pr_debug("timer handler start.\n");
	/*Restarting the timer...*/
    mod_timer(t, jiffies + msecs_to_jiffies(time_msecs_period));

	if(callback)
		callback();

	pr_debug("timer handler end.\n");
}

void init_periodic_timer(void (*cb)(void), unsigned long msecs_period)
{
	callback = cb;
	time_msecs_period = msecs_period;
	timer.expires = jiffies + msecs_to_jiffies(time_msecs_period);

	// start timer
	add_timer(&timer);

	pr_info("Periodic timer init success! period: %ld ms", time_msecs_period);
}

void exit_periodic_timer(void)
{
	del_timer(&timer);
	pr_info("Periodic timer exit.");
}
