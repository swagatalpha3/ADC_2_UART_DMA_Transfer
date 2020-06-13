#include "project.h"
#include<stdio.h>
# define vdda 5.0
# define ADC_COUNT 65536  // 16 bit ADC
# define TX_BUFF_SIZE 256 // 256 * (uint16) data to be transfered

//char rx_buffer;          // software buffer
uint8_t rx_buffer_flag = 0;
uint16 tx_buffer[TX_BUFF_SIZE];


// Interrupt function to be activcated ever time a Byte is received
void ISR_UART_Rx(void){
    //rx_buffer = UART_GetByte();
    rx_buffer_flag = 1;
    
}

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
    
    
    for(;;)
       { 
        /* transmit the data only when requested from outside 
        (i.e. rx_buffer_flag is high) and data loaded to tx buffer 
        (i.e adc_to_tx_buf_flag is high) */
        
        if((rx_buffer_flag ==1)&& (adc_to_tx_buf_flag ==1)){
  
                UART_PutArray(tx_buffer,TX_BUFF_SIZE * sizeof(uint16_t));//<-----------------------------sending data
            
                adc_to_tx_buf_flag = 0;
                rx_buffer_flag = 0;
            }
        
       
        // load ADC readings to tx buffer when requested from outside (i.e. rx_buffer_flag is high)
        if (rx_buffer_flag ==1){
           for (int i =0; i<TX_BUFF_SIZE ; i++){
              ADC_IsEndConversion(ADC_WAIT_FOR_RESULT);
              tx_buffer[i] = ADC_GetResult32(); 
           }
           adc_to_tx_buf_flag = 1;
        }
      
        // default operation ( read ADC data and light the appropriate LED)
        ADC_StartConvert();
        ADC_IsEndConversion(ADC_WAIT_FOR_RESULT);
        adc_result = ADC_GetResult32();
        adc_to_vg = (vdda/ADC_COUNT) * adc_result ;
      /*  
        if (adc_to_vg < (0.088*vdda)){  // flash green LED if adc reads less than 8.8% of vdda 
            led_green_Write(1);
            CyDelay(1000);
            led_green_Write(0);
            CyDelay(1000);
            led_red_Write(0);
            led_yellow_Write(0);
        }
        if ((adc_to_vg >= (0.088*vdda)) && (adc_to_vg < (0.5*vdda))){
            led_green_Write(1);
            led_red_Write(0);
            led_yellow_Write(0);
        }
        if ((adc_to_vg >= (0.5*vdda)) && (adc_to_vg < (0.911*vdda))){
            led_green_Write(0);
            led_red_Write(0);
            led_yellow_Write(1);
        }
        if (adc_to_vg >= (0.911*vdda)){
            led_green_Write(0);
            led_red_Write(1);
            led_yellow_Write(0);
        }*/
        CyDelay(1);
       }
       
}
     

/* [] END OF FILE */
