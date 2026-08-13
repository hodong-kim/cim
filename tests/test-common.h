#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdbool.h>

int  test_set_plugin_env         (const char *target);
int  test_set_verbose_env        (bool enabled);
void test_unset_plugin_env       (void);

#endif
