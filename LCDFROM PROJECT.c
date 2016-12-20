
#include  <msp430xG43x.h>
#include "stdint.h"
#include "intrinsics.h"
#include "math.h"
#include  "stdio.h"
#include  "string.h"
#include "stdlib.h"

/////////////////////////////////////////



//unsigned char key = 0; // pressed key representation //

///////////////////////////////////
char read_keybord(void);
void delaym (long int tt);       // local delay//
void init_lcd (void);        // lcd initiation : 8-bit Data, clears screen , cursor home //
 void printm(unsigned char *string, unsigned char line,unsigned char cell );
                          //print message in given line from given cell//
 void clearlcd() ;                        
 void setcode(int code);  // to set the code to lcd
 void setdata(char a);  
  void printnum (unsigned int num, unsigned char line,unsigned int cell );


   

   
 /////////////////////////    DELAY    ////////////////////////////

void delaym (long int tt)
{
  int t;
  for (t=0; t < tt*10; t++); 
}

///////////////////////////////////setcode

void setcode(int code)
{
  P4DIR = 0x28; // make P4.4 and P4.6 as output
  P3DIR = 0xFF; // make P3 as output
  
  P4OUT=0X20;   //(0x28, P4.4=1  P4.6=1),  (0X08 P4.4=1 P4.6=0),  
                //(0X20 P4.4=0 P4.6=1) ,( P4.6=EN, P4.4=RS)
  P3OUT=code;
  delaym(5);
  P4OUT=0X00;
}
///////////////////clearscr
void clearlcd()
{
  setcode(0x01) ;
    delaym(40);
}
/////////////////////setdata
void setdata(char a)
{
  P4DIR = 0x28; // make P4.4 and P4.6 as output
  P3DIR = 0xFF; // make P3 as output
  
  P4OUT=0X28;   //(0x28, P4.4=1  P4.6=1),  (0X08 P4.4=1 P4.6=0),  
                //(0X20 P4.4=0 P4.6=1) ,( P4.6=EN, p6.4=RS)
  P3OUT=a;
  delaym(5);
  P4OUT=0X00;
}
////////////////////////     INIT LCD ////////////////////////////
   void init_lcd(void)

 {
    setcode(0x38); 
    delaym(10);
    setcode(0x0e);
    delaym(10);
    setcode(0x0c); //do not show cursor
    delaym(10);
    setcode(0x06);
    delaym(10);
    setcode(0x01) ;
    delaym(40);
 }

 ///////////////////////// PRINT message on LCD /////////////////////////////
   void printm(unsigned char *string, unsigned char line,unsigned char cell )
 {
    unsigned char i=0;
    switch (line)
   {  case(1): setcode(0x80|cell); break;
      case(2): setcode(0xC0|cell); break;
   }
 do
 {
   delaym(10);
   setdata(string[i]);
   i++;
 }
  while(string[i]);
  }

 
 

  

 ////////////////////////// PRINtnumber 3 digits max///////////////////////////
  
  void printnum (unsigned int num, unsigned char line,unsigned int cell )
{
unsigned char n1,n2,n3,n4;

 switch (line)
  {  case(1): setcode(0x80|cell); break;
     case(2): setcode(0xC0|cell); break;
  }
delaym(20);


n1=num/100;
n2=num%100;
n3=n2/10;
n4=n2%10;

 setdata(n1 + 0x30);
delaym(10);

 setdata(n3 + 0x30);
delaym(10);

 setdata(n4 + 0x30);
delaym(10);
  }



/////////////////  read keybord    //////////////////////////////////////// 
 
   char read_keybord(void)   
   { unsigned char key=0;
     int button1=0,button=0;
    int i;
    P5DIR=0xF0; // set 4 lsb of p5 as input and 4 msb as output.
    for(i=0x10; i<0x81;i*=2)
    {
      P5OUT=i; //scanning the rows of the keyboard with '1'
      key=0;
      if((P5IN &0xf ))   //if any key pressed, **no debounce needed with this device
      {    
        button1=P5IN&0x0F;
        button=i+button1;
        P5OUT=0;
      }
   if(button)
   {
    switch (button)
    {
      case(0x11) :key = 'D';  return key;
      case(0x12) :key = 'C';  return key;
      case(0x14) :key = 'B';  return key;
      case(0x18) :key = 'A';  return key;
      case(0x21) :key = '#';  return key;
      case(0x22) :key = '9';  return key;
      case(0x24) :key = '6';  return key;
      case(0x28) :key = '3';  return key; 
      case(0x41) :key = '0';  return key;
      case(0x42) :key = '8'; return key;
      case(0x44) :key ='5'; return key;
      case(0x48) :key ='2'; return key;
      case(0x81) :key ='*'; return key; 
      case(0x82) :key ='7'; return key;
      case(0x84) :key ='4'; return key;
      case(0x88) :key ='1'; return key;
    } 
   }
      
    }key=0;
      return key;
  
}


 ///////////////main
 void main()
 {    unsigned char k=0;
    WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
    init_lcd();
    printm("Hello Emil",1,6);
    printm("Press key to start",2,3);
    while(1)
    {  
      k=read_keybord();
      if(k)
      { 
          clearlcd();
          printm("you pressed ",1,3);
          printm(&k,2,10);
          printm("  ",2,11);
          k=0;
    
      }
    }
 }
 