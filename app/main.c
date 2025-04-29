#include "mik32_hal_usart.h"
#include "mik32_hal_i2c.h"
#include "lcd_driver.h"
#include "mik32_hal_gpio.h"
#include "delay.h"
#include "util.h"

#include "epic.h"
#include "gpio.h"
#include "gpio_irq.h"
#include "mik32_hal_irq.h"
#include "mik32_memory_map.h"
#include "pad_config.h"
#include "power_manager.h"
#include "scr1_timer.h"
#include "timer32.h"
#include "wakeup.h"
#include "xprintf.h"

#include "hc_sr04.h"

#define SYSTEM_FREQ_HZ (32000000UL)

#define SERVO_PIN_NUM (1) //D5

#define ECHO_PIN_NUM ( GPIO_PIN_10) //D2
#define TRIGGER_PIN_NUM ( GPIO_PIN_0) //D3

#define PWM_FREQ_HZ (50)
#define PWM_PERIOD_TICKS (SYSTEM_FREQ_HZ / PWM_FREQ_HZ)
#define TICKS_PER_MS (SYSTEM_FREQ_HZ / 1000U)
#define UPDATE_LCD_MS (500)

#define MIN_ANGLE_PER_SEC (1)
#define MAX_ANGLE_PER_SEC (90)
#define ONE_ANGLE_ROTATE_MS (5)

static void SystemClock_Config();
static void GPIO_Init();
static void USART_Init();
static void I2C_Init();
void TMR_PWM_Init();
static void configure_interrupts();
void trap_handler();

extern void update_servo_angle(int angle);
extern int get_delay_ms_by_angle_per_second(int angle_per_second);


USART_HandleTypeDef husart0;
I2C_HandleTypeDef hi2c;

static unsigned char angle_per_second = 10;
static int delay_update_servo;


int main()
{
    SystemClock_Config();
    GPIO_Init();
    USART_Init();
    I2C_Init();
    TMR_PWM_Init();

    configure_interrupts();

    HAL_GPIO_WritePin(GPIO_0, TRIGGER_PIN_NUM, GPIO_PIN_HIGH);
    
    HC_SR04 hc_sr04;

    lcd_init();
    lcd_clear();

    delay_update_servo = get_delay_ms_by_angle_per_second(angle_per_second); 

    HC_SR04_init(&hc_sr04, ECHO_PIN_NUM, TRIGGER_PIN_NUM);
    int current_angle = 0;
    int step = 1;

    int timeout_update_lcd = 0;
    while(1) {
        HAL_DelayMs(delay_update_servo);
        update_servo_angle(current_angle);
        int value = get_distance_sm(&hc_sr04);
        if (timeout_update_lcd > UPDATE_LCD_MS) {
            lcd_clear();
            lcd_send_string("Angle = ", 0, 0);
            lcd_send_int(current_angle, 0, 8);
            lcd_send_string("Dist [cm] = ", 1, 0);
            lcd_send_int(value > 0 ? value : MAX_DISTANCE_CM, 1, 12);
            timeout_update_lcd = 0;
        }
        timeout_update_lcd += delay_update_servo;
        xprintf("%d %d\r\n", current_angle, value);
        current_angle += step;
        if (current_angle > 180 || current_angle < 0) {
            step *= -1;
            current_angle += step;
        }
    }    
}

void SystemClock_Config(void)
{
    WU->CLOCKS_SYS &=
        ~(0b11 << WU_CLOCKS_SYS_OSC32M_EN_S); // Включить OSC32M и HSI32M
    WU->CLOCKS_BU &=
        ~(0b11 << WU_CLOCKS_BU_OSC32K_EN_S); // Включить OSC32K и LSI32K

    // Поправочный коэффициент HSI32M
    WU->CLOCKS_SYS = (WU->CLOCKS_SYS & (~WU_CLOCKS_SYS_ADJ_HSI32M_M)) |
                        WU_CLOCKS_SYS_ADJ_HSI32M(128);
    // Поправочный коэффициент LSI32K
    WU->CLOCKS_BU = (WU->CLOCKS_BU & (~WU_CLOCKS_BU_ADJ_LSI32K_M)) |
                    WU_CLOCKS_BU_ADJ_LSI32K(8);

    // Автоматический выбор источника опорного тактирования
    WU->CLOCKS_SYS &= ~WU_CLOCKS_SYS_FORCE_32K_CLK_M;

    // ожидание готовности
    while (!(PM->FREQ_STATUS & PM_FREQ_STATUS_OSC32M_M))
        ;

    // переключение на тактирование от OSC32M
    PM->AHB_CLK_MUX = PM_AHB_CLK_MUX_OSC32M_M | PM_AHB_FORCE_MUX_UNFIXED;
    PM->DIV_AHB = 0;   // Задать делитель шины AHB.
    PM->DIV_APB_M = 0; // Задать делитель шины APB_M.


    PCC_InitTypeDef PCC_OscInit = {0};

    PCC_OscInit.OscillatorEnable = PCC_OSCILLATORTYPE_ALL;
    PCC_OscInit.FreqMon.OscillatorSystem = PCC_OSCILLATORTYPE_OSC32M;
    PCC_OscInit.FreqMon.ForceOscSys = PCC_FORCE_OSC_SYS_UNFIXED;
    PCC_OscInit.FreqMon.Force32KClk = PCC_FREQ_MONITOR_SOURCE_OSC32K;
    PCC_OscInit.AHBDivider = 0;
    PCC_OscInit.APBMDivider = 0;
    PCC_OscInit.APBPDivider = 0;
    PCC_OscInit.HSI32MCalibrationValue = 128;
    PCC_OscInit.LSI32KCalibrationValue = 8;
    PCC_OscInit.RTCClockSelection = PCC_RTC_CLOCK_SOURCE_AUTO;
    PCC_OscInit.RTCClockCPUSelection = PCC_CPU_RTC_CLOCK_SOURCE_OSC32K;
    HAL_PCC_Config(&PCC_OscInit);
}

void USART_Init()
{
    husart0.Instance = UART_0;
    husart0.transmitting = Enable;
    husart0.receiving = Enable;
    husart0.frame = Frame_8bit;
    husart0.parity_bit = Disable;
    husart0.parity_bit_inversion = Disable;
    husart0.bit_direction = LSB_First;
    husart0.data_inversion = Disable;
    husart0.tx_inversion = Disable;
    husart0.rx_inversion = Disable;
    husart0.swap = Disable;
    husart0.lbm = Disable;
    husart0.stop_bit = StopBit_1;
    husart0.mode = Asynchronous_Mode;
    husart0.xck_mode = XCK_Mode3;
    husart0.last_byte_clock = Disable;
    husart0.overwrite = Disable;
    husart0.rts_mode = AlwaysEnable_mode;
    husart0.dma_tx_request = Disable;
    husart0.dma_rx_request = Disable;
    husart0.channel_mode = Duplex_Mode;
    husart0.tx_break_mode = Disable;
    husart0.Interrupt.ctsie = Disable;
    husart0.Interrupt.eie = Disable;
    husart0.Interrupt.idleie = Disable;
    husart0.Interrupt.lbdie = Disable;
    husart0.Interrupt.peie = Disable;
    husart0.Interrupt.rxneie = Disable;
    husart0.Interrupt.tcie = Disable;
    husart0.Interrupt.txeie = Disable;
    husart0.Modem.rts = Disable; //out
    husart0.Modem.cts = Disable; //in
    husart0.Modem.dtr = Disable; //out
    husart0.Modem.dcd = Disable; //in
    husart0.Modem.dsr = Disable; //in
    husart0.Modem.ri = Disable;  //in
    husart0.Modem.ddis = Disable;//out
    husart0.baudrate = 115200;
    HAL_USART_Init(&husart0);
}

void I2C_Init() {
    hi2c.Instance = I2C_1;
    hi2c.Init.Mode = HAL_I2C_MODE_MASTER;

    hi2c.Init.DigitalFilter = I2C_DIGITALFILTER_OFF;
    hi2c.Init.AnalogFilter = I2C_ANALOGFILTER_DISABLE;
    hi2c.Init.AutoEnd = I2C_AUTOEND_ENABLE;

    /* Настройка частоты */
    hi2c.Clock.PRESC = 5;
    hi2c.Clock.SCLDEL = 15;
    hi2c.Clock.SDADEL = 15;
    hi2c.Clock.SCLH = 15;
    hi2c.Clock.SCLL = 15;

    if (HAL_I2C_Init(&hi2c) == HAL_OK)
    {
        HAL_USART_Print(&husart0, "I2C init OK\r\n", USART_TIMEOUT_DEFAULT);
    }
}

void GPIO_Init() {
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    __HAL_PCC_GPIO_0_CLK_ENABLE();

    GPIO_InitStruct.Pin = ECHO_PIN_NUM; //D2
    GPIO_InitStruct.Pull = HAL_GPIO_PULL_DOWN;
    GPIO_InitStruct.Mode = HAL_GPIO_MODE_GPIO_INPUT;
    HAL_GPIO_Init(GPIO_0, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = TRIGGER_PIN_NUM; //D3
    GPIO_InitStruct.Pull = HAL_GPIO_PULL_NONE;
    GPIO_InitStruct.Mode = HAL_GPIO_MODE_GPIO_OUTPUT;
    HAL_GPIO_Init(GPIO_0, &GPIO_InitStruct);

}

void TMR_PWM_Init() {
    // Подключене пинов к TIMER1_0_CH1
    PAD_CONFIG->PORT_0_CFG |= (2 << (SERVO_PIN_NUM * 2));
    // Включение тактирования TIMER32_1
    PM->CLK_APB_P_SET = PM_CLOCK_APB_P_TIMER32_1_M;
    TIMER32_1->ENABLE = 0;
    TIMER32_1->TOP = PWM_PERIOD_TICKS;
    TIMER32_1->PRESCALER = 0;
    TIMER32_1->CONTROL =
        TIMER32_CONTROL_MODE_UP_M | TIMER32_CONTROL_CLOCK_PRESCALER_M;
    TIMER32_1->INT_MASK = 0;
    TIMER32_1->INT_CLEAR = 0xFFFFFFFF;
  
    TIMER32_1->CHANNELS[1].CNTRL =
        TIMER32_CH_CNTRL_MODE_PWM_M | TIMER32_CH_CNTRL_ENABLE_M;
  
    TIMER32_1->CHANNELS[2].CNTRL =
        TIMER32_CH_CNTRL_MODE_PWM_M | TIMER32_CH_CNTRL_ENABLE_M;    
    TIMER32_1->ENABLE = 1;
}

void configure_interrupts() {
    __HAL_PCC_EPIC_CLK_ENABLE();
    HAL_EPIC_MaskLevelSet(HAL_EPIC_UART_0_MASK);
    HAL_USART_RXNE_EnableInterrupt(&husart0);
    HAL_IRQ_EnableInterrupts();
}

void trap_handler()
{
    if(EPIC_CHECK_UART_0()) {
        if (HAL_USART_RXNE_ReadFlag(&husart0)) {
            HAL_USART_RXNE_ClearFlag(&husart0);
            angle_per_second = HAL_USART_ReadByte(&husart0);
            if (angle_per_second >= MIN_ANGLE_PER_SEC && angle_per_second <= MAX_ANGLE_PER_SEC) {
                delay_update_servo = get_delay_ms_by_angle_per_second(angle_per_second); 
            }
        }
        HAL_USART_ClearFlags(&husart0);
    }
    HAL_EPIC_Clear(0xFFFFFFFF);
}


