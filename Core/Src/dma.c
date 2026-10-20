/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    dma.c
  * @brief   DMA configuration code.
  ******************************************************************************
  */
/* USER CODE END Header */
#include "dma.h"

/**
  * Enable DMA controller clock and ADC1 DMA interrupt.
  */
void MX_DMA_Init(void)
{
  __HAL_RCC_DMA1_CLK_ENABLE();

  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
}
