#include "app/route_detector.h"
#include "test_assert.h"

int main(void)
{
    RouteDetectorInput input = {0};

    RouteDetector_Init();
    input.candidate = ROUTE_POINT_B;
    input.candidate_valid = 1U;

    TEST_ASSERT_EQ_INT(ROUTE_EVENT_NONE, RouteDetector_Update(&input));
    TEST_ASSERT_EQ_INT(ROUTE_POINT_B, RouteDetector_Update(&input));
    TEST_ASSERT_EQ_INT(ROUTE_EVENT_NONE, RouteDetector_Update(&input));

    input.candidate_valid = 0U;
    TEST_ASSERT_EQ_INT(ROUTE_EVENT_NONE, RouteDetector_Update(&input));

    input.candidate = ROUTE_POINT_B;
    input.candidate_valid = 1U;
    TEST_ASSERT_EQ_INT(ROUTE_EVENT_NONE, RouteDetector_Update(&input));
    TEST_ASSERT_EQ_INT(ROUTE_POINT_B, RouteDetector_Update(&input));

    RouteDetector_Reset();
    input.candidate = ROUTE_POINT_D;
    TEST_ASSERT_EQ_INT(ROUTE_EVENT_NONE, RouteDetector_Update(&input));
    input.candidate = ROUTE_POINT_C;
    TEST_ASSERT_EQ_INT(ROUTE_EVENT_NONE, RouteDetector_Update(&input));

    return test_failures;
}
