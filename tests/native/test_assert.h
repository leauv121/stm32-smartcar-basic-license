#ifndef FINAL_WORK_TEST_ASSERT_H
#define FINAL_WORK_TEST_ASSERT_H

#include <stdio.h>

static int test_failures = 0;

#define TEST_ASSERT_TRUE(condition)                                             \
    do                                                                         \
    {                                                                          \
        if (!(condition))                                                      \
        {                                                                      \
            fprintf(stderr, "断言失败：%s，第 %d 行\n", #condition, __LINE__); \
            test_failures++;                                                   \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_EQ_INT(expected, actual)                                   \
    do                                                                         \
    {                                                                          \
        int expected_value = (int)(expected);                                  \
        int actual_value = (int)(actual);                                      \
        if (expected_value != actual_value)                                    \
        {                                                                      \
            fprintf(stderr,                                                    \
                    "断言失败：期望 %d，实际 %d，第 %d 行\n",                \
                    expected_value,                                            \
                    actual_value,                                              \
                    __LINE__);                                                 \
            test_failures++;                                                   \
        }                                                                      \
    } while (0)

#endif
