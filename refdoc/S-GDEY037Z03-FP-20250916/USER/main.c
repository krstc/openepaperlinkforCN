#include "stm32f10x.h"
#include "delay.h"
#include "sys.h"
//EPD
#include "Display_EPD_W21_spi.h"
#include "Display_EPD_W21.h"
#include "Ap_29demo.h"	
//GUI
#include "GUI_Paint.h"
#include "fonts.h"

#if 0 
   unsigned char BlackImage[EPD_ARRAY];//Define canvas space  
#endif
//Tips//
/*
1.Flickering is normal when EPD is performing a full screen update to clear ghosting from the previous image so to ensure better clarity and legibility for the new image.
2.There will be no flicker when EPD performs a partial refresh.
3.Please make sue that EPD enters sleep mode when refresh is completed and always leave the sleep mode command. Otherwise, this may result in a reduced lifespan of EPD.
4.Please refrain from inserting EPD to the FPC socket or unplugging it when the MCU is being powered to prevent potential damage.)
5.Re-initialization is required for every full screen update.
6.When porting the program, set the BUSY pin to input mode and other pins to output mode.
*/

int	main(void)
{
	  unsigned char i;
		delay_init();	    	     //Delay function initialization
		NVIC_Configuration(); 	//Set NVIC interrupt grouping 2
    EPD_GPIO_Init();       //EPD GPIO  initialization
	while(1)
	{    
#if 1 //Full screen refresh, fast refresh, and partial refresh demostration.

		 /************Full display*******************/
			EPD_Init(); //Full screen refresh initialization.
			EPD_WhiteScreen_ALL(gImage_BW1,gImage_RW1); //To Display one image using full screen refresh.
			EPD_DeepSleep(); //Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
			delay_s(3); //Delay for 3s.
					
			/************Fast refresh mode(11s)*******************/
	#if 1	
		  EPD_Init_Fast(); //Fast refresh initialization.
			EPD_WhiteScreen_ALL(gImage_BW2,gImage_RW2); //To display one image using fast refresh.
			EPD_DeepSleep(); //Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
			delay_s(3); //Delay for 2s.
  #endif
	#if 1 //Partial refresh demostration.
	//Partial refresh demo support displaying a clock at 5 locations with 00:00.  If you need to perform partial refresh more than 5 locations, please use the feature of using partial refresh at the full screen demo.
	//After 5 partial refreshes, implement a full screen refresh to clear the ghosting caused by partial refreshes.
	//////////////////////Partial refresh time demo/////////////////////////////////////
			EPD_Init(); //Electronic paper initialization.	
			EPD_SetRAMValue_BaseMap(gImage_BWbasemap,gImage_RWbasemap); //Please do not delete the background color function, otherwise it will cause unstable display during partial refresh.	
			for(i=0;i<6;i++)
			{
        EPD_Dis_Part_Num2(16,280,Num[2],Num[i],2,32,64); //x,y,DATA-A~E,number,Resolution 32*64    
        EPD_Dis_Part_Num3(112,280,Num[5],Num[9-i],2,32,64); //x,y,DATA-A~E,number,Resolution 32*64        				
			}				
			EPD_DeepSleep();  //Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
			delay_s(3);	//Delay for 3s.
			EPD_Init(); //Full screen refresh initialization.
			EPD_WhiteScreen_White(); //Clear screen function.
			EPD_DeepSleep(); //Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
			delay_s(2); //Delay for 2s.
	#endif	
	
	#if 0 //Demonstration of full screen refresh with 180-degree rotation, to enable this feature, please change 0 to 1.
			/************Full display(2s)*******************/
			EPD_Init_180(); //Full screen refresh initialization.
			EPD_WhiteScreen_ALL(gImage_BW1,gImage_RW1); //To Display one image using full screen refresh.
			EPD_DeepSleep(); //Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
			delay_s(2); //Delay for 2s.
	#endif				
	
#endif

  while(1);	// The program stops here
	}
}	


