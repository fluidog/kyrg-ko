/**
 * @file kyrg-core.c
 * @author liuqi (liuqi1@kylinos.cn)
 * @brief
 * @version 0.1
 * @date 2022-12-22
 *
 * @copyright Copyright (c) 2022
 *
 */
// #define DEBUG
#include "kyrg.h"

#include <linux/kernel.h>

static enum RG_STATUS rg_status = RG_STATUS_RUNNING;

static unsigned long long rg_periodic_sec = 3; // seconds

int do_rg(void)
{
    int rc;

    if (rg_status == RG_STATUS_STOP)
        return 0;

    rc = do_rg_kernel();
    if (rc < 0)
        return rc;

    rc = do_rg_modules();
    if (rc < 0)
        return rc;

    rc = do_rg_processes();
    if (rc < 0)
        return rc;

    return 0;
}

static int start_rg(void)
{
    if (rg_periodic_sec == 0) // not periodic
        return 0;

    init_periodic_timer(do_rg, rg_periodic_sec * 1000);

    return 0;
}

static void stop_rg(void)
{
    exit_periodic_timer();
}

int get_rg_status(void)
{
    return rg_status;
}

int set_rg_status(int status)
{
    if (status != RG_STATUS_STOP && status != RG_STATUS_RUNNING)
        return -EINVAL;

    if (rg_status == RG_STATUS_STOP && status == RG_STATUS_RUNNING) {
        int error;
        error = start_rg();
        if (!error)
            rg_status = RG_STATUS_RUNNING;
        return error;
    }

    if (rg_status == RG_STATUS_RUNNING && status == RG_STATUS_STOP) {
        stop_rg();
        rg_status = RG_STATUS_STOP;
        return 0;
    }

    // rg_status == status

    return 0;
}

unsigned long long get_rg_periodic(void)
{
    return rg_periodic_sec;
}

int set_rg_periodic(unsigned long long period_sec)
{
    rg_periodic_sec = period_sec;

    if (rg_status != RG_STATUS_RUNNING)
        return 0;

    stop_rg();
    return start_rg();
}

int init_rg(void)
{
    int rc;

    rc = init_rg_kernel();
    if (rc < 0)
        return rc;

    rc = init_rg_modules();
    if (rc < 0) {
        exit_rg_kernel();
        return rc;
    }

    rc = init_rg_processes();
    if (rc < 0) {
        exit_rg_kernel();
        exit_rg_modules();
        return rc;
    }

    if (rg_status == RG_STATUS_RUNNING)
        return start_rg();

    pr_info("Kylin Runtime Guard init success!\n");

    return 0;
}

void exit_rg(void)
{
    if (rg_status == RG_STATUS_RUNNING)
        stop_rg();

    exit_rg_processes();
    exit_rg_modules();
    exit_rg_kernel();

    pr_info("Kylin Runtime Guard exit.\n");
}
