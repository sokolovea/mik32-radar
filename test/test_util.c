#define TEST

#include "unity.h"
#include "delay.h"
#include "util.h"

void setUp(void) {}
void tearDown(void) {}

TEST_CASE(450, 0)
TEST_CASE(22, 20)
TEST_CASE(10, 45)
TEST_CASE(7, 60)
TEST_CASE(5, 90)
TEST_CASE(5, 180)
TEST_CASE(5, 360)
TEST_CASE(450, -30)
void test_get_delay_ms_by_angle_per_second_should_be_correct(int result, int angle_per_second) {

    //Act
    int delay = get_delay_ms_by_angle_per_second(angle_per_second);

    // Assert
    TEST_ASSERT_EQUAL_INT(result, delay);
}
