// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "systemconfig.h"
#include "usart.h"

TUSART_CONTEXT USARTINFO[3] =
{
    {(TUSART *)USART1},
    {(TUSART *)USART2},
    {(TUSART *)USART3}
};

boolean USART_Initialize(TUSART *USART, uint32_t Speed, void (*Handler)(void), uint32_t Flags)
{
    uint32_t IRQ_CODE, PD_CODE;

    switch ((uint32_t)USART)
    {
    case USART1_base:
        IRQ_CODE = IRQ_UART1_CODE;
        PD_CODE = PD_UART1;
        break;
    case USART2_base:
        IRQ_CODE = IRQ_UART2_CODE;
        PD_CODE = PD_UART2;
        break;
    case USART3_base:
        IRQ_CODE = IRQ_UART3_CODE;
        PD_CODE = PD_UART3;
        break;
    default:
        return false;
//        break;
    }

    NVIC_UnregisterIRQ(IRQ_CODE);                                                                   //Disable USART Interrupts
    PCTL_PowerUp(PD_CODE);                                                                          //Enable USART clock

    USART->LCR = U_WLS(WLS_8);                                                                      //DLAB = 0, LCR in default state

    //Set USART to default state
    USART->DMA_EN = 0x00;                                                                           //Disable DMA
    USART->AUTOBAUD_EN = 0x00;
    USART->IER = 0x00;                                                                              //Disable interrupts
    USART->MCR = 0x00;
    USART->FCR = CLRT | CLRR;                                                                       //Flash TX and RX, and disable FIFO
    USART->LSR = USART->LSR & ~U_DR;                                                                //Reading LSR for status update
    USART->HIGHSPEED = USPEED(USPEED_16);                                                           //16 samples per bit
    USART->RATEFIX_AD = FREQ_SEL_26 | AUTOBAUDRATEFIX_52 | RATEFIX_52;                              //Set to default state

    if (Speed)
    {
        uint32_t BR = (2 * USARTDEFCLOCK / USARTDEFOVS) / Speed;                                    //oversampling by 16, baud rate

        BR = ((BR & 0x0001) ? BR + 1 : BR) >> 1;
        if (BR <= 1) return false;                                                                  //Divider can not be lower or equal 1

        USART->LCR |= U_DLAB;
        USART->DLM = (BR >> 8) & 0xFF;                                                              //Set baud rate
        USART->DLL = BR & 0xFF;
        USART->LCR &= ~U_DLAB;
    }
    else
    {
        PCTL_PowerDown(PD_CODE);
        return false;
    }

    USART_EnableInterrupts(USART, Flags);                                                           //Check interrupt enable flags

    /* Enable HW flow control, if need */
//    if (Flags & USART_HWFlow_EnFlag) USART->CR3 |= (USART_CR3_RTSE | USART_CR3_CTSE);             //TO DO!!!!!!

    if (Flags & (USART_TXInt_EnFlag | USART_RXInt_EnFlag | USART_ERRInt_EnFlag))                    //Configure NVIC
    {
        if  (Handler != NULL)
        {
            NVIC_RegisterIRQ(IRQ_CODE, Handler, IRQ_SENS_EDGE, true);
        }
    }

    if (!(Flags & USART_EnFlag))
    {
        PCTL_PowerDown(PD_CODE);                                                                    //Disable USART clock
    }
    return true;
}

void USART_EnableInterrupts(TUSART *USART, TUSART_FLAGS Flags)
{
    if (USART != NULL)
    {
        uint32_t intflags = DisableInterrupts();
        uint8_t  tmpLCR = USART->LCR;

        USART->LCR &= ~U_DLAB;

        if (Flags & USART_TXInt_EnFlag)  USART->IER |= ETBEI;                                       //Enable transmitter interrupt
        if (Flags & USART_RXInt_EnFlag)  USART->IER |= ERBFI;                                       //Enable receiver interrupt
        if (Flags & USART_ERRInt_EnFlag) USART->IER |= ELSI;                                        //Enable errors interrupt
        USART->LCR = tmpLCR; //-V519
        RestoreInterrupts(intflags);
    }
}

void USART_DisableInterrupts(TUSART *USART, TUSART_FLAGS Flags)
{
    if (USART != NULL)
    {
        uint32_t intflags = DisableInterrupts();
        uint8_t  tmpLCR = USART->LCR;

        USART->LCR &= ~U_DLAB;

        if (Flags & USART_TXInt_EnFlag) USART->IER &= ~ETBEI;                                       //Disable transmitter interrupt
        if (Flags & USART_RXInt_EnFlag) USART->IER &= ~ERBFI;                                       //Disable receiver interrupt
        if (Flags & USART_ERRInt_EnFlag) USART->IER &= ~ELSI;                                       //Disable errors interrupt
        USART->LCR = tmpLCR; //-V519
        RestoreInterrupts(intflags);
    }
}

void USART_FlashRX(TUSART *USART)
{
    if ((USART != NULL) && (USART->FCR_RD & FIFOEN)) USART->FCR = CLRR | FIFOEN;
}

void USART_FlashTX(TUSART *USART)
{
    if ((USART != NULL) && (USART->FCR_RD & FIFOEN)) USART->FCR = CLRT | FIFOEN;
}

void USART_SendChar(TUSART *USART, char c)
{
    volatile TUSART *tmpUSART = USART;

    if (USART == NULL) return;

    while(!(tmpUSART->LSR & U_TEMT));
    tmpUSART->TXB = c;
}

void USART_SendString(TUSART *USART, char *Str)
{
    if (Str != NULL)
        while (*Str != '\0') USART_SendChar(USART, *Str++);
}

void USART_SendBufferPoll(TUSART_CONTEXT *USARTINF)
{
    uint8_t  *Data;

    if ((USARTINF == NULL) || (USARTINF->USART == NULL)) return;
    if (USARTINF->USART_TX_Buffer != NULL)
    {
        volatile TUSART *tmpUSART = USARTINF->USART;

        USART_DisableInterrupts(USARTINF->USART, USART_TXInt_EnFlag);                               //Disable USART TX interrupts

        Data = USARTINF->USART_TX_Buffer + USARTINF->USART_TX_Index;

        while(USARTINF->USART_TX_Index < USARTINF->USART_TX_Count)
        {
            while (!(tmpUSART->LSR & U_TEMT));
            USARTINF->USART->TXB = *Data++;
            USARTINF->USART_TX_Index++;
        }
        USARTINF->USART_TX_Index = 0;
        USARTINF->USART_TX_Count = 0;
    }
}

void USART_SendBufferInt(TUSART_CONTEXT *USARTINF)
{
    uint32_t Sent = 0;
    uint8_t  *Data;

    if ((USARTINF == NULL) || (USARTINF->USART == NULL)) return;
    if (USARTINF->USART_TX_Buffer != NULL)
    {
        USART_DisableInterrupts(USARTINF->USART, USART_TXInt_EnFlag);                               //Disable USART TX interrupts

        Data = USARTINF->USART_TX_Buffer + USARTINF->USART_TX_Index;

        while((USARTINF->USART->LSR & U_TEMT) &&
                (Sent < (USARTINF->USART_TX_Count - USARTINF->USART_TX_Index)))
        {
            USARTINF->USART->TXB = *Data++;
            USARTINF->USART_TX_Index++;
            Sent++;
        }
        USART_EnableInterrupts(USARTINF->USART, USART_TXInt_EnFlag);                                //Enable USART TX interrupts
    }
}
