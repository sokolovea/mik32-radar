#include "hc_sr04.h"
#include "delay.h"
#include "mik32_hal_gpio.h"


void HC_SR04_init(HC_SR04 *hc_sr04, int echo_pin, int trig_pin)
{
  hc_sr04->_echo_pin = echo_pin;
  hc_sr04->_trig_pin = trig_pin;
  GPIO_0->DIRECTION_IN |= echo_pin;
  GPIO_0->DIRECTION_OUT |= trig_pin;
  HAL_GPIO_WritePin(GPIO_0, trig_pin, GPIO_PIN_LOW); 
}


int get_distance_sm(HC_SR04 *hc_sr04)
{
  HAL_GPIO_WritePin(GPIO_0, hc_sr04->_trig_pin, GPIO_PIN_HIGH);
  delayMicroseconds(10);
  HAL_GPIO_WritePin(GPIO_0, hc_sr04->_trig_pin, GPIO_PIN_LOW);

  uint64_t timeout_start = SCR1_TIMER_GET_TIME();

  while (HAL_GPIO_ReadPin(GPIO_0, hc_sr04->_echo_pin) == GPIO_PIN_LOW)
  {
    if (SCR1_TIMER_GET_TIME() - timeout_start > (uint64_t)TIMEOUT_DURATION * CYCLES_PER_MILLISECOND)
    {
      return TIMEOUT_ERROR;
    }
  }

  uint64_t impulse_start = SCR1_TIMER_GET_TIME();

  while (HAL_GPIO_ReadPin(GPIO_0, hc_sr04->_echo_pin) == GPIO_PIN_HIGH)
  {
    if (SCR1_TIMER_GET_TIME() - impulse_start > (uint64_t)TIMEOUT_DURATION * CYCLES_PER_MILLISECOND)
    {
      return TIMEOUT_ERROR;
    }
  }

  uint32_t impulse_duration_us = (SCR1_TIMER_GET_TIME() - impulse_start) / CYCLES_PER_MICROSECOND;

  int distance_cm = convert_us_to_distance(impulse_duration_us);

  if (distance_cm < 1 || distance_cm > MAX_DISTANCE_CM) {
      return MAX_DISTANCE;
  }

  return distance_cm;
}

int convert_us_to_distance(int impulse_us) {
  return (int)(impulse_us * SOUND_SPEED_CM_PER_US / 2 / 10000U);
}

