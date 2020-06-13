#include "project.h"
#include<stdio.h>
# define vdda 5.0
# define ADC_COUNT 65536  // 16 bit ADC
# define TX_BUFF_SIZE 1024 // 1024 * (uint16) data to be transfered
# define RX_BUFF_SIZE 1
# define ADC_BUFF_SIZE 1024 
/* Defines for DMA_ADC */
#define DMA_ADC_BYTES_PER_BURST 2   //no of bytes to transfer per request
#define DMA_ADC_REQUEST_PER_BURST 1 // request per burst 1- auto  0- manual
#define DMA_ADC_SRC_BASE (CYDEV_PERIPH_BASE)  // source address
#define DMA_ADC_DST_BASE (CYDEV_SRAM_BASE)    // destination address

//char rx_buffer[RX_BUFF_SIZE];          
uint8_t rx_buffer_flag = 0;
uint8_t dma_xfer_done_flag = 0;
uint16 tx_buffer[TX_BUFF_SIZE];
uint16 ADC_buffer[ADC_BUFF_SIZE];//DMA destination



/* Variable declarations for DMA_ADC */
/* Move these variable declarations to the top of the function */
uint8 DMA_ADC_Chan;
uint8 DMA_ADC_TD[1];


// Interrupt function to be activcated ever time a Byte is received
void ISR_UART_Rx(void){
    //rx_buffer = UART_GetByte();
    rx_buffer_flag = 1;
    
    }
// Interrupt function to be activcated when DMA transfer is complete
void ISR_DMA_XFER_DONE(void){
    dma_xfer_done_flag = 1;
};
int main(void)
{
    CyGlobalIntEnable; /* Enable global interrupts. */
    UART_Start();
    WaveDAC_Start();
    ADC_Start();
    ADC_StartConvert();
    uint16 adc_result;
    float adc_to_vg;
    uint8 adc_to_tx_buf_flag = 0;
    
    // Interrupt function to be activcated ever time a Byte is received
    ISR_UART_Rx_StartEx(ISR_UART_Rx);
    ISR_DMA_XFER_DONE_StartEx(ISR_DMA_XFER_DONE);
    
    /* DMA Configuration for DMA_ADC */
    DMA_ADC_Chan = DMA_ADC_DmaInitialize(DMA_ADC_BYTES_PER_BURST, 
                                         DMA_ADC_REQUEST_PER_BURST, 
                                         HI16(DMA_ADC_SRC_BASE),  // upper source address
                                         HI16(DMA_ADC_DST_BASE)   //upper destination address
    );
    DMA_ADC_TD[0] = CyDmaTdAllocate();
    CyDmaTdSetConfiguration(DMA_ADC_TD[0],          // current transport descriptor 
                                      2048,          // total no of bytes
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
    /*Enables the DMA*/
    CyDmaChEnable(DMA_ADC_Chan,      // DMA module handler
                             1       // restore TD config after the end of the TD
    );
    
    
    for(;;)
       { 
        /* transmit the data only when requested from outside 
        (i.e. rx_buffer_flag is high) and data loaded to tx buffer 
        (i.e adc_to_tx_buf_flag is high) */
        
        if((rx_buffer_flag ==1)&& (adc_to_tx_buf_flag ==1)){
  
                UART_PutArray(tx_buffer,TX_BUFF_SIZE * sizeof(uint16_t));//<-----------------------------sending data
                adc_to_tx_buf_flag = 0;
                rx_buffer_flag = 0;
                /*Enables the DMA*/
                  CyDmaChEnable(DMA_ADC_Chan, // DMA module handler
                  1             // restore TD config after the end of the TD
                  );
            }
        
       
        /* load ADC readings to tx buffer when requested from 
        outside (i.e. rx_buffer_flag is high) and DMA transfer
        is complete*/
       
        if ((rx_buffer_flag ==1) && (dma_xfer_done_flag == 1)){
           for (int i =0; i<ADC_BUFF_SIZE ; i++){
              tx_buffer[i] = ADC_buffer[i]; 
           }
           adc_to_tx_buf_flag = 1;
           dma_xfer_done_flag =0;
        }
      
        // default operation ( read ADC data and light the appropriate LED)
        ADC_StartConvert();
        ADC_IsEndConversion(ADC_WAIT_FOR_RESULT);
        adc_result = ADC_GetResult32();
        adc_to_vg = (vdda/ADC_COUNT) * adc_result ;
    
        CyDelay(1);
       }
       
}
     

/* [] END OF FILE */
