#include "unity.h"
#include "hc_sr04.h"
#include "delay.h"
#include "mock_mik32_hal_gpio.h"

void setUp(void) {}
void tearDown(void) {}

TEST_CASE(0, 0)
TEST_CASE(1, 100)
TEST_CASE(7, 410)
TEST_CASE(17150, 1000000)
void test_cm_distance_by_delay_should_be_correct(int result, int duration_us) {

    //Act
    int distance = convert_us_to_distance(duration_us);

    // Assert
    TEST_ASSERT_EQUAL_INT(result, distance);
}
