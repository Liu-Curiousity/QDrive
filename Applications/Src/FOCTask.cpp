#include "task_public.h"
#include "FOC.h"
#include "FOC_config.h"
#include "tim.h"
#include "spi.h"
#include "adc.h"
#include "cmsis_os2.h"
#include "Encoder_MT6825.h"
#include "BLDC_Driver_FD6288.h"
#include "Storage_EmbeddedFlash.h"
#include "CurrentSensor_Embed.h"
#include "filters.h"

BLDC_Driver_DRV8300 bldc_driver(&htim1, 2125);
Encoder_MT6825 bldc_encoder(SPI1_CSn_GPIO_Port, SPI1_CSn_Pin, &hspi1);
CurrentSensor_Embed current_sensor(&hadc1, &hadc2);

LowPassFilter_2_Order CurrentQFilter(0.00005f, 1500); // 20kHz
LowPassFilter_2_Order CurrentDFilter(0.00005f, 1500); // 20kHz
LowPassFilter_2_Order SpeedFilter(0.00005f, 300);     // 20kHz

__attribute__((section(".ccmram")))
FOC foc(FOC_POLE_PAIRS, 1000, 20000,
        CurrentQFilter, CurrentDFilter, SpeedFilter,
        bldc_driver, bldc_encoder, storage, current_sensor,
        PID(PID::delta_type,
            FOC_CURRENT_KP,
            FOC_CURRENT_KI,
            FOC_CURRENT_KD,
            NAN,
            NAN,
            1.0f,
            -1.0f
        ),
        PID(PID::delta_type,
            FOC_CURRENT_KP,
            FOC_CURRENT_KI,
            FOC_CURRENT_KD,
            NAN,
            NAN,
            1.0f,
            -1.0f
        ),
        PID(PID::position_type,
            FOC_SPEED_KP,
            FOC_SPEED_KI,
            FOC_SPEED_KD,
            2e3f,
            -2e3f,
            FOC_MAX_CURRENT,
            -FOC_MAX_CURRENT
        ),
        PID(PID::position_type,
            FOC_ANGLE_KP,
            FOC_ANGLE_KI,
            FOC_ANGLE_KD,
            NAN,
            NAN,
            FOC_MAX_SPEED,
            -FOC_MAX_SPEED
        )
);

void StartFOCTask(void *argument) {
    HAL_TIM_Base_Start_IT(&htim6);            // 开启速度环位置环中断控制
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4); //开启PWM输出,用于触发ADC采样
    foc.init();                               // 初始化FOC
    while (true) {
        if (!LL_ADC_REG_IsConversionOngoing(hadc1.Instance)) {
            LL_ADC_REG_StartConversion(hadc1.Instance);
            foc.updateVoltage(hadc1.Instance->DR / 4095.0f * 3.3f / 2 * 17);
            LL_ADC_REG_StopConversion(hadc1.Instance);
        }
        delay(1);
    }
}

__attribute__((section(".ccmram_func")))
void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc) {
    if (&hadc1 == hadc) {
        current_sensor.update();
        foc.loopCtrl();
    }
}

__attribute__((section(".ccmram_func")))
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (&htim6 == htim) {
        foc.Ctrl_ISR();
    }
}
