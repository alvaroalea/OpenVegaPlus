/*=====================================================================
 * Open Vega+ Emulator. Handheld ESP32 based hardware with 
 * ZX Spectrum emulation
 * 
 * (C) 2019 Alvaro Alea Fernandez
 *
 * This program is free software; redistributable under the terms of 
 * the GNU General Public License Version 2
 * 
 * Based on the Aspectrum emulator Copyright (c) 2000 Santiago Romero Iglesias
 *======================================================================
 *
 * This file contains the specific ZX Spectrum part
 *
 *======================================================================
 */
#include <stdint.h>
#include <stdlib.h>
#include "spectrum.h"
#include "hardware.h"


extern tipo_mem mem;
extern tipo_hwopt hwopt;

uint8_t z80_peek(uint16_t dir){
  int page;
  uint8_t dato;
  page= ( dir & mem.mp ) >> mem.mr ;
  dato=*( mem.p + mem.ro[page] + (dir & mem.md)) ;
  return dato;
}

void z80_poke(uint16_t dir, uint8_t dato){
  int page;
  page= ( dir & mem.mp ) >> mem.mr ;
  *( mem.p + mem.wo[page] + (dir & mem.md))=dato ;
}

uint8_t readvmem(uint16_t offset, int page){
  return *( mem.p + mem.vo[page] + offset) ;
}

uint8_t z80_in(uint16_t  port){
uint8_t dato = 0xFF;

  if (!(port & 0x01))
    {
      if (!(port & 0x0100)) dato &= speckey[0]; //teclas shift,z-v
      if (!(port & 0x0200)) dato &= speckey[1]; //teclas a-g
      if (!(port & 0x0400)) dato &= speckey[2]; //teclas q-t
      if (!(port & 0x0800)) dato &= speckey[3]; //teclas 1-5
      if (!(port & 0x1000)) dato &= speckey[4]; //teclas 6-0
      if (!(port & 0x2000)) dato &= speckey[5]; //teclas y-p
      if (!(port & 0x4000)) dato &= speckey[6]; //teclas h-l,enter
      if (!(port & 0x8000)) dato &= speckey[7]; //teclas b-m,symb,space
    }
  // kempston Joystick, bit 5=0  
  if (!(port & 0x010)) dato &= kempston_port;

  // emulacion port FF
  if ((port & 0xFF) == 0xFF) {  
     if (!hwopt.emulate_FF)
       dato = 0xFF;
     else {
       dato=hwopt.portFF;
     }
  }
  return dato;
}

void z80_out(uint16_t port,uint8_t dato){
  if (!(port & 0x01)) {
    hwopt.BorderColor = (dato & 0x07);
    hwopt.SoundBits = (dato & 0x18)>>3;
  }  
}


/* por que estas rutinas y no paginar directamente sobre los arrais???
 * Supon la arquitectura del +2 y la del +2 con el pokeador automatico de MH
 * en el primero el tamaño de la pagina es de 16K mientras que en el segundo es de 2K
 * Con esto deberiamos poder aprobechar la rutina de paginacion del 128 en los dos casos
 * ya que esta rutina tiene en cuenta el tamaño de pagina con el que trabajamos y hace los
 * ajustes necesarios
 */
void pagein(int size, int block, int page, int ro, int issystem){
  int npag,cf,c,d;
  npag=size/mem.sp;
  d=block*npag;
  c=page*npag;
  cf=c+npag;
  for(;c<cf;c++,d+=mem.sp)
    if ( mem.ro[c]==mem.sro[c] ) { // no hay periferico
      if (issystem==SYSTEM_PAGE) {      // y pagino systema
        mem.ro[c]=d;
        mem.sro[c]=d;
        mem.wo[c]=(ro==RW_PAGE?d:mem.roo);
        mem.swo[c]=(ro==RW_PAGE?d:mem.roo);
        } else {        // y meto periferico
        mem.ro[c]=d;
        mem.wo[c]=(ro==RW_PAGE?d:mem.roo);
        }
     } else {       // si hay periferico
        if (issystem==SYSTEM_PAGE) {      // los cambios solo se notaran en el futuro.
           mem.sro[c]=d;
           mem.swo[c]=(ro==RW_PAGE?d:mem.roo);
        } else {      // y cambio de periferico.
           mem.ro[c]=d;
           mem.wo[c]=(ro==RW_PAGE?d:mem.roo);
        }
     }
        
}

void pageout(int size, int bloq, int page){
  int npag,c;
  npag=size/mem.sp;
  c=page*npag;
  mem.ro[c]=mem.sro[c];
  mem.wo[c]=mem.swo[c];
}


int load_48krom(char *filename){
  File f;
  f=SPIFFS.open(filename,FILE_READ);
  if (!f) {
    AS_printf("Algo fallo cargando 1 bloque de rom\n");
    return 1;
  }
  AS_printf("Cargando %s\tSIZE: %i\n", filename, f.size()); // 48.rom"); AS_printf("\tSIZE: "); AS_print(f.size()); AS_printf("\n");
  f.read(mem.p,f.size()); 
  f.close();
  return 0;
}

int load_128krom(char *filename){
  File f;
  f=SPIFFS.open(filename,FILE_READ);
  if (!f) {
    AS_printf("Algo fallo cargando 2 bloques de rom\n");
    return 1;
  }
  AS_printf("Cargando %s\tSIZE: %i\n", filename, f.size());
  //AS_printf("Cargando 48.rom"); AS_printf("\tSIZE: "); AS_print(f.size()); AS_printf("\n");
  f.read(mem.p,f.size()); 
  f.close();
  return 0;
}

int load_p3rom(char *filename){
  File f;
  f=SPIFFS.open(filename,FILE_READ);
  if (!f) {
    AS_printf("Algo fallo cargando 4 bloques de rom\n");
    return 1;
  } 
  AS_printf("Cargando %s\tSIZE: %i\n", filename, f.size());
  //AS_printf("Cargando 48.rom"); AS_printf("\tSIZE: "); AS_print(f.size()); AS_printf("\n");
  f.read(mem.p,f.size()); 
  f.close();
  return 0;
  }

int init_spectrum(int model,char *romfile){
  int ret;
  switch (model) {
    case SPECMDL_16K:
      ret=init_16k(romfile);
      break;
    case SPECMDL_48K:
      ret=init_48k(romfile);
      break;
    case SPECMDL_INVES:
      ret=init_inves(romfile);
      break;
    case SPECMDL_128K:
      ret=init_128k();
      break;
    case SPECMDL_PLUS2:
      ret=init_plus2();
      break;
    case SPECMDL_PLUS3:
      ret=init_plus3();
      break;
/*    case SPECMDL_48KIF1:
      ret=init_48k_if1(romfile);
      break;
*/    default:
      printf("MODELO NO SOPORTADO %x\n",model);
      ret=1;
      break;
    }
  return ret;
}

extern tipo_hwopt hwopt;

/* This do aditional stuff for reset, like mute sound chip, o reset bank switch */
int reset_spectrum(Z80Regs * regs){
  int ret=0;
  switch (hwopt.hw_model) {
    case SPECMDL_128K:
    case SPECMDL_PLUS2:
      ret=reset_128k();
      break;
    case SPECMDL_PLUS3:
      ret=reset_plus3();
      break;
  }
  Z80Reset (regs, 0);

  return ret;
}


int end_spectrum(void){
    free (mem.p); /* free RAM */
  return 0;
}



int init_48k(char *romfile){
  int i;
  printf(__FILE__": Init 48K hardware.\n");
  mem.md = 0x3FFF ;
  mem.mp = 0xC000 ;
  mem.mr = 14 ;
  mem.np = 4 ;
  mem.sp = 16*1024 ;
  mem.roo = 4*mem.sp ;
  mem.vn = 1 ;
  mem.ro[0] = mem.sro[0] = 0 ;
  mem.wo[0] = mem.swo[0] = mem.roo ;
  mem.ro[1] = mem.sro[1] = mem.wo[1] =mem.swo[1] = mem.sp;
  mem.ro[2] = mem.sro[2] = mem.wo[2] =mem.swo[2] = 2*mem.sp;
  mem.ro[3] = mem.sro[3] = mem.wo[3] =mem.swo[3] = 3*mem.sp;
  mem.vo[0] = 0x4000;
  
  mem.p=(uint8_t *)malloc((1+3+1)*mem.sp);
  if (mem.p==NULL){
    printf("NO MEMORY...\n");
    return 1;
  }
  // ULA config
  hwopt.emulate_FF=1;     
  hwopt.ts_lebo=24;       // left border t states
  hwopt.ts_grap=128;       // graphic zone t states
  hwopt.ts_ribo=24;       // right border t states
  hwopt.ts_hore=48;       // horizontal retrace t states
  hwopt.ts_line=224;       // to speed the calc, the sum of 4 abobe
  hwopt.line_poin=16;       // lines in retraze post interrup
  hwopt.line_upbo=48;       // lines of upper border
  hwopt.line_grap=192;       // lines of graphic zone = 192
  hwopt.line_bobo=48;       // lines of bottom border
  hwopt.line_retr=8;         // lines of the retrace
  hwopt.TSTATES_PER_LINE=224;
  hwopt.TOP_BORDER_LINES=64;
  hwopt.SCANLINES=192;
  hwopt.BOTTOM_BORDER_LINES=56;
  hwopt.tstate_border_left=24;
  hwopt.tstate_graphic_zone=128;
  hwopt.tstate_border_right=72;
  hwopt.hw_model=SPECMDL_48K;
  hwopt.int_type=NORMAL;
  hwopt.videopage=0;
  hwopt.SoundBits=1;     
  return load_48krom("/48.rom");
}

/* "Inves Spectrum +" is a (spanish clone) 48K, but:
 * without port FF. 
 * Int at start of screen (normal has int at0 start of border).
 * 16K RAM maped at 0x0000 as writeonly that modify INs function.
 * Custom ROM.
 */
int init_inves(char *romfile){
//  FILE *fp;
  int i;
  printf(__FILE__": Init Inves Spectrum+ hardware.\n");
  mem.md= 0x3FFF ;
  mem.mp= 0xC000 ;
  mem.mr= 14;
  mem.np=4;
  mem.sp=16*1024;
  mem.p=(uint8_t *)malloc((1+3+1)*mem.sp);
  if (mem.p==NULL){
    printf("NO MEMORY...\n");
    return 1;
  } 
  mem.roo=0x10000; //4*mem.sp;
  mem.vn=1;

  mem.ro[0]=mem.sro[0]=0;
  mem.wo[0]=mem.swo[0]=mem.roo;
  mem.ro[1]=mem.sro[1]=mem.wo[1]=mem.swo[1]=mem.sp;
  mem.ro[2]=mem.sro[2]=mem.wo[2]=mem.swo[2]=2*mem.sp;
  mem.ro[3]=mem.sro[3]=mem.wo[3]=mem.swo[3]=3*mem.sp;
  mem.vo[0]=0x4000;

  // ULA config
  hwopt.emulate_FF=0;     
  hwopt.ts_lebo=24;       // left border t states
  hwopt.ts_grap=128;       // graphic zone t states
  hwopt.ts_ribo=24;       // right border t states
  hwopt.ts_hore=48;       // horizontal retrace t states
  hwopt.ts_line=224;       // to speed the calc, the sum of 4 abobe
  hwopt.line_poin=16;       // lines in retraze post interrup
  hwopt.line_upbo=48;       // lines of upper border
  hwopt.line_grap=192;       // lines of graphic zone = 192
  hwopt.line_bobo=48;       // lines of bottom border
  hwopt.line_retr=8;         // lines of the retrace
  hwopt.TSTATES_PER_LINE=224;
  hwopt.TOP_BORDER_LINES=64;
  hwopt.SCANLINES=192;
  hwopt.BOTTOM_BORDER_LINES=56;
  hwopt.tstate_border_left=24;
  hwopt.tstate_graphic_zone=128;
  hwopt.tstate_border_right=72;
  hwopt.hw_model=SPECMDL_INVES;
  hwopt.int_type=INVES;
  hwopt.SoundBits=1;     
  hwopt.videopage=0;
  // cargar rom de 16K
  return load_48krom("/inves.rom");
  }
  
  
int init_16k(char *romfile){
//  FILE *fp;
  int i;
  printf(__FILE__": Init 16K hardware.\n");
  mem.md= 0x3FFF ;
  mem.mp= 0xC000 ;
  mem.mr= 14;
  mem.np=4;
  mem.sp=16*1024;
  mem.p=(uint8_t *)malloc((1+1+1)*mem.sp);
  if (mem.p==NULL){
    printf("NO MEMORY...\n");
    return 1;
  }
  mem.roo=2*mem.sp;
  mem.vn=1;

  mem.ro[0]=mem.sro[0]=0;
  mem.wo[0]=mem.swo[0]=mem.roo;
  mem.ro[1]=mem.sro[1]=mem.wo[1]=mem.swo[1]=mem.sp;
  mem.ro[2]=mem.sro[2]=mem.wo[2]=mem.swo[2]=mem.sp;
  mem.ro[3]=mem.sro[3]=mem.wo[3]=mem.swo[3]=mem.sp;
  mem.vo[0]=0x4000;

  // ULA config
  hwopt.emulate_FF=1;     
  hwopt.ts_lebo=24;       // left border t states
  hwopt.ts_grap=128;       // graphic zone t states
  hwopt.ts_ribo=24;       // right border t states
  hwopt.ts_hore=48;       // horizontal retrace t states
  hwopt.ts_line=224;       // to speed the calc, the sum of 4 abobe
  hwopt.line_poin=16;       // lines in retraze post interrup
  hwopt.line_upbo=48;       // lines of upper border
  hwopt.line_grap=192;       // lines of graphic zone = 192
  hwopt.line_bobo=48;       // lines of bottom border
  hwopt.line_retr=8;         // lines of the retrace
  hwopt.TSTATES_PER_LINE=224;
  hwopt.TOP_BORDER_LINES=64;
  hwopt.SCANLINES=192;
  hwopt.BOTTOM_BORDER_LINES=56;
  hwopt.tstate_border_left=24;
  hwopt.tstate_graphic_zone=128;
  hwopt.tstate_border_right=72;
  hwopt.hw_model=SPECMDL_16K;
  hwopt.int_type=NORMAL;
  hwopt.videopage=0;
  hwopt.SoundBits=1;     
  return load_48krom("/48.rom");
}

int init_128k(void){
//  FILE *fp;
  int i;
  printf(__FILE__": Init 128K hardware.\n");
  mem.md= 0x3FFF ;
  mem.mp= 0xC000 ;
  mem.mr= 14;
  mem.np=4;
  mem.sp=16*1024;
  mem.vn=2;
  mem.p=(uint8_t *)malloc((8+2+1)*mem.sp);
  //FIXME: si inicializamos el +2 gris carga 2 veces la ROM
  mem.roo=10*mem.sp;
  mem.ro[0]=mem.sro[0]=mem.sp*8;
  mem.wo[0]=mem.swo[0]=mem.roo;
  mem.ro[1]=mem.sro[1]=mem.wo[1]=mem.swo[1]=5*mem.sp;
  mem.ro[2]=mem.sro[2]=mem.wo[2]=mem.swo[2]=2*mem.sp;
  mem.ro[3]=mem.sro[3]=mem.wo[3]=mem.swo[3]=0*mem.sp;
  mem.vo[0]=5*mem.sp;
  mem.vo[1]=7*mem.sp;
  // ULA config
  // FIXME update de timing, also for the +3
  hwopt.emulate_FF=1;     
  hwopt.ts_lebo=24;       // left border t states
  hwopt.ts_grap=128;       // graphic zone t states
  hwopt.ts_ribo=24;       // right border t states
  hwopt.ts_hore=52;       // horizontal retrace t states
  hwopt.ts_line=228;       //to speed the calc, the sum of 4 abobe
  hwopt.line_poin=15;       // lines in retraze post interrup
  hwopt.line_upbo=48;       // lines of upper border
  hwopt.line_grap=192;       // lines of graphic zone = 192
  hwopt.line_bobo=48;       // lines of bottom border
  hwopt.line_retr=8;         // lines of the retrace
  hwopt.TSTATES_PER_LINE=228;
  hwopt.TOP_BORDER_LINES=64;
  hwopt.SCANLINES=192;
  hwopt.BOTTOM_BORDER_LINES=56;
  hwopt.tstate_border_left=24;
  hwopt.tstate_graphic_zone=128;
  hwopt.tstate_border_right=80;
  hwopt.hw_model=SPECMDL_128K;
  hwopt.int_type=NORMAL;
  hwopt.videopage=0;
  hwopt.BANKM=0x00;
  hwopt.SoundBits=1;     
  return load_128krom("/derbi.rom");
;
}

int reset_128k(void){
   hwopt.BANKM=0x00; /* need to clear lock latch or next dont work. */
   outbankm_128k(0x00);
   return 0;
}

void outbankm_128k(uint8_t dato){
 
   if ((hwopt.BANKM | 0xDF) == 0xFF) return; /* El bloqueo en el bit 5 */
   
   hwopt.BANKM=dato;
   hwopt.videopage= (dato & 0x08) >> 3 ;
   mem.ro[0]=mem.sro[0]=mem.sp*(8 + ((dato & 0x10) >> 4) );
//  pagein(0x4000,0,(dato & 0x10) >> 4,RO_PAGE,SYSTEM_PAGE);
   // definitivamente la pagina 5 se queda quieta y es la ula la que accede a la 7 o la 5.
   // mem.ro[1]=mem.sro[1]=mem.wo[1]=mem.swo[1]=( (dato & 0x08)==0x08 ? 7 : 5 ) *mem.sp;
   mem.ro[3]=mem.sro[3]=mem.wo[3]=mem.swo[3]=(dato & 0x07)*mem.sp;
//  pagein(0x4000,3,dato & 0x07,RW_PAGE,SYSTEM_PAGE);

} 

int init_plus2(void){
  init_128k();
  AS_printf(__FILE__": Init +2 hardware.\n");
  hwopt.hw_model=SPECMDL_PLUS2;
  return load_128krom("/p2.rom");
}

int init_plus3(void){
//  FILE *fp;
  int i;
  printf(__FILE__": Init +3 hardware.\n");
  mem.md= 0x3FFF ;
  mem.mp= 0xC000 ;
  mem.mr= 14;
  mem.np=4;
  mem.sp=16*1024;
  mem.vn=2;
  mem.p=(uint8_t *)malloc((8+4+1)*mem.sp);
  mem.roo=12*mem.sp; 
  mem.ro[0]=mem.sro[0]=mem.sp*8;
  mem.wo[0]=mem.swo[0]=mem.roo;
  mem.ro[1]=mem.sro[1]=mem.wo[1]=mem.swo[1]=5*mem.sp;
  mem.ro[2]=mem.sro[2]=mem.wo[2]=mem.swo[2]=2*mem.sp;
  mem.ro[3]=mem.sro[3]=mem.wo[3]=mem.swo[3]=0*mem.sp;
  mem.vo[0]=5*mem.sp;
  mem.vo[1]=7*mem.sp;
  // ULA config
  hwopt.emulate_FF=0;     
  hwopt.ts_lebo=24;       // left border t states
  hwopt.ts_grap=128;       // graphic zone t states
  hwopt.ts_ribo=24;       // right border t states
  hwopt.ts_hore=52;       // horizontal retrace t states
  hwopt.ts_line=228;       // to speed the calc, the sum of 4 abobe
  hwopt.line_poin=15;       // lines in retraze post interrup
  hwopt.line_upbo=48;       // lines of upper border
  hwopt.line_grap=192;       // lines of graphic zone = 192
  hwopt.line_bobo=48;       // lines of bottom border
  hwopt.line_retr=8;         // lines of the retrace
  hwopt.TSTATES_PER_LINE=228;
  hwopt.TOP_BORDER_LINES=64;
  hwopt.SCANLINES=192;
  hwopt.BOTTOM_BORDER_LINES=56;
  hwopt.tstate_border_left=24;
  hwopt.tstate_graphic_zone=128;
  hwopt.tstate_border_right=72;
  hwopt.hw_model=SPECMDL_PLUS3;
  hwopt.int_type=NORMAL;
  hwopt.videopage=0;
  hwopt.BANKM=0x00;
  hwopt.BANK678=0x00;
  hwopt.SoundBits=1;     
  return   load_p3rom("/p3.rom");
;

}

void outbankm_p31(uint8_t dato){ // the 0x1FFD latch
int roms;
   hwopt.BANK678=dato;
   if ((dato & 0x01) == 0x01) {
    printf("Trying to page 64K of ram");
  roms=(dato & 0x06) >> 1;
  switch (roms) { 
    case 0:
            mem.ro[0]=mem.sro[0]=mem.wo[0]=mem.swo[0]=0*mem.sp;
      mem.ro[1]=mem.sro[1]=mem.wo[1]=mem.swo[1]=1*mem.sp;
            mem.ro[2]=mem.sro[2]=mem.wo[2]=mem.swo[2]=2*mem.sp;
            mem.ro[3]=mem.sro[3]=mem.wo[3]=mem.swo[3]=3*mem.sp;
      break;
    case 1: 
            mem.ro[0]=mem.sro[0]=mem.wo[0]=mem.swo[0]=4*mem.sp;
      mem.ro[1]=mem.sro[1]=mem.wo[1]=mem.swo[1]=5*mem.sp;
            mem.ro[2]=mem.sro[2]=mem.wo[2]=mem.swo[2]=6*mem.sp;
            mem.ro[3]=mem.sro[3]=mem.wo[3]=mem.swo[3]=7*mem.sp;
      break;
    case 2:
            mem.ro[0]=mem.sro[0]=mem.wo[0]=mem.swo[0]=4*mem.sp;
      mem.ro[1]=mem.sro[1]=mem.wo[1]=mem.swo[1]=5*mem.sp;
            mem.ro[2]=mem.sro[2]=mem.wo[2]=mem.swo[2]=6*mem.sp;
            mem.ro[3]=mem.sro[3]=mem.wo[3]=mem.swo[3]=3*mem.sp;
      break;
    case 3:
            mem.ro[0]=mem.sro[0]=mem.wo[0]=mem.swo[0]=4*mem.sp;
      mem.ro[1]=mem.sro[1]=mem.wo[1]=mem.swo[1]=7*mem.sp;
            mem.ro[2]=mem.sro[2]=mem.wo[2]=mem.swo[2]=3*mem.sp;
            mem.ro[3]=mem.sro[3]=mem.wo[3]=mem.swo[3]=6*mem.sp;
      break;
  }
   } else {
        /* if we pass from 64K ram to normal mode, need to ensure that all the
   * pages go to where need, rom, ram, videoram.         */
  mem.ro[1]=mem.sro[1]=mem.wo[1]=mem.swo[1]=5*mem.sp;
        mem.ro[2]=mem.sro[2]=mem.wo[2]=mem.swo[2]=2*mem.sp;
        outbankm_p37(hwopt.BANKM);
/*    
//       printf("Paging in BANK678=%x ",dato);
       roms= 8 + ((hwopt.BANKM & 0x10) >> 4) + ((dato & 0x04) >>1);
//       printf("Rom is %x\n",roms-8);
       mem.ro[0]=mem.sro[0]=mem.sp * roms;
       mem.wo[0]=mem.swo[0]=mem.roo;
*/
       }
}

void outbankm_p37(uint8_t dato){ // the 0x7FFD latch
int roms;  
//   printf("Paging in BANKM=%x ",dato);
   if ((hwopt.BANKM | 0xDF) == 0xFF) return; /* El bloqueo en el bit 5 */
   
   hwopt.BANKM=dato;
   hwopt.videopage= (dato & 0x08) >> 3 ; /* videoram switch even on 64K ram??? */
   if ((hwopt.BANK678 & 0x01) == 0x01) return; /* 64K ram, no more to do */ 
   roms= 8 + ((dato & 0x10) >> 4) + ((hwopt.BANK678 & 0x04) >>1);
  // printf("Rom is %x\n",roms-8);
   mem.ro[0]=mem.sro[0]=mem.sp * roms;
   mem.wo[0]=mem.swo[0]=mem.roo;
//  pagein(0x4000,0,(dato & 0x10) >> 4,RO_PAGE,SYSTEM_PAGE);
   mem.ro[3]=mem.sro[3]=mem.wo[3]=mem.swo[3]=(dato & 0x07)*mem.sp;
//  pagein(0x4000,3,dato & 0x07,RW_PAGE,SYSTEM_PAGE);
}

int reset_plus3(void){
        hwopt.BANKM=0x00; // clear lock latch or outbankm_p31 don't work
        outbankm_p31(0x00);
        outbankm_p37(0x00);
  return 0;
}
