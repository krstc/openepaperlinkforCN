#include "EPD_3in98g.h"

/******************************************************************************
function :	Software reset
parameter:
******************************************************************************/
static void EPD_3IN98G_Reset(void)
{
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(20);
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(2);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(20);
}

/******************************************************************************
function :	send command
parameter:
     Reg : Command register
******************************************************************************/
static void EPD_3IN98G_SendCommand(UBYTE Reg)
{
    DEV_Digital_Write(EPD_DC_PIN, 0);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_WriteByte(Reg);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	send data
parameter:
    Data : Write data
******************************************************************************/
static void EPD_3IN98G_SendData(UBYTE Data)
{
    DEV_Digital_Write(EPD_DC_PIN, 1);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_WriteByte(Data);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	Wait until the busy_pin goes LOW
parameter:
******************************************************************************/
static void EPD_3IN98G_ReadBusyH(void)
{
    Debug("e-Paper busy H\r\n");
    while(!DEV_Digital_Read(EPD_BUSY_PIN)) {
        DEV_Delay_ms(5);
    }
    Debug("e-Paper busy H release\r\n");
}

/******************************************************************************
function :	Turn On Display
parameter:
******************************************************************************/
static void EPD_3IN98G_TurnOnDisplay(void)
{
    EPD_3IN98G_SendCommand(0x12);
    EPD_3IN98G_SendData(0x00);
    EPD_3IN98G_ReadBusyH();

    EPD_3IN98G_SendCommand(0x02);
    EPD_3IN98G_SendData(0X00);
    EPD_3IN98G_ReadBusyH();
}

/******************************************************************************
function :	Initialize the e-Paper register
parameter:
******************************************************************************/
void EPD_3IN98G_Init(void)
{
    EPD_3IN98G_Reset();
    EPD_3IN98G_ReadBusyH();
    DEV_Delay_ms(30);

    EPD_3IN98G_SendCommand(0xAA);
    EPD_3IN98G_SendData(0x49);
    EPD_3IN98G_SendData(0x55);
    EPD_3IN98G_SendData(0x20);
    EPD_3IN98G_SendData(0x08);
    EPD_3IN98G_SendData(0x09);
    EPD_3IN98G_SendData(0x18);

    EPD_3IN98G_SendCommand(0x01);
    EPD_3IN98G_SendData(0x3F);

    EPD_3IN98G_SendCommand(0x00);
    EPD_3IN98G_SendData(0x4B);
    EPD_3IN98G_SendData(0x69);

    EPD_3IN98G_SendCommand(0x05);
    EPD_3IN98G_SendData(0x40);
    EPD_3IN98G_SendData(0x1F);
    EPD_3IN98G_SendData(0x1F);
    EPD_3IN98G_SendData(0x2C);

    EPD_3IN98G_SendCommand(0x08);
    EPD_3IN98G_SendData(0x6F);
    EPD_3IN98G_SendData(0x1F);
    EPD_3IN98G_SendData(0x1F);
    EPD_3IN98G_SendData(0x22);

    EPD_3IN98G_SendCommand(0x06);
    EPD_3IN98G_SendData(0x6F);
    EPD_3IN98G_SendData(0x1F);
    EPD_3IN98G_SendData(0x14);
    EPD_3IN98G_SendData(0x14);

    EPD_3IN98G_SendCommand(0x03);
    EPD_3IN98G_SendData(0x00);
    EPD_3IN98G_SendData(0x54);
    EPD_3IN98G_SendData(0x00);
    EPD_3IN98G_SendData(0x44);

    EPD_3IN98G_SendCommand(0x60);
    EPD_3IN98G_SendData(0x02);
    EPD_3IN98G_SendData(0x00);
    
    EPD_3IN98G_SendCommand(0x30);
    EPD_3IN98G_SendData(0x08);

    EPD_3IN98G_SendCommand(0x50);
    EPD_3IN98G_SendData(0x3F);

    EPD_3IN98G_SendCommand(0x61);
    EPD_3IN98G_SendData(0x03);
    EPD_3IN98G_SendData(0x00);
    EPD_3IN98G_SendData(0x02); 
    EPD_3IN98G_SendData(0x28); 

    EPD_3IN98G_SendCommand(0x65);
    EPD_3IN98G_SendData(0x10);
    EPD_3IN98G_SendData(0x00);
    EPD_3IN98G_SendData(0x20);
    EPD_3IN98G_SendData(0x00); 

    EPD_3IN98G_SendCommand(0xE3);
    EPD_3IN98G_SendData(0x2F);

    EPD_3IN98G_SendCommand(0x84);
    EPD_3IN98G_SendData(0x01);
}

/******************************************************************************
function :	Clear screen
parameter:
******************************************************************************/
void EPD_3IN98G_Clear(UBYTE color)
{
    UWORD Width, Height;
    Width = (EPD_3IN98G_WIDTH % 4 == 0)? (EPD_3IN98G_WIDTH / 4 ): (EPD_3IN98G_WIDTH / 4 + 1);
    Height = EPD_3IN98G_HEIGHT;
    
    EPD_3IN98G_SendCommand(0x04);
    EPD_3IN98G_ReadBusyH();

    EPD_3IN98G_SendCommand(0x10);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            EPD_3IN98G_SendData((color << 6) | (color << 4) | (color << 2) | color);
        }
    }
    EPD_3IN98G_TurnOnDisplay();
}

/******************************************************************************
function :	Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
void EPD_3IN98G_Display(const UBYTE *Image)
{
    UWORD Width, Height;
    Width = (EPD_3IN98G_WIDTH % 4 == 0)? (EPD_3IN98G_WIDTH / 4 ): (EPD_3IN98G_WIDTH / 4 + 1);
    Height = EPD_3IN98G_HEIGHT;
    
    EPD_3IN98G_SendCommand(0x04);
    EPD_3IN98G_ReadBusyH();

    EPD_3IN98G_SendCommand(0x10);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            EPD_3IN98G_SendData(Image[i + j * Width]);
        }
    }
    EPD_3IN98G_TurnOnDisplay();
}

/******************************************************************************
function :	Enter sleep mode
parameter:
******************************************************************************/
void EPD_3IN98G_Sleep(void)
{
    EPD_3IN98G_SendCommand(0x02);
    EPD_3IN98G_SendData(0X00);
    EPD_3IN98G_SendCommand(0x07);
    EPD_3IN98G_SendData(0XA5);
}