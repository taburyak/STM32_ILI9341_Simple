/*
 * ili9341_light.c
 *  Created on: 2023-08-15
 *      Author: tabur
 */
#include <ili9341_simple.h>
#include "main.h"
#include <stdbool.h>

enum {
  MemoryAccessControlNormalOrder,
  MemoryAccessControlReverseOrder
} MemoryAccessControlRefreshOrder;

enum {
	MemoryAccessControlColorOrderRGB,
	MemoryAccessControlColorOrderBGR
} MemoryAccessControlColorOrder;

static unsigned char lcdPortraitConfig = 0;
static unsigned char lcdLandscapeConfig = 0;
static unsigned char lcdPortraitMirrorConfig = 0;
static unsigned char lcdLandscapeMirrorConfig = 0;
static lcdPropertiesTypeDef  lcdProperties = { ILI9341_PIXEL_WIDTH, ILI9341_PIXEL_HEIGHT, LCD_ORIENTATION_PORTRAIT, true, true };

extern DMA_HandleTypeDef hdma_memtomem_dma2_stream0;

// Write an 8 bit command to the IC driver
static void lcdWriteCommand(unsigned char command)
{
	LCD_CmdWrite(command);
}

// Write an 16 bit data word to the IC driver
static void lcdWriteData(unsigned short data)
{
	LCD_DataWrite(data);
}

static void lcdReset(void)
{
	lcdWriteCommand(ILI9341_SOFTRESET);
	HAL_Delay(50);
}

static unsigned char lcdBuildMemoryAccessControlConfig(
                        bool rowAddressOrder,
                        bool columnAddressOrder,
                        bool rowColumnExchange,
                        bool verticalRefreshOrder,
                        bool colorOrder,
                        bool horizontalRefreshOrder)
{
  unsigned char value 				      = 0;
  if(horizontalRefreshOrder) value 	|= ILI9341_MADCTL_MH;
  if(colorOrder) value 				      |= ILI9341_MADCTL_BGR;
  if(verticalRefreshOrder) value 	  |= ILI9341_MADCTL_ML;
  if(rowColumnExchange) value 		  |= ILI9341_MADCTL_MV;
  if(columnAddressOrder) value 		  |= ILI9341_MADCTL_MX;
  if(rowAddressOrder) value 		    |= ILI9341_MADCTL_MY;
  return value;
}

uint16_t lcdReadID(void)
{
	uint16_t id;
	lcdWriteCommand(0xD3);
	(void)LCD_DataRead();     // Dummy read
	(void)LCD_DataRead();     // Skip 0x00
	id = LCD_DataRead() << 8; // Read 0x93
	id |= LCD_DataRead();     // Read 0x41
	return id;
}

void lcdInit(void)
{
  lcdPortraitConfig = lcdBuildMemoryAccessControlConfig(
                                                    MemoryAccessControlNormalOrder,		// rowAddressOrder
                                                    MemoryAccessControlReverseOrder,	// columnAddressOrder
                                                    MemoryAccessControlNormalOrder,		// rowColumnExchange
                                                    MemoryAccessControlNormalOrder,		// verticalRefreshOrder
                                                    MemoryAccessControlColorOrderBGR,	// colorOrder
                                                    MemoryAccessControlNormalOrder);	// horizontalRefreshOrder

  lcdLandscapeConfig = lcdBuildMemoryAccessControlConfig(
                                                    MemoryAccessControlNormalOrder,		// rowAddressOrder
                                                    MemoryAccessControlNormalOrder,		// columnAddressOrder
                                                    MemoryAccessControlReverseOrder,	// rowColumnExchange
                                                    MemoryAccessControlNormalOrder,		// verticalRefreshOrder
                                                    MemoryAccessControlColorOrderBGR,	// colorOrder
                                                    MemoryAccessControlNormalOrder);	// horizontalRefreshOrder

  lcdPortraitMirrorConfig = lcdBuildMemoryAccessControlConfig(
		  	  	  	  	  	  	  	  	  	  	  	MemoryAccessControlReverseOrder,	// rowAddressOrder
		                                            MemoryAccessControlNormalOrder,		// columnAddressOrder
		                                            MemoryAccessControlNormalOrder,		// rowColumnExchange
		                                            MemoryAccessControlNormalOrder,		// verticalRefreshOrder
		                                            MemoryAccessControlColorOrderBGR,	// colorOrder
		                                            MemoryAccessControlNormalOrder);	// horizontalRefreshOrder

  lcdLandscapeMirrorConfig = lcdBuildMemoryAccessControlConfig(
                                                    MemoryAccessControlReverseOrder,	// rowAddressOrder
                                                    MemoryAccessControlReverseOrder,	// columnAddressOrder
                                                    MemoryAccessControlReverseOrder,	// rowColumnExchange
                                                    MemoryAccessControlNormalOrder,		// verticalRefreshOrder
                                                    MemoryAccessControlColorOrderBGR,	// colorOrder
                                                    MemoryAccessControlNormalOrder);	// horizontalRefreshOrder

  lcdReset();

  lcdWriteCommand(ILI9341_DISPLAYOFF);

  lcdWriteCommand(0xCF);
  lcdWriteData(0x00);
  lcdWriteData(0x83);
  lcdWriteData(0x30);

  lcdWriteCommand(0xED);
  lcdWriteData(0x64);
  lcdWriteData(0x03);
  lcdWriteData(0x12);
  lcdWriteData(0x81);

  lcdWriteCommand(0xE8);
  lcdWriteData(0x85);
  lcdWriteData(0x01);
  lcdWriteData(0x79);

  lcdWriteCommand(0xCB);
  lcdWriteData(0x39);
  lcdWriteData(0x2C);
  lcdWriteData(0x00);
  lcdWriteData(0x34);
  lcdWriteData(0x02);

  lcdWriteCommand(0xF7);
  lcdWriteData(0x20);

  lcdWriteCommand(0xEA);
  lcdWriteData(0x00);
  lcdWriteData(0x00);

  lcdWriteCommand(ILI9341_POWERCONTROL1);
  lcdWriteData(0x26);

  lcdWriteCommand(ILI9341_POWERCONTROL2);
  lcdWriteData(0x11);

  lcdWriteCommand(ILI9341_VCOMCONTROL1);
  lcdWriteData(0x35);
  lcdWriteData(0x3E);

  lcdWriteCommand(ILI9341_VCOMCONTROL2);
  lcdWriteData(0xBE);

  lcdWriteCommand(ILI9341_MEMCONTROL);
  lcdWriteData(lcdPortraitMirrorConfig);

  lcdWriteCommand(ILI9341_PIXELFORMAT);
  lcdWriteData(0x55);

  lcdWriteCommand(ILI9341_FRAMECONTROLNORMAL);
  lcdWriteData(0x00);
  lcdWriteData(0x1B);

  lcdWriteCommand(0xF2);
  lcdWriteData(0x08);

  lcdWriteCommand(ILI9341_GAMMASET);
  lcdWriteData(0x01);

  lcdWriteCommand(ILI9341_POSITIVEGAMMCORR);
  lcdWriteData(0x1F);
  lcdWriteData(0x1A);
  lcdWriteData(0x18);
  lcdWriteData(0x0A);
  lcdWriteData(0x0F);
  lcdWriteData(0x06);
  lcdWriteData(0x45);
  lcdWriteData(0x87);
  lcdWriteData(0x32);
  lcdWriteData(0x0A);
  lcdWriteData(0x07);
  lcdWriteData(0x02);
  lcdWriteData(0x07);
  lcdWriteData(0x05);
  lcdWriteData(0x00);

  lcdWriteCommand(ILI9341_NEGATIVEGAMMCORR);
  lcdWriteData(0x00);
  lcdWriteData(0x25);
  lcdWriteData(0x27);
  lcdWriteData(0x05);
  lcdWriteData(0x10);
  lcdWriteData(0x09);
  lcdWriteData(0x3A);
  lcdWriteData(0x78);
  lcdWriteData(0x4D);
  lcdWriteData(0x05);
  lcdWriteData(0x18);
  lcdWriteData(0x0D);
  lcdWriteData(0x38);
  lcdWriteData(0x3A);
  lcdWriteData(0x1F);

  lcdWriteCommand(ILI9341_COLADDRSET);
  lcdWriteData(0x00);
  lcdWriteData(0x00);
  lcdWriteData(0x00);
  lcdWriteData(0xEF);

  lcdWriteCommand(ILI9341_PAGEADDRSET);
  lcdWriteData(0x00);
  lcdWriteData(0x00);
  lcdWriteData(0x01);
  lcdWriteData(0x3F);

  lcdWriteCommand(ILI9341_ENTRYMODE);
  lcdWriteData(0x07);

  lcdWriteCommand(ILI9341_DISPLAYFUNC);
  lcdWriteData(0x0A);
  lcdWriteData(0x82);
  lcdWriteData(0x27);
  lcdWriteData(0x00);

  lcdWriteCommand(ILI9341_SLEEPOUT);
  HAL_Delay(100);
  lcdWriteCommand(ILI9341_DISPLAYON);
  HAL_Delay(100);
  lcdWriteCommand(ILI9341_MEMORYWRITE);
}

void lcdBacklightOff(void)
{
	LCD_BL_OFF();
}

void lcdBacklightOn(void)
{
	LCD_BL_ON();
}

void lcdSetOrientation(lcdOrientationTypeDef value)
{
	lcdProperties.orientation = value;
	lcdWriteCommand(ILI9341_MEMCONTROL);

	switch (lcdProperties.orientation)
	{
		case LCD_ORIENTATION_PORTRAIT:
			lcdWriteData(lcdPortraitConfig);
			lcdProperties.width = ILI9341_PIXEL_WIDTH;
			lcdProperties.height = ILI9341_PIXEL_HEIGHT;
			break;
		case LCD_ORIENTATION_PORTRAIT_MIRROR:
			lcdWriteData(lcdPortraitMirrorConfig);
			lcdProperties.width = ILI9341_PIXEL_WIDTH;
			lcdProperties.height = ILI9341_PIXEL_HEIGHT;
			break;
		case LCD_ORIENTATION_LANDSCAPE:
			lcdWriteData(lcdLandscapeConfig);
			lcdProperties.width = ILI9341_PIXEL_HEIGHT;
			lcdProperties.height = ILI9341_PIXEL_WIDTH;
			break;
		case LCD_ORIENTATION_LANDSCAPE_MIRROR:
			lcdWriteData(lcdLandscapeMirrorConfig);
			lcdProperties.width = ILI9341_PIXEL_HEIGHT;
			lcdProperties.height = ILI9341_PIXEL_WIDTH;
			break;
		default:
			break;
	}

	//lcdWriteCommand(ILI9341_MEMORYWRITE);
	lcdSetWindow(0, 0, lcdProperties.width - 1, lcdProperties.height - 1);
}

void lcdSetWindow(unsigned short x0, unsigned short y0, unsigned short x1, unsigned short y1)
{
  lcdWriteCommand(ILI9341_COLADDRSET);
  lcdWriteData((x0 >> 8) & 0xFF);
  lcdWriteData(x0 & 0xFF);
  lcdWriteData((x1 >> 8) & 0xFF);
  lcdWriteData(x1 & 0xFF);
  lcdWriteCommand(ILI9341_PAGEADDRSET);
  lcdWriteData((y0 >> 8) & 0xFF);
  lcdWriteData(y0 & 0xFF);
  lcdWriteData((y1 >> 8) & 0xFF);
  lcdWriteData(y1 & 0xFF);
  lcdWriteCommand(ILI9341_MEMORYWRITE);
}

void lcdTest(void)
{
	lcdSetWindow(0, 0, lcdProperties.width - 1, lcdProperties.height - 1);

	uint8_t stripSize = lcdProperties.height / 8;

	for (int y = 0; y < lcdProperties.height; y++)
	{
		for (int x = 0; x < lcdProperties.width; x++)
		{
		  if (y > lcdProperties.height - 1 - (stripSize * 1)) lcdWriteData(COLOR_WHITE);
		  else if (y > lcdProperties.height - 1 - (stripSize * 2)) lcdWriteData(COLOR_BLUE);
		  else if (y > lcdProperties.height - 1 - (stripSize * 3)) lcdWriteData(COLOR_GREEN);
		  else if (y > lcdProperties.height - 1 - (stripSize * 4)) lcdWriteData(COLOR_CYAN);
		  else if (y > lcdProperties.height - 1 - (stripSize * 5)) lcdWriteData(COLOR_RED);
		  else if (y > lcdProperties.height - 1 - (stripSize * 6)) lcdWriteData(COLOR_MAGENTA);
		  else if (y > lcdProperties.height - 1 - (stripSize * 7)) lcdWriteData(COLOR_YELLOW);
		  else lcdWriteData(COLOR_BLACK);
		}
	}
}

void lcdFlushFrameBufferDMA(DMA_HandleTypeDef *hdma, uint16_t *buffer, uint32_t pixels_count)
{
    if (hdma == NULL || buffer == NULL || pixels_count == 0)
    {
        return;
    }

    lcdWaitDMAIdle(hdma);

    /* 1. Issue RAMWR (0x2C) command to start writing into GRAM */
    LCD_CmdWrite(ILI9341_MEMORYWRITE);

    /* 2. Transfer pixel buffer in chunks of <= 60000 halfwords via DMA (clear XferCpltCallback so polling works) */
    void (*saved_cb)(DMA_HandleTypeDef *) = hdma->XferCpltCallback;
    hdma->XferCpltCallback = NULL;

    uint32_t transferred = 0;
    while (pixels_count > 0)
    {
        uint32_t chunk = (pixels_count > 60000) ? 60000 : pixels_count;

        HAL_DMA_Start(hdma,
                      (uint32_t)(buffer + transferred),
                      (uint32_t)LCD_BASE1,
                      chunk);

        HAL_DMA_PollForTransfer(hdma, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);

        transferred += chunk;
        pixels_count -= chunk;
    }

    hdma->XferCpltCallback = saved_cb;
}

void lcdWaitDMAIdle(DMA_HandleTypeDef *hdma)
{
    if (hdma == NULL)
    {
        return;
    }

    uint32_t start = HAL_GetTick();
    while (HAL_DMA_GetState(hdma) == HAL_DMA_STATE_BUSY)
    {
        if ((HAL_GetTick() - start) > 100U)
        {
            (void)HAL_DMA_Abort(hdma);
            break;
        }
    }
}

bool lcdFlushFrameBufferDMA_IT(DMA_HandleTypeDef *hdma, uint16_t *buffer, uint32_t pixels_count)
{
    if (hdma == NULL || buffer == NULL || pixels_count == 0U || pixels_count > 65535U)
    {
        return false;
    }

    lcdWaitDMAIdle(hdma);
    LCD_CmdWrite(ILI9341_MEMORYWRITE);

    if (HAL_DMA_Start_IT(hdma, (uint32_t)buffer, (uint32_t)LCD_BASE1, pixels_count) != HAL_OK)
    {
        return false;
    }
    return true;
}

/* DMA source variable in internal SRAM (never in CCM or on a task stack) */
static volatile uint16_t s_dma_fill_color __attribute__((aligned(4))) = 0U;

void lcdFillConstantDMA(DMA_HandleTypeDef *hdma, uint16_t color, uint32_t pixels_count)
{
    if (hdma == NULL || pixels_count == 0U)
    {
        return;
    }

    lcdWaitDMAIdle(hdma);
    s_dma_fill_color = color;
    __DSB();

    LCD_CmdWrite(ILI9341_MEMORYWRITE);

    /* Temporarily clear XferCpltCallback so HAL_DMA_PollForTransfer is allowed by STM32 HAL */
    void (*saved_cb)(DMA_HandleTypeDef *) = hdma->XferCpltCallback;
    hdma->XferCpltCallback = NULL;

    /* Temporarily disable source increment (PINC) so DMA reads s_dma_fill_color repeatedly */
    hdma->Instance->CR &= ~DMA_SxCR_PINC;

    while (pixels_count > 0U)
    {
        uint32_t chunk = (pixels_count > 60000U) ? 60000U : pixels_count;

        HAL_DMA_Start(hdma,
                      (uint32_t)&s_dma_fill_color,
                      (uint32_t)LCD_BASE1,
                      chunk);

        HAL_DMA_PollForTransfer(hdma, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
        pixels_count -= chunk;
    }

    /* Restore source increment and LVGL completion callback */
    hdma->Instance->CR |= DMA_SxCR_PINC;
    hdma->XferCpltCallback = saved_cb;
    __DSB();
}

void lcdReadPixelsRGB565(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t *dst)
{
    if (dst == NULL || w == 0U || h == 0U)
    {
        return;
    }

    lcdWaitDMAIdle(&hdma_memtomem_dma2_stream0);

    /* ILI9341 GRAM read (0x2E, t_RATFM <= 340 ns) is much slower than GRAM write (t_WC >= 66 ns).
     * Temporarily widen FSMC Bank 4 read timing (ADDSET=6, DATAST=36, BUSTURN=4 -> ~250 ns)
     * so the controller outputs valid RGB666 bytes without bus lockup. */
    uint32_t saved_btr = FSMC_Bank1->BTCR[7];
    FSMC_Bank1->BTCR[7] = (saved_btr & ~(0xFFUL | (0xFFUL << 8) | (0x0FUL << 16))) |
                          (6UL << 0) | (36UL << 8) | (4UL << 16);
    __DSB();

    uint16_t x1 = (uint16_t)(x0 + w - 1U);
    uint16_t y1 = (uint16_t)(y0 + h - 1U);

    lcdWriteCommand(ILI9341_COLADDRSET);
    lcdWriteData((x0 >> 8) & 0xFFU);
    lcdWriteData(x0 & 0xFFU);
    lcdWriteData((x1 >> 8) & 0xFFU);
    lcdWriteData(x1 & 0xFFU);

    lcdWriteCommand(ILI9341_PAGEADDRSET);
    lcdWriteData((y0 >> 8) & 0xFFU);
    lcdWriteData(y0 & 0xFFU);
    lcdWriteData((y1 >> 8) & 0xFFU);
    lcdWriteData(y1 & 0xFFU);

    /* Issue RAMRD (0x2E): first read is a dummy cycle, followed by packed R8G8 / B8R8 / G8B8 */
    lcdWriteCommand(ILI9341_MEMORYREAD);
    (void)LCD_DataRead();

    uint32_t total = (uint32_t)w * (uint32_t)h;
    uint32_t idx   = 0U;
    while ((idx + 1U) < total)
    {
        uint16_t w0 = LCD_DataRead(); /* R0[15:8], G0[7:0] */
        uint16_t w1 = LCD_DataRead(); /* B0[15:8], R1[7:0] */
        uint16_t w2 = LCD_DataRead(); /* G1[15:8], B1[7:0] */

        uint8_t r0 = (uint8_t)(w0 >> 8);
        uint8_t g0 = (uint8_t)(w0 & 0xFFU);
        uint8_t b0 = (uint8_t)(w1 >> 8);

        uint8_t r1 = (uint8_t)(w1 & 0xFFU);
        uint8_t g1 = (uint8_t)(w2 >> 8);
        uint8_t b1 = (uint8_t)(w2 & 0xFFU);

        dst[idx]      = (uint16_t)(((r0 & 0xF8U) << 8) | ((g0 & 0xFCU) << 3) | (b0 >> 3));
        dst[idx + 1U] = (uint16_t)(((r1 & 0xF8U) << 8) | ((g1 & 0xFCU) << 3) | (b1 >> 3));
        idx += 2U;
    }

    if (idx < total)
    {
        uint16_t w0 = LCD_DataRead();
        uint16_t w1 = LCD_DataRead();
        uint8_t r0 = (uint8_t)(w0 >> 8);
        uint8_t g0 = (uint8_t)(w0 & 0xFFU);
        uint8_t b0 = (uint8_t)(w1 >> 8);
        dst[idx] = (uint16_t)(((r0 & 0xF8U) << 8) | ((g0 & 0xFCU) << 3) | (b0 >> 3));
    }

    /* Restore fast FSMC Bank 4 write timing */
    FSMC_Bank1->BTCR[7] = saved_btr;
    __DSB();
}

