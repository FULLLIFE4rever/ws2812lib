#ifndef __WS2812
#define __WS2812



#ifndef WS2812_DIODROWS
#define WS2812_DIODROWS 2
#endif

#ifndef WS2812_DIODCOLS
#define WS2812_DIODCOLS 2
#endif

#define WS2812_SINGLE 24

#define WS2812_DIOD WS2812_DIODCOLS*WS2812_DIODROWS


#define TIM_NUM	   4  ///< Timer number
#define TIM_CH	   TIM_CHANNEL_2  ///< Timer's PWM channel
#define DMA_HANDLE hdma_tim4_ch2 ///< DMA Channel
#define DMA_SIZE_WORD     ///< DMA Memory Data Width: {.._BYTE, .._HWORD, .._WORD}


#include "stm32f4xx_hal.h"
#include <string.h>

typedef enum ARGB_STATE {
    ARGB_BUSY = 0,  ///< DMA Transfer in progress
    ARGB_READY = 1, ///< DMA Ready to transfer
    ARGB_OK = 2,    ///< Function execution success
    ARGB_PARAM_ERR = 3, ///< Error in input parameters
} ARGB_STATE;

typedef struct
{
	uint8_t red;
	uint8_t	green;
	uint8_t	blue;
}ws2812color_t;
 

void WS2812_Init(void);
void WS2812_SetColour(uint8_t red,uint8_t green, uint8_t blue, uint16_t pos);

void WS2812_Clear(void);
void WS2812_SetLed(uint8_t colR,uint8_t colG,uint8_t colB, uint16_t ledX,uint16_t ledY);
void WS2812_SetCol(uint8_t colR,uint8_t colG,uint8_t colB, uint16_t ledX);
void WS2812_SetRow(uint8_t colR,uint8_t colG,uint8_t colB, uint16_t ledY);
void WS2812_Send(void);


#endif
