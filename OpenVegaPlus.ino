t  /*=====================================================================
 * Open Vega+ Emulator. Handheld ESP32 based hardware with 
 * ZX Spectrum emulation
 * 
 * (C) 2019 Alvaro Alea Fernandez
 *
 * This program is free software; redistributable under the terms of 
 * the GNU General Public License Version 2
 * 
 * Based on the Aspectrum emulator Copyright (c) 2000 Santiago Romero Iglesias
 * Use Adafruit's IL9341 and GFX libraries.
 *======================================================================
 */
#include "SPIFFS.h"
//#include <FS.h>
#include <driver/dac.h>
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

#define _cs 32
#define _dc 5
#define _rst 33
#define _mosi 23
#define _sclk 18
#define _miso 19

// Use hardware SPI 
Adafruit_ILI9341 tft = Adafruit_ILI9341(_cs, _dc, _rst);
//Adafruit_ILI9341 tft = Adafruit_ILI9341(_cs, _dc, _mosi, _sclk, _rst, _miso);

#include "hardware.h"
#include "snaps.h"
#include "spectrum.h"
#include "z80.h"

/* Definimos unos objetos
 *  spectrumZ80 es el procesador en si
 *  mem es la memoria ROM/RAM del ordenador
 *  hwopt es la configuracion de la maquina a emular esto se guarda en los snapshots
 *  emuopt es la configuracion del emulador en si, no se guarda con el snapshot
 */
Z80Regs spectrumZ80;
tipo_mem mem;
tipo_hwopt hwopt;
tipo_emuopt emuopt;
/*FIXME: para una buena orientacion a objetos, mem y hwopts deberian pasarse como puntero a
 *cada funcion que lo necesite, igual que hacemos con spectrumZ80
 */
 
void show_splash(void){

  tft.setCursor(0, 60);
  tft.setTextColor(ILI9341_WHITE);  
  tft.setTextSize(3);
  tft.println("    OpenVEGA+");
  tft.setTextColor(ILI9341_YELLOW);  
  tft.setTextSize(2);
  tft.println("\n     By Alvaro Alea F.");
  tft.setTextSize(1);
  tft.println("");
  tft.setTextSize(2);
  tft.println("          (C) 2019");
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_CYAN);  
  tft.println("\n\n       A Opensource Portable Spectrum Emulator");
}

void setup(void)
{
  Serial.begin(115200);
  AS_printf("OpenVega+ Boot!\n");
  //pinmode(25);
  //pinmode(26);
  dac_output_enable(DAC_CHANNEL_1);
  dac_output_enable(DAC_CHANNEL_2);
  dac_output_voltage(DAC_CHANNEL_1, 0);
  dac_output_voltage(DAC_CHANNEL_2, 0);
  keypad_i2c_init();  
  // Initialize SPIFFS
   if(!SPIFFS.begin(true)){
    AS_printf("An Error has occurred while mounting SPIFFS\n");
    return;
  }

  tft.begin();
  // read diagnostics (optional but can help debug problems)
  uint8_t x = tft.readcommand8(ILI9341_RDMODE);
  AS_printf("Display Power Mode: 0x"); AS_print(x, HEX);AS_printf("\n");
  x = tft.readcommand8(ILI9341_RDMADCTL);
  AS_printf("MADCTL Mode: 0x"); AS_print(x, HEX);AS_printf("\n");
  x = tft.readcommand8(ILI9341_RDPIXFMT);
  AS_printf("Pixel Format: 0x"); AS_print(x, HEX);AS_printf("\n");
  x = tft.readcommand8(ILI9341_RDIMGFMT);
  AS_printf("Image Format: 0x"); AS_print(x, HEX);AS_printf("\n");
  x = tft.readcommand8(ILI9341_RDSELFDIAG);
  AS_printf("Self Diagnostic: 0x"); AS_print(x, HEX); AS_printf("\n");
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);

  show_splash();
  //FIXME porque 69888??
  Z80Reset (&spectrumZ80, 69888);
  Z80FlagTables ();
  AS_printf("Z80 Initialization completed\n");
  
  init_spectrum(SPECMDL_48K,"/48.rom");
  reset_spectrum(&spectrumZ80);
  Load_SNA(&spectrumZ80,"/manic.sna");
//  Load_SNA(&spectrumZ80,"/t1.sna");
  
  delay (2000);
  AS_printf("Entrando en el loop\n");
}


void loop (void){ 
  draw_scanline();
}

const uint8_t SoundTable[4]={0,1,0,1};
void draw_scanline(){
  uint16_t c=0; 
  long ticks;
// dividimos el scanline en la parte central, y los bordes y retrazo, para la emulacion del puerto FF  
  for(ticks=0;ticks<256;ticks++) ula_tick();
  c=Z80Run (&spectrumZ80, 128) ;  
  for(ticks=0;ticks<192;ticks++) ula_tick();  
  c=Z80Run (&spectrumZ80, 96) ;
//  AS_printf("PC=%i\n",c);
  // Sound on each scanline means 15.6Khz, not bad...
  //dac_output_voltage(DAC_CHANNEL_1, SoundTable[hwopt.SoundBits]);
}

void ula_tick(void){
  const int specpal[48]={
    0,0,205,205, 0,0,205,212,     0,0,255,255, 0,0,255,255,
    0,0,0,0,     205,205,205,212, 0,0,0,0,     255,255,255,255,
    0,205,0,205, 0,205,0,212,     0,255,0,255, 0,255,0,255 };
  int color;
  byte pixel;
  int col,fil,scan,inkOpap;
  static int px=31;
  static int py=24;
  static int ch=-1;
  static int cv=-1;
  static int cf=0;
  static int cf2=0;
  static int cf3=0;
  static uint8_t attr;

 ch++;
 if (ch>=448) ch=0;
 if (ch==416) px=0;
 if ((ch<=288) or (ch>416)) px++;

 if (ch==0) { 
  cv++;
  if (cv>=313) {
    cv=0;
    leebotones();
  }
  if (cv==288) { 
    py=0;
    cf++;  if (cf>=32) cf=0;
  //    leebotones();
    cf2++; if (cf2>=50) cf2=0;
    cf3++; if (cf3>=4) cf3=0;
  }
  if ((cv<218) or (cv>288)) py++;
 }
 if ((ch==0) and (cv==248)) spectrumZ80.petint=1;


// Read the first ATTR of each col in advance.
/*
 if (cv=312 and ch==447) {
  attr=z80_peek(0x5800); 
 }
 if (cv<192 and ch==447) {
  fil=cv>>3;
  attr=z80_peek(0x5800 + 32*cv>>3  +0); 
 }
*/
 if ((cv>=192) or (ch>=256)) {
  hwopt.portFF = 0xFF;
  color = hwopt.BorderColor;
 } else {
  pixel=7-(ch & B111);
  col=(ch & B11111000)>>3;
  fil=cv>>3;
  scan = (cv & B11000000) + ((cv & B111)<<3) + ((cv & B111000)>>3) ;  
  //FIXME: Two acces to mem in same tick, really ATTR is Access a pixel early
  //       so a reading must be done when cv<192 and ch=447 for 0x5800 + 32*fil
  //       later attr for col+1 on ch & B111 == 0b111
  //       deberia ir en un if separado para no leer 33 valores, el ultimo no hace falta asi que ch<250
  if (( ch & B111 ) == 0b000 ) {
    attr=readvmem(0x1800 + 32*fil  +col,hwopt.videopage);
  }
  if (( ch & B111 ) == 0b000 ) {
    hwopt.portFF=readvmem(0x0000 + 32*scan +col,hwopt.videopage);
  }
  inkOpap=(hwopt.portFF>>pixel) & 1;
  // FLASH
  if  ( (cf>=16) && ((attr & B10000000)!=0) ) inkOpap=!inkOpap;
  // INK or PAPER
  if (inkOpap!=0) {
   color=attr & B000111;
  } else {
   color=(attr & B111000) >>3;
  }
  // BRIGHT
  if  ((attr & B01000000)!=0) {
   color=color+8;
  }
 }
 // Paint the real screen
 if ((px<320) and (py<240) and (cf3==0)) { 
  tft.drawPixel(px,py,tft.color565(specpal[color] ,specpal[color+16] ,specpal[color+32]));
 }
 // Frame indication
 if ((px==0) and (py==16)) {
  tft.fillRect(306,0,14,10,tft.color565(0,0,0));
  tft.setTextSize(1);
  tft.setCursor(307, 1);
  tft.setTextColor(ILI9341_WHITE);  
  tft.print(cf2);   
 }
}