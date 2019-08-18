/* GUI for the handheld edition of the emulator 
 *  Gui for File selection
 *  Gui for hardware configuration
 *  - Change keymaps
 *  - Load a snapshot
 *  - Reset the emulator
 *  - define hardware emulation
 *  - define keymaps
 *  - volver el emulador
 *  Gui confirmacion
 */
/* Borrar la parte de la pantalla a utilizar
 *  Escribir un titulo de la "ventana"
 *  varias opciones a elegir
 *  scroll si son mas que las que caben en pantalla
 *  mover una barra o algo entre las opciones
 *  retornar si/no y el resultado en funcion de la seleccion
 */

 void gui_draw_window(int w,int h, char *title){
  int x,y,n,m;
  x=(320-w)/2;
  y=(240-h)/2;
  n=x+w-2;
  m=y+8;
  tft.fillRect(x-2,y-2,w+4,h+4,tft.color565(64,64,64));
  tft.fillRect(x,  y,  w,  8,tft.color565(0,0,0));
  tft.fillRect(x+1,y+9,w-2,h-9,tft.color565(255,255,255));
  tft.drawRect(x,  y+8,w,  h-7,tft.color565(0,0,0));
//  tft.drawRect(x-1,y-1,w+2,h+2,tft.color565(64,64,64));
//  tft.drawRect(x-2,y-2,w+4,h+4,tft.color565(64,64,64));
//  tft.drawLine(n,y,(n--)-8,m,tft.color565(64,64,64));
//  tft.drawLine(n,y,(n--)-8,m,tft.color565(64,64,64));
  tft.setCursor(x+2, y+1);
  tft.setTextColor(ILI9341_WHITE);  
  tft.setTextSize(1);
  tft.println(title);
  tft.drawLine(n,y,(n--)-8,m,tft.color565(0,0,255));
  tft.drawLine(n,y,(n--)-8,m,tft.color565(0,0,255));
  tft.drawLine(n,y,(n--)-8,m,tft.color565(0,0,255));
  tft.drawLine(n,y,(n--)-8,m,tft.color565(0,0,255));
  tft.drawLine(n,y,(n--)-8,m,tft.color565(0,255,0));
  tft.drawLine(n,y,(n--)-8,m,tft.color565(0,255,0));
  tft.drawLine(n,y,(n--)-8,m,tft.color565(0,255,0));
  tft.drawLine(n,y,(n--)-8,m,tft.color565(0,255,0));
  tft.drawLine(n,y,(n--)-8,m,tft.color565(255,255,0));
  tft.drawLine(n,y,(n--)-8,m,tft.color565(255,255,0));
  tft.drawLine(n,y,(n--)-8,m,tft.color565(255,255,0));
  tft.drawLine(n,y,(n--)-8,m,tft.color565(255,255,0));
  tft.drawLine(n,y,(n--)-8,m,tft.color565(255,0,0));
  tft.drawLine(n,y,(n--)-8,m,tft.color565(255,0,0));
  tft.drawLine(n,y,(n--)-8,m,tft.color565(255,0,0));
  tft.drawLine(n,y,(n--)-8,m,tft.color565(255,0,0));
 
 }

void gui_update_menu(int w,int h,int n,int s, char *data[]){
  int c,x,y;
  x=(320-w)/2;
  y=(240-h)/2;
/*  int x,y,n,m;
  y=(240-h)/2;
  n=x+w-2;
  m=y+8;
*/
  tft.setCursor(x+2, y+1+9);
  tft.setTextColor(ILI9341_BLACK);  
  tft.setTextSize(1);
  for (c=0;c<n;c++){
   tft.println(data[n]);
  }
} 
int gui_draw_menu(int w, int n, char *title, char *data[]){
  int h;
  int s=0;
  if (n>6) {
    h=10+9*n;
  }
  else {
    h=10+9*6;     
  }
  gui_draw_window(w,h,title);
  gui_update_menu(w,h,n,s,data);
}

 
