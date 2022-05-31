#include "ws2812.h"

#define WS2812_HIGH 60
#define WS2812_LOW 30

#if TIM_NUM == 1
#define TIMER htim1
#elif TIM_NUM == 2
#define TIMER htim2
#elif TIM_NUM == 3
#define TIMER htim3
#elif TIM_NUM == 4
#define TIMER htim4
#elif TIM_NUM == 5
#define TIMER htim5
#elif TIM_NUM == 6
#define TIMER htim6
#elif TIM_NUM == 7
#define TIMER htim7
#elif TIM_NUM == 8
#define TIMER htim8
#else
#error
#endif

#if TIM_CH == TIM_CHANNEL_1
#define TIM_DMA_ID TIM_DMA_ID_CC1
#define TIM_DMA_CC TIM_DMA_CC1
#define TIM_CCR CCR1
#elif TIM_CH == TIM_CHANNEL_2
#define TIM_DMA_ID TIM_DMA_ID_CC2
#define TIM_DMA_CC TIM_DMA_CC2
#define TIM_CCR CCR2
#elif TIM_CH == TIM_CHANNEL_3
#define TIM_DMA_ID TIM_DMA_ID_CC3
#define TIM_DMA_CC TIM_DMA_CC3
#define TIM_CCR CCR3
#elif TIM_CH == TIM_CHANNEL_4
#define TIM_DMA_ID TIM_DMA_ID_CC4
#define TIM_DMA_CC TIM_DMA_CC4
#define TIM_CCR CCR4
#endif

#if TIMER == 1 || TIMER == 8
#define APB1
#else
#define APB2
#endif	

volatile uint32_t WS2812_buf[WS2812_SINGLE*2];
volatile ws2812color_t color_buf[WS2812_DIOD] = {0};
uint16_t ws2812_bufcount = 0;
uint16_t ws2812_max = WS2812_DIOD+2;
uint16_t high,low;
uint16_t error_count = 0;

extern TIM_HandleTypeDef TIMER;
extern DMA_HandleTypeDef DMA_HANDLE;

void WS2818_TIM_DMADelayHalfCplt(DMA_HandleTypeDef *hdma);
void WS2818_TIM_DMADelayCplt(DMA_HandleTypeDef *hdma);




void WS2812_Init(void)
{
	    uint32_t APBfq; // Clock freq
#ifdef APB1
    APBfq = HAL_RCC_GetPCLK1Freq();
    APBfq *= (RCC->CFGR & RCC_CFGR_PPRE1) == 0 ? 1 : 2;
#endif
#ifdef APB2
    APBfq = HAL_RCC_GetPCLK2Freq();
    APBfq *= (RCC->CFGR & RCC_CFGR_PPRE2) == 0 ? 1 : 2;
#endif
	APBfq /= (uint32_t) (800 * 1000);  // 800 KHz - 1.25us
	TIMER.Instance->PSC = 0;                        // dummy hardcode now
    TIMER.Instance->ARR = (uint16_t) (APBfq - 1);   // set timer prescaler
    TIMER.Instance->EGR = 1;                        // update timer registers
		high = (uint8_t) (APBfq * 0.56);     // Log.1 - 56% - 0.70us
    low	 = (uint8_t) (APBfq * 0.28);     // Log.0 - 28% - 0.35us
	
	  TIM_CCxChannelCmd(TIMER.Instance, TIM_CH, TIM_CCx_ENABLE); // Enable GPIO to IDLE state
    HAL_Delay(1); // Make some delay
}


void WS2812_SetColour(uint8_t colR,uint8_t colG, uint8_t colB, uint16_t pos)
{
		color_buf[pos].red=colR;
		color_buf[pos].green=colG;	
		color_buf[pos].blue=colB;
}




void WS2812_Clear(void)
{
	uint8_t i;
	for(i = 0; i< WS2812_DIOD; i++)
	{
		color_buf[i].red=0x00;
		color_buf[i].green=0x00;	
		color_buf[i].blue=0x00;
	}
}

void WS2812_SetLed(uint8_t colR,uint8_t colG,uint8_t colB, uint16_t ledX,uint16_t ledY)
{
	uint16_t pos = ledX+ledY*WS2812_DIODCOLS;
	WS2812_SetColour(colR,colG, colB, pos);
}

void WS2812_SetRow(uint8_t colR,uint8_t colG,uint8_t colB, uint16_t ledX)
{
	ledX%=WS2812_DIODCOLS;
	if(ledX >= WS2812_DIODROWS) return;
	else
	{
		for(uint8_t i = 0; i < WS2812_DIODCOLS;i++)
		{
			WS2812_SetLed(colR,colG,colB,ledX,i);
		}
	}
}

void WS2812_SetCol(uint8_t colR,uint8_t colG,uint8_t colB, uint16_t ledY)
{
	ledY%=WS2812_DIODCOLS;
	if(ledY >= WS2812_DIODCOLS) return;
	else
	{
		for(uint8_t i = 0; i < WS2812_DIODCOLS;i++)
		{
			WS2812_SetLed(colR,colG,colB,i,ledY);
		}
	}
}






void WS2812_Send(void)
{
	
	    if (ws2812_bufcount != 0 || DMA_HANDLE.State != HAL_DMA_STATE_READY) {
        return;
	//if(ws2812_bufcount!=0)return;
			}
	else
	{
		for(uint8_t i = 0; i<8;i++)
		{
			WS2812_buf[i]	 	= (color_buf[0].green << i)&0x80? high:low;
			WS2812_buf[i+8]	 	= (color_buf[0].red << i)&0x80? high:low;
			WS2812_buf[i+16]	= (color_buf[0].blue << i)&0x80? high:low;
			#if WS2812_DIOD > 1
			WS2812_buf[i+24]	= (color_buf[1].green << i)&0x80? high:low;
			WS2812_buf[i+32]	= (color_buf[1].red << i)&0x80? high:low;
			WS2812_buf[i+40]	= (color_buf[1].blue << i)&0x80? high:low;
			#else 
			memset((uint32_t *)&WS2812_buf[3], 0, WS2812_SINGLE*sizeof(uint32_t));
			#endif
		}
		        HAL_StatusTypeDef DMA_Send_Stat = HAL_ERROR;
        while (DMA_Send_Stat != HAL_OK) {
            if (TIM_CHANNEL_STATE_GET(&TIMER, TIM_CH) == HAL_TIM_CHANNEL_STATE_BUSY) {
                DMA_Send_Stat = HAL_BUSY;
                continue;
            } else if (TIM_CHANNEL_STATE_GET(&TIMER, TIM_CH) == HAL_TIM_CHANNEL_STATE_READY) {
                TIM_CHANNEL_STATE_SET(&TIMER, TIM_CH, HAL_TIM_CHANNEL_STATE_BUSY);
            } else {
                DMA_Send_Stat = HAL_ERROR;
                continue;
            }

            TIMER.hdma[TIM_DMA_ID]->XferCpltCallback = WS2818_TIM_DMADelayCplt;
            TIMER.hdma[TIM_DMA_ID]->XferHalfCpltCallback = WS2818_TIM_DMADelayHalfCplt;
            TIMER.hdma[TIM_DMA_ID]->XferErrorCallback = TIM_DMAError;
            if (HAL_DMA_Start_IT(TIMER.hdma[TIM_DMA_ID], (uint32_t) WS2812_buf,
                                 (uint32_t) &TIMER.Instance->TIM_CCR,
                                 48) != HAL_OK) {
                DMA_Send_Stat = HAL_ERROR;
                continue;
            }
            __HAL_TIM_ENABLE_DMA(&TIMER, TIM_DMA_CC);
            if (IS_TIM_BREAK_INSTANCE(TIMER.Instance) != RESET)
                __HAL_TIM_MOE_ENABLE(&TIMER);
            if (IS_TIM_SLAVE_INSTANCE(TIMER.Instance)) {
                uint32_t tmpsmcr = TIMER.Instance->SMCR & TIM_SMCR_SMS;
                if (!IS_TIM_SLAVEMODE_TRIGGER_ENABLED(tmpsmcr))
                    __HAL_TIM_ENABLE(&TIMER);
            } else
                __HAL_TIM_ENABLE(&TIMER);
            DMA_Send_Stat = HAL_OK;
        }
				#if WS2812_DIOD > 1
				ws2812_bufcount = 2;
				#else 
				ws2812_bufcount = 1;
				#endif
		return;
	}
}

void WS2818_TIM_DMADelayHalfCplt(DMA_HandleTypeDef *hdma)
{
	TIM_HandleTypeDef *htim = (TIM_HandleTypeDef *) ((DMA_HandleTypeDef *) hdma)->Parent;
	if (hdma != &DMA_HANDLE || htim != &TIMER || ws2812_bufcount == 0) return;
	if(ws2812_bufcount < WS2812_DIOD)
	{
		for(int i = 0; i<8;i++)
		{
			WS2812_buf[i]	 	= 	(color_buf[ws2812_bufcount].green << i)&0x80? high:low;
			WS2812_buf[i+8]	 	= (color_buf[ws2812_bufcount].red 	<< i)&0x80? high:low;
			WS2812_buf[i+16]	= (color_buf[ws2812_bufcount].blue 	<< i)&0x80? high:low;
			
		}
	}
	else if(ws2812_bufcount < ws2812_max)
	{
		memset((uint32_t *)&WS2812_buf[0], 0, WS2812_SINGLE*sizeof(uint32_t));
	}
	ws2812_bufcount++;

}

void WS2818_TIM_DMADelayCplt(DMA_HandleTypeDef *hdma)
{
  TIM_HandleTypeDef *htim = (TIM_HandleTypeDef *) ((DMA_HandleTypeDef *) hdma)->Parent;
  // if wrong handlers
    if (hdma != &DMA_HANDLE || htim != &TIMER) return;
    if (ws2812_bufcount == 0) return; // if no data to transmit - return
    if (hdma == htim->hdma[TIM_DMA_ID_CC1]) {
        htim->Channel = HAL_TIM_ACTIVE_CHANNEL_1;
        if (hdma->Init.Mode == DMA_NORMAL) {
            TIM_CHANNEL_STATE_SET(htim, TIM_CHANNEL_1, HAL_TIM_CHANNEL_STATE_READY);
        }
    } else if (hdma == htim->hdma[TIM_DMA_ID_CC2]) {
        htim->Channel = HAL_TIM_ACTIVE_CHANNEL_2;
        if (hdma->Init.Mode == DMA_NORMAL) {
            TIM_CHANNEL_STATE_SET(htim, TIM_CHANNEL_2, HAL_TIM_CHANNEL_STATE_READY);
        }
    } else if (hdma == htim->hdma[TIM_DMA_ID_CC3]) {
        htim->Channel = HAL_TIM_ACTIVE_CHANNEL_3;
        if (hdma->Init.Mode == DMA_NORMAL) {
            TIM_CHANNEL_STATE_SET(htim, TIM_CHANNEL_3, HAL_TIM_CHANNEL_STATE_READY);
        }
    } else if (hdma == htim->hdma[TIM_DMA_ID_CC4]) {
        htim->Channel = HAL_TIM_ACTIVE_CHANNEL_4;
        if (hdma->Init.Mode == DMA_NORMAL) {
            TIM_CHANNEL_STATE_SET(htim, TIM_CHANNEL_4, HAL_TIM_CHANNEL_STATE_READY);
        }
    } else {
        /* nothing to do */
    }
	if(ws2812_bufcount < WS2812_DIOD)
	{
		for(int i = 0; i<8;i++)
		{
			
			WS2812_buf[i+24]	 	= (color_buf[ws2812_bufcount].green << i)&0x80? high:low;
			WS2812_buf[i+32]	 	= (color_buf[ws2812_bufcount].red << i)&0x80? high:low;
			WS2812_buf[i+40]	= (color_buf[ws2812_bufcount].blue << i)&0x80? high:low;

		}
 
	}
	else if(ws2812_bufcount < ws2812_max)
	{
		memset((uint32_t *)&WS2812_buf[3], 0, WS2812_SINGLE*sizeof(uint32_t));
	}
	ws2812_bufcount++;
	
	ws2812_bufcount %= ws2812_max;
	if(!ws2812_bufcount)
	{
		__HAL_TIM_DISABLE_DMA(htim, TIM_DMA_CC2);
		(void)HAL_DMA_Abort_IT(htim->hdma[TIM_DMA_ID_CC2]);
		if (IS_TIM_BREAK_INSTANCE(htim->Instance) != RESET) 
			{
        __HAL_TIM_MOE_DISABLE(htim);
			}
			/* Disable the Peripheral */
			__HAL_TIM_DISABLE(htim);
			/* Set the TIM channel state */
			TIM_CHANNEL_STATE_SET(htim, TIM_CH, HAL_TIM_CHANNEL_STATE_READY);
			
	}
	htim->Channel = HAL_TIM_ACTIVE_CHANNEL_CLEARED;
}
