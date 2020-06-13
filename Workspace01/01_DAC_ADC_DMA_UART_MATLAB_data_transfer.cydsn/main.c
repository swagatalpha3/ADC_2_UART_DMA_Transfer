/*
Author : Swagat Panda
Matrikelnummer: 766473
Project name :01_DAC_ADC_DMA_UART_MATLAB_data_transfer
version 3.0
*/

#include "project.h"
#include<stdio.h>
/* Defines for ADC and UART */
#define vdda 5.0
#define ADC_COUNT 65536           // 16 bit ADC
#define TX_BUFF_SIZE 1024         // 1024 * (uint16) data to be transfered
#define RX_BUFF_SIZE 1
#define ADC_BUFF_SIZE 1024
#define MATLAB_COUNT_VALUE 10
/* Defines for DMA_ADC */
#define DMA_ADC_BYTES_PER_BURST 2  //no of bytes to transfer per request
#define DMA_ADC_REQUEST_PER_BURST 1// request per burst 1- auto  0- manual
#define DMA_ADC_SRC_BASE (CYDEV_PERIPH_BASE)  // source address
#define DMA_ADC_DST_BASE (CYDEV_SRAM_BASE)    // destination address
/*global variable*/
char rx_buffer[RX_BUFF_SIZE];          
uint8_t rx_buffer_flag = 0;       //flag for teansmission request
uint8_t dma_xfer_done_flag = 0;   //flag for DMA transfer confirm
uint8_t data_received_flag =0;    //flag for data to MATLAB transfer confirm
uint8_t button_pressed_flag =0;   //flag for push button
uint16 tx_buffer[TX_BUFF_SIZE];
uint16 ADC_buffer[ADC_BUFF_SIZE]; //DMA destination in SRAM
uint8 count_value = 1;
/* Global Variable declarations for DMA_ADC */
uint8 DMA_ADC_Chan;
uint8 DMA_ADC_TD[1];

/*ALL functions prototype and defination*/

/* Interrupt function to be activcated ever time a Byte is received*/
void ISR_UART_Rx(void){
    ISR_UART_Rx_ClearPending();
    rx_buffer[0] = UART_GetByte();
    if (rx_buffer[0] == 's'){        // 's' sending request
       rx_buffer_flag = 1;           // 'o' received confirmation
        }
    else if (rx_buffer[0] =='o'){
      data_received_flag =1;
            }
    
    }
/* Interrupt function to be activcated when DMA transfer is complete*/
void ISR_DMA_XFER_DONE(void){
    ISR_DMA_XFER_DONE_ClearPending();
    dma_xfer_done_flag = 1;
}
/* Interrupt function to be activcated when a button is pressed*/
void ISR_BUTTON(void){
   isr_button_ClearPending();
   if (rx_buffer_flag == 1)
       button_pressed_flag =1;
   else 
      button_pressed_flag = 0; 
    }

/*green led pulsing function - default state*/
void GREEN_LED(void){
  if (button_pressed_flag == 0){
    led_green_Write(1);
    CyDelay(500);
    led_green_Write(0);
    CyDelay(500);
      }
    }
/*red led function - DMA completion state*/
void RED_LED(void){
  if ((dma_xfer_done_flag == 1)&&(rx_buffer_flag ==1))
    led_red_Write(1);
  else if (dma_xfer_done_flag == 0)
    led_red_Write(0); 

    }
/*yellow led pulsing function - system completion state*/
void YELLOW_LED(void){
  if (count_value == MATLAB_COUNT_VALUE+1){
        for(uint8 i=0;i<3;i++){
           led_yellow_Write(1);
           CyDelay(500);
           led_yellow_Write(0);
           CyDelay(500);
           }
 
        }
    }
/*counting function - number of times data received at MATLAB*/ 
void DATA_COUNTER(){
  if (count_value<=MATLAB_COUNT_VALUE)
      count_value++;
    }


int main(void)
{
    CyGlobalIntEnable; /* Enable global interrupts. */
    UART_Start();
    WaveDAC_Start();
    ADC_Start();
    ADC_StartConvert();
    uint16 adc_result;
    //float adc_to_vg;
    uint8 adc_to_tx_buf_flag = 0;
    
    /* Interrupt to be activcated ever time a Byte is received(FROM MATLAB)*/
    ISR_UART_Rx_StartEx(ISR_UART_Rx);
    /*Interrupt to be activcated ever time a DMA transfer is complete*/
    ISR_DMA_XFER_DONE_StartEx(ISR_DMA_XFER_DONE);
    /*button pressed interrupt*/
    isr_button_StartEx(ISR_BUTTON);
    
    /* DMA Configuration for DMA_ADC */
    DMA_ADC_Chan = DMA_ADC_DmaInitialize(DMA_ADC_BYTES_PER_BURST, 
                                         DMA_ADC_REQUEST_PER_BURST, 
                                         HI16(DMA_ADC_SRC_BASE),  // upper source address
                                         HI16(DMA_ADC_DST_BASE)   //upper destination address
    );
    DMA_ADC_TD[0] = CyDmaTdAllocate();
    CyDmaTdSetConfiguration(DMA_ADC_TD[0],          // current transport descriptor 
                                      2048,         // total no of bytes
                            CY_DMA_DISABLE_TD,      // terminates channel after TD finishes 
                            DMA_ADC__TD_TERMOUT_EN  // DMA complition event pulse eneble
                            | CY_DMA_TD_INC_DST_ADR // increament destination address after each burst
    ); 
    CyDmaTdSetAddress(DMA_ADC_TD[0],
                      LO16((uint32)ADC_DEC_SAMP_PTR), // lower source address
                      LO16((uint32)ADC_buffer)        // lower destination address
    );
    CyDmaChSetInitialTd(DMA_ADC_Chan, // DMA module handler
                        DMA_ADC_TD[0] // current transport descriptor 
    );
    
    
    
    for(;;)
       { 
        /* default state operation ( read ADC data and PLUSE green led in 500ms)*/
        
        ADC_IsEndConversion(ADC_WAIT_FOR_RESULT);
        adc_result = ADC_GetResult32();
        //adc_to_vg = (vdda/ADC_COUNT) * adc_result ;
        GREEN_LED();
        /****************************************************************
        Enables  the  DMA transfer when requested from MATLAB
        (i.e rx_buffer_flag is high)and  previous data(if any)
        has already transfered from UART  to  MATLAB     (i.e
        data_received_flag is low(reset))
        ***************************************************************/
        if ((rx_buffer_flag ==1)&& (data_received_flag ==0)){
             CyDmaChEnable(DMA_ADC_Chan,  // DMA module handler
                             1            // restore TD config after the end of the TD
              );
            }
        
        RED_LED();  // DMA transfer done
        /****************************************************************
        load ADC readings to tx buffer  when  requested  from 
        outside (i.e. rx_buffer_flag is high) and DMA transfer
        is(i.e dma_xfer_done_flag is high) complete  and  the 
        button is pressed(i.e button_pressed_flag is high)
        *****************************************************************/
        if((rx_buffer_flag ==1)  
            && (dma_xfer_done_flag == 1)  
            &&  (button_pressed_flag ==1)){
           for (int i =0; i<ADC_BUFF_SIZE ; i++){
              tx_buffer[i] = ADC_buffer[i]; 
             }
           adc_to_tx_buf_flag = 1;
           //dma_xfer_done_flag =0;
          }
        /****************************************************************
        transmit the data only when requested from outside 
        (i.e. rx_buffer_flag is high) and data loaded to tx buffer 
        (i.e adc_to_tx_buf_flag is high) 
        ***************************************************************/
            
        if((rx_buffer_flag ==1)&& (adc_to_tx_buf_flag ==1)){
                UART_PutArray(tx_buffer,TX_BUFF_SIZE * sizeof(uint16_t));//<---sending data
                adc_to_tx_buf_flag = 0;
                rx_buffer_flag = 0;
                /*Enables the DMA*/
                /*  CyDmaChEnable(DMA_ADC_Chan, // DMA module handler
                  1             // restore TD config after the end of the TD
                  );*/
            }
        
        if (data_received_flag ==1){
           DATA_COUNTER();
           dma_xfer_done_flag =0;
           data_received_flag =0;
           
           /*if (count_value <= MATLAB_COUNT_VALUE)
              ISR_button_Stop();*/
            }
        if (count_value > MATLAB_COUNT_VALUE){
            button_pressed_flag=0;
            rx_buffer_flag =0;
            dma_xfer_done_flag =0;
            data_received_flag=0;
            //ISR_button_StartEx(ISR_BUTTON);
            YELLOW_LED();
            count_value = 1;
            
            }
           
        }
       
}
     

/* [] END OF FILE */
