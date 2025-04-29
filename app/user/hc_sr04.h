#ifndef hc_sr04_h
#define hc_sr04_h

#define MIN_ANGLE_PER_SEC (1)
#define MAX_ANGLE_PER_SEC (90)
#define ONE_ANGLE_ROTATE_MS (5)

#define TIMEOUT_DURATION (100) // Таймаут в миллисекундах
#define SOUND_SPEED_CM_PER_US (343) // Скорость звука в м/с
#define MAX_DISTANCE_CM (400)   // Максимальное измеряемое расстояние в см

#define TIMEOUT_ERROR -1
#define MAX_DISTANCE -2


typedef struct
{
  int _echo_pin;
  int _trig_pin;
} HC_SR04;


void HC_SR04_init(HC_SR04 *hc_sr04, int echo_pin, int trig_pin);

int get_distance_sm(HC_SR04 *hc_sr04);

int convert_us_to_distance(int impulse_us);

#endif
