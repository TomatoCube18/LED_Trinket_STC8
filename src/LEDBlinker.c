// STC8 LED Trinket v1.0
// Last Modified: 19 Nov 2022
// Function: Off -> On -> Strobe -> Alternate
//
//           +-----------+
//          <| 1         |> Switch (Assert Low)
//     VCC  <|           |>
//          <|           |> LED 2 (+)
//     GND  <|           |> LED 1 (+)
//           +-----------+
//

#include "fw_hal.h"

uint16_t button1Buffer = 0x0F;
uint8_t lastButtonState = 0;
uint8_t led_mode = 0;
uint8_t wake_up = 0;

static uint16_t ticks_count = 0;

#define Nop() \
    __asm nop \
        __endasm

void GPIO_Init(void)
{
    // Output Pins:
    // P30, P31
    GPIO_P3_SetMode(GPIO_Pin_0 | GPIO_Pin_1, GPIO_Mode_Output_PP);

    // Input Pins:
    // P33
    GPIO_P3_SetMode(GPIO_Pin_3, GPIO_Mode_Input_HIP);
    GPIO_SetPullUp(GPIO_Port_3, GPIO_Pin_3, HAL_State_ON);
    // P3 = (P3 | 0x08);
}

void setLED(uint8_t leds)
{
    P3 = (P3 & 0b11111100) | (leds & 0b11);
}

INTERRUPT(Timer0_Routine, EXTI_VectTimer0)
{
    ticks_count++;
    if (ticks_count >= 5000)
    {
        ticks_count = 0;
    }
}

void TimeStart()
{
    TIM_Timer0_Config(HAL_State_ON, TIM_TimerMode_16BitAuto, 1000);
    EXTI_Timer0_SetIntState(HAL_State_ON);
    EXTI_Timer0_SetIntPriority(EXTI_IntPriority_High);
    EXTI_Global_SetIntState(HAL_State_ON);
    TIM_Timer0_SetRunState(HAL_State_ON);
}

int main(void)
{
    SYS_SetClock();
    GPIO_Init();
    TimeStart();

    while (1)
    {
        button1Buffer = (button1Buffer << 1) | ((P3 & 0x08) >> 3);
        if ((button1Buffer & 0x1FF) == 0x00)
        { // If ButtonPressed is detected
            lastButtonState = 1;
        }
        else if ((button1Buffer & 0x1FF) == 0x0FF)
        { // Detect Last Button Down (single choice)
            led_mode = (lastButtonState == 1) ? (led_mode < 4) ? led_mode + 1 : 0 : led_mode;
            lastButtonState = 0;
            ticks_count = 0;
        }

        // Leds Control
        switch (led_mode)
        {
        case 0: // Led Off
            setLED(0x00);

            // Sleep STC8
            IE1 = 0; // 外中断1标志位
            IE0 = 0; // 外中断0标志位
            EX1 = 1; // INT1 Enable
            EX0 = 1; // INT0 Enable
            IT1 = 1; // INT1 下降沿中断

            PCON |= 0x02; // Sleep
            Nop();
            Nop();
            Nop();
            Nop();

            // Workaround: Overly sensitive wake on pinChange
            wake_up = 5; // Increase number to decrease sensitivity
            while (wake_up)
            {
                // Check pin level is indeed LOW
                if ((P3 & 0x08) == 0)
                {
                    wake_up -= 1;
                }
                else // Else reset counter & Go back to Sleep
                {
                    wake_up = 5;
                    setLED(0x00);
                    IE1 = 0; // 外中断1标志位
                    IE0 = 0; // 外中断0标志位
                    EX1 = 1; // INT1 Enable
                    EX0 = 1; // INT0 Enable
                    IT1 = 1; // INT1 下降沿中断

                    PCON |= 0x02; // Sleep
                    Nop();
                    Nop();
                    Nop();
                    Nop();
                }
            }

            // Hold till button is release
            while ((P3 & 0x08) == 0)
                ;

            led_mode = 1;
            break;
        case 1: // On
            setLED(0x03);
            break;
        case 2: // Alternate
            if (ticks_count < 500)
            {
                setLED(0x01);
            }
            else
            {
                setLED(0x02);
                if (ticks_count >= 1000)
                {
                    ticks_count = 0;
                }
            }
            break;
        case 3: // Strobe
            // _-_-_-______ _-_-_-______
            //
            if ((ticks_count < 150) || ((ticks_count > 300) && (ticks_count < 450)) || ((ticks_count > 600) && (ticks_count < 750)) || (ticks_count > 900))
            {
                setLED(0x00);
                if (ticks_count >= 2000)
                {
                    ticks_count = 0;
                }
            }
            else
            {
                setLED(0x03);
            }
            break;
        case 4: // Blink
            if (ticks_count < 500)
            {
                setLED(0x00);
            }
            else
            {
                setLED(0x03);
                if (ticks_count >= 1000)
                {
                    ticks_count = 0;
                }
            }
            break;
        default:
            led_mode = 0;
            break;
        }
    }
}
