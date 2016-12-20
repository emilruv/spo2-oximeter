
#include  <msp430xG43x.h>
#include "stdint.h"
#include "intrinsics.h"
#include "math.h"
#include  "stdio.h"
#include  "string.h"
#include "stdlib.h"

/////////////////////////////////////////

// SaO2 Look-up Table
const unsigned int Lookup [43] = {100,100,100,100,99,99,99,99,99,99,98,98,98,98,
                                  98,97,97,97,97,97,97,96,96,96,96,96,96,95,95,
                                  95,95,95,95,94,94,94,94,94,93,93,93,93,93};


// LED Target Range 
#define FIRST_STAGE_TARGET_HIGH         3500
#define FIRST_STAGE_TARGET_LOW          3000
#define FIRST_STAGE_TARGET_HIGH_FINE    4096
#define FIRST_STAGE_TARGET_LOW_FINE     2700
#define FIRST_STAGE_STEP                5
#define FIRST_STAGE_FINE_STEP           1




int ir_dc_offset = 2000;
int vs_dc_offset = 2000;
int ir_LED_level;
int vs_LED_level;
int ir_sample;
int vs_sample;
char is_IR; 
int ir_heart_signal;
int vs_heart_signal;
int ir_heart_ac_signal;
int vs_heart_ac_signal;
unsigned int rms_ir_heart_ac_signal;
unsigned int rms_vs_heart_ac_signal;
int32_t ir_2nd_dc_register = 0;
int32_t vs_2nd_dc_register = 0;
unsigned long log_sq_ir_heart_ac_signal;
unsigned long log_sq_vs_heart_ac_signal;
unsigned long sq_ir_heart_ac_signal;
unsigned long sq_vs_heart_ac_signal;
unsigned int pos_edge = 0;
unsigned int edge_debounce;
unsigned int heart_beat_counter;
unsigned int log_heart_signal_sample_counter;
unsigned int heart_signal_sample_counter;

volatile unsigned int j;

/* The results */
unsigned int heart_rate;
unsigned int heart_rate_LSB = 0;
unsigned int SaO2, Ratio;
unsigned int SaO2_LSB = 0;
int count=0; //for finger in/out recognition
int countaverage=0;// for heart rate average calculation
unsigned int heart_rate_average=0;//for heart rate average calculation




///////////////////////////////////functions

void delaym (long int tt);       // local delay//
void init_lcd (void);        // lcd initiation : 8-bit Data, clears screen , cursor home //
 void printm(unsigned char *string, unsigned char line,unsigned char cell );
                          //print message in given line from given cell//
 void clearlcd() ;  
 char read_keybord(void) ;
 void setcode(int code);  // to set the code to lcd
 void setdata(char a);  
  void printnum (unsigned int num, unsigned char line,unsigned int cell );
void display_pulse(int on);
int16_t dc_estimator(register int32_t *p, register int16_t x);
int16_t ir_filter(int16_t sample);
int16_t vs_filter(int16_t sample);
void menuroutine(void);   
int menu2(void);
int menu1(void);

void mainroutine(void);

///////////////////////////////////////////////
// UART Transmission Structure Definition
enum scope_type_e
{
    SCOPE_TYPE_OFF = 0,   //will not transmit online to labview
    SCOPE_TYPE_HEART_SIGNALS, // by default
    SCOPE_TYPE_RAW_SIGNALS,
    SCOPE_TYPE_LED_DRIVE,
};
int scope_type = SCOPE_TYPE_HEART_SIGNALS;// now by default will transmit online data
//////////////////////////////////////////////
   
 /////////////////////////    DELAY    ////////////////////////////

void delaym (long int tt)
{
  int t;
  for (t=0; t < tt*10; t++); 
   

}

///////////////////////////////////setcode
//set codewords to operate the LCD
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
//sets data to show on the LCD
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
 //returns pressed key
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


//////////////////////////////////menu1
int menu1(void)
{
  unsigned char c,k=0;
 
  clearlcd();
  delaym(3000);
  while(1)
  {
  printm("1: Online reading  ",1,0);
  printm("2: About project      B",2,0);
  c=62;
  printm(&c,2,23);
  delaym(3000);
  k=read_keybord();
      if(k)
   {
    switch (k)
    {
      case('B') :
          
          menu2();
        
      case('1') :
        {
          delaym(3000);
          k=0;
          clearlcd();
          mainroutine();
        }
      case('2'): 
        {
          clearlcd();
          while(1)
          {
          printm("Made by",1,7);
          printm("Emil & Tslil      A",2,5);
          c=60;
          printm(&c,2,22);
          k=read_keybord();
          delaym(3000);
          if(k=='A')
            break;
          }
         
        }
    }
}
  }
}

///////////////////////////////////menuroutine

void menuroutine(void)
{
  unsigned char k=0;
 
  
   clearlcd();
   while(1)
   {
          printm("   WELCOME! Press",1,3);
          printm(" the 'A' key for menu",2,2);
          delaym(3000);
          k=0;
          k=read_keybord();
          delaym(3000);
      if(k=='A')
      { 
        menu1();
       }
      } 

   }
  


////////////////////////////////////////////////+++++++++++++++++++++++//////////////

int32_t mul16(register int16_t x, register int16_t y);
                                           
//FIR filter coefficient for removing 50/60Hz and 100/120Hz from the signals
#if 0 
static const int16_t coeffs[9] =
{
    5225,
    5175,
    7255,
    9453,
    11595,
    13507,
    15016,
    15983,
    16315
};
#else
static const int16_t coeffs[12] =
{
    688,
    1283,
    2316,
    3709,
    5439,
    7431,
    9561,
    11666,
    13563,
    15074,
    16047,
    16384
};
#endif


 


/////////////////////////////////////////menu2

int menu2(void)
{
  unsigned char c,k=0;
  char ip[]="52.192.872.32";
  unsigned int s_LSB,s=0,i;
  clearlcd();
  delaym(3000);
  while(1)
  {
    k=0;
  if (scope_type==SCOPE_TYPE_HEART_SIGNALS)
    printm("3: Stop RS232  ",1,0);
  else
    printm("3: Start RS232  ",1,0);
  
  printm("4: Wifi SSID        <AB>",2,0);
  
  k=read_keybord();
  delaym(3000);
      if(k)
   {
    switch (k)
    {
      case('A') : 
        {
          delaym(3000);
          
          menu1();
        }
      case('3') :
        { 
          if (scope_type==SCOPE_TYPE_HEART_SIGNALS)
          {
            scope_type=SCOPE_TYPE_OFF;   //will turn off the RS232 tranmition
            k=0;
            delaym(3000);
            menu2();
          }
          else
          {
            scope_type=SCOPE_TYPE_HEART_SIGNALS;  //will turn on the RS232 tranmition
            k=0;
            delaym(3000);
            menu2();
          }
        }
      case(('4')): 
        {
          /*clearlcd();
          while(1)
          {
          printm("This option is",1,2); 
          printm("under development      A",2,0);
          c=60;
          printm(&c,2,22);
          k=read_keybord();
          if(k=='A')
          {
            k=0;
            delaym(3000);
            menu2();
          }
          }*/
          s='a';
          s_LSB = s & 0x00FF;
          //delaym(3000);
          c=0;
          TXBUF0 = 'q';         // Byte 4 - Heart rate data
          while (!(IFG1 & UTXIFG0));
          while(c!='4')
          {
            
           c=read_keybord(); 
           delaym(2000);
           delaym(2000);
           if((c!='2')&&(c)&&(c!='A'))
           {
             
            for(i=0;i<strlen(ip);i++)
            {
            TXBUF0 = ip[i];         // Byte 4 - Heart rate data
            while (!(IFG1 & UTXIFG0));
            }
           }
           if(c=='2')
           {
             TXBUF0 = 'q';         // Byte 4 - Heart rate data
             while (!(IFG1 & UTXIFG0));
             break;
           }
           if(c=='A')
           {
             TXBUF0 = 52;         // Byte 4 - Heart rate data
             while (!(IFG1 & UTXIFG0));
           }
                   
          }
        }
          
    }
   }
  }
}

///////////////////////////mainroutine

void mainroutine(void)
{
    double f1;
    int i;
    int32_t x;
    int32_t y;
  delaym(3000);
clearlcd();  
  while(1) 
    {
        __bis_SR_register(LPM0_bits + GIE); 
        
        
        /* Heart Rate Computation */
        f1 = 60.0*512.0*3.0/(float)log_heart_signal_sample_counter;
        heart_rate = (unsigned int)f1;
        for(i=0;countaverage<50;i++)
        {
          heart_rate_average+=heart_rate;
          countaverage++;
        }
        if(countaverage==50)
        {
          printm("Heart rate is:    bpm",1,0);
          printnum((heart_rate_average/50),1,15);
          heart_rate_average=0;
          countaverage=0;
          i=0;
        }
        
        heart_rate_LSB = heart_rate & 0x00FF;
        
        /* SaO2 Computation */
        x = log_sq_ir_heart_ac_signal/log_heart_signal_sample_counter;
        y = log_sq_vs_heart_ac_signal/log_heart_signal_sample_counter;
        Ratio = (unsigned int) (100.0*log(y)/log(x));
        if (Ratio > 66)
          SaO2 = Lookup[Ratio - 66];        // Ratio - 50 (Look-up Table Offset) - 16 (Ratio offset)
        else if (Ratio > 50)
          SaO2 = Lookup[Ratio - 50];        // Ratio - 50 (Look-up Table Offset)
        else
          //SaO2 = 100;
          SaO2 = 99;
          
          printm("SaO2 ratio is:     %",2,0);
          printnum(SaO2,2,15);
          SaO2_LSB = SaO2 & 0x00FF;
    }
}
///////////////////////////




// Timer A0 interrupt service routine
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A0(void)
{
    int i;
    unsigned char k=0; //added1
    k=read_keybord();
      if(k=='A')
      {
        delaym(3000);
        menuroutine();
        k=0;
        delaym(3000);
      }
       
         
    //added1 end
    if ((DAC12_0CTL & DAC12OPS))            // D2 enabled in demo board
    {
                                            // Immediately enable the visible
                                            // LED, to allow time for the
                                            // transimpedance amp to settle
        DAC12_0CTL &= ~DAC12ENC;
        P2OUT &= ~BIT3;                     // turn on source for D3
        DAC12_0CTL &= ~DAC12OPS;            // Disable IR LED, enable visible LED
        DAC12_0CTL |= DAC12ENC;
        DAC12_0DAT = vs_LED_level;
        DAC12_1DAT = vs_dc_offset;          // Load op-amp offset value for visible
        P2OUT |= BIT2;                      // turn off source for D2        
        
        is_IR = 0;                          // IR LED OFF
        
        ir_sample = ADC12MEM0;              // Read the IR LED results 
        i = ADC12MEM1; 
                                            // Enable the next conversion sequence. 
                                            // The sequence is started by TA1 
        ADC12CTL0 &= ~ENC;
        ADC12CTL0 |= ENC;  
        
                                            // Filter away 50/60Hz electrical pickup, 
                                            // and 100/120Hz room lighting optical pickup 
        ir_heart_signal = ir_filter(i);
                                            // Filter away the large DC
                                            // component from the sensor */
        ir_heart_ac_signal = ir_heart_signal - dc_estimator(&ir_2nd_dc_register, ir_heart_signal);  
        
        /* Bring the IR signal into range through the second opamp */    
        if (i >= 4095)
        {
            if (ir_dc_offset > 100)
                ir_dc_offset--;
        }
        else if (i < 100)
        {
            if (ir_dc_offset < 4095)
                ir_dc_offset++;
        }        

        sq_ir_heart_ac_signal += (mul16(ir_heart_ac_signal, ir_heart_ac_signal) >> 10);

                                            //Tune the LED intensity to keep
                                            //the signal produced by the first
                                            //stage within our target range.
                                            //We don't really care what the
                                            //exact values from the first
                                            //stage are. They need to be
                                            //quite high, because a weak
                                            //signal will give poor results
                                            //in later stages. However, the
                                            //exact value only has to be
                                            //within the range that can be
                                            //handled properly by the next
                                            //stage. */        
        
        if (ir_sample > FIRST_STAGE_TARGET_HIGH
            ||
            ir_sample < FIRST_STAGE_TARGET_LOW)
        {
                                            //We are out of the target range
                                            //Starting kicking the LED
                                            //intensity in the right
                                            //direction to bring us back
                                            //into range. We use fine steps
                                            //when we are close to the target
                                            //range, and coarser steps when
                                            //we are far away.
            if (ir_sample > FIRST_STAGE_TARGET_HIGH)
            {
                if (ir_sample >= FIRST_STAGE_TARGET_HIGH_FINE)
                    ir_LED_level -= FIRST_STAGE_STEP;
                else
                    ir_LED_level -= FIRST_STAGE_FINE_STEP;
                                            // Clamp to the range of the DAC 
                if (ir_LED_level < 0)
                    ir_LED_level = 0;
            } 
            else
            {
                if (ir_sample < FIRST_STAGE_TARGET_LOW_FINE)
                    ir_LED_level += FIRST_STAGE_STEP;
                else
                    ir_LED_level += FIRST_STAGE_FINE_STEP;
                                            // Clamp to the range of the DAC 
                if (ir_LED_level > 4095)
                    ir_LED_level = 4095;
            }
        } 

        /* UART Transmission - IR heart signals */      
        switch (scope_type)
        {
        case SCOPE_TYPE_HEART_SIGNALS:
            i = (ir_heart_ac_signal >> 6) + 128;
                                            // Saturate to a byte 
            if (i >= 255)                   // Make sure the data != 0x0 or 0xFF
                i = 254;                    // as 0x0 and 0xFF are used for sync
            else if (i <= 0)                // bytes in the LABVIEW GUI
                i = 1;
            
           TXBUF0 = 0x00;                   // Byte 1 - 0x00 (synchronization byte)
           while (!(IFG1 & UTXIFG0)); 
           TXBUF0 = 0xFF;                   // Byte 2 - 0xFF (synchronization byte)
           while (!(IFG1 & UTXIFG0)); 
           TXBUF0 = i;                      // Byte 3 - IR Heart signal (AC only)
           while (!(IFG1 & UTXIFG0));
           TXBUF0 = heart_rate_LSB;         // Byte 4 - Heart rate data
           while (!(IFG1 & UTXIFG0));
           TXBUF0 = SaO2_LSB;               // Byte 5 - %SaO2 data
           break;
            
        case SCOPE_TYPE_RAW_SIGNALS:
            while (!(IFG1 & UTXIFG0));            
            TXBUF0 = ir_sample >> 4;
            break;
        case SCOPE_TYPE_LED_DRIVE:
            TXBUF0 = ir_LED_level >> 4;
            break;
        }   
        
        /* Track the beating of the heart */
        heart_signal_sample_counter++;
        if (pos_edge)
        {
            if (edge_debounce < 120)
            {
                edge_debounce++;
            }
            else
            {
                if (ir_heart_ac_signal < -200) 
                   {
                    edge_debounce = 0;
                    pos_edge = 0;
                    display_pulse(0);
                   }                                   
                }
         }
        //}
        else
        {      
                       //added start until line 
                   
                      if ((i<139)&&(i>105))
                      {
                        count++;
                        if(count>400)
                          count=450;
                      }
                      
                      else
                        count=0;
                      
                      //clearlcd();
                    if(count>400)
                    {
                      printm("Please place           ",1,0);
                      printm("your finger             ",2,0);
                      printnum(heart_rate,2,19);
                      printnum(i,2,13);
                      printnum(count,1,14);
                      printnum((400-ir_heart_ac_signal),1,20);
                      
                    }
                                                     ///added end
            if (edge_debounce < 120)
            {
                edge_debounce++;
            }
            else
            {
                if (ir_heart_ac_signal > 1000)//changed from 200=>1000
                {
                    edge_debounce = 0;
                    pos_edge = 1;
                    display_pulse(1);
                    
                    if (++heart_beat_counter >= 3)
                    {
                        log_heart_signal_sample_counter = heart_signal_sample_counter;
                        log_sq_ir_heart_ac_signal = sq_ir_heart_ac_signal;
                        log_sq_vs_heart_ac_signal = sq_vs_heart_ac_signal;
                        heart_signal_sample_counter = 0;
                        sq_ir_heart_ac_signal = 0;
                        sq_vs_heart_ac_signal = 0;
                        heart_beat_counter = 0;
                        _BIC_SR_IRQ(LPM0_bits);
                                            // Do a dummy wake up roughly
                                            // every 2 seconds
                    }
                }
            }
        }
    }
    else                                    //D3 enabled in demoboard
    {
                                            //Immediately enable the IR LED,
                                            //to allow time for the
                                            //transimpedance amp to settle */
        DAC12_0CTL &= ~DAC12ENC; 
        P2OUT &= ~BIT2;                     //turn on source for D3        
        DAC12_0CTL |= DAC12OPS;             // Disable visible LED, enable IR LED
        DAC12_0CTL |= DAC12ENC;
        DAC12_0DAT = ir_LED_level;
        DAC12_1DAT = ir_dc_offset;          // Load op-amp offset value for IR
        P2OUT |= BIT3;                      //turn off source for D2
        
        is_IR = 1;                          // IR LED ON
        
        vs_sample = ADC12MEM0;              //Read the visible LED results 
        i = ADC12MEM1;
        
                                            //Enable the next conversion sequence. 
                                            //The sequence is started by TA1 
        ADC12CTL0 &= ~ENC;
        ADC12CTL0 |= ENC;  


                                            //Filter away 50/60Hz electrical
                                            //pickup, and 100/120Hz room
                                            //lighting optical pickup */
        vs_heart_signal = vs_filter(i);
                                            //Filter away the large DC
                                            //component from the sensor */
        vs_heart_ac_signal = vs_heart_signal - dc_estimator(&vs_2nd_dc_register, vs_heart_signal);
        
       /* Bring the VS signal into range through the second opamp */
        if (i >= 4095)
        {
            if (vs_dc_offset > 100)
                vs_dc_offset--;
        }
        else if (i < 100)
        {
            if (vs_dc_offset < 4095)
                vs_dc_offset++;
        }        

        sq_vs_heart_ac_signal += (mul16(vs_heart_ac_signal, vs_heart_ac_signal) >> 10);
        
        if (vs_sample > FIRST_STAGE_TARGET_HIGH
            ||
            vs_sample < FIRST_STAGE_TARGET_LOW)
        {
            /* We are out of the target range */
            //display_correcting(1, 1);
            if (vs_sample > FIRST_STAGE_TARGET_HIGH)
            {
                if (vs_sample >= FIRST_STAGE_TARGET_HIGH_FINE)
                    vs_LED_level -= FIRST_STAGE_STEP;
                else
                    vs_LED_level -= FIRST_STAGE_FINE_STEP;
                if (vs_LED_level < 0)
                    vs_LED_level = 0;
            }
            else
            {
                if (vs_sample < FIRST_STAGE_TARGET_LOW_FINE)
                    vs_LED_level += FIRST_STAGE_STEP;
                else
                    vs_LED_level += FIRST_STAGE_FINE_STEP;
                if (vs_LED_level > 4095)
                    vs_LED_level = 4095;
            }
        }
    }
       
}
    
#pragma vector=ADC_VECTOR
__interrupt void ADC12ISR(void)
{
    ADC12IFG &= ~BIT1;                      // Clear the ADC12 interrupt flag
    DAC12_0DAT = 0;                         // Turn OFF the LED 
    DAC12_1DAT = 0; 
    // Turn OFF the H-Bridge completely
    if(is_IR)                               // If IR LED was ON in TA0 ISR
      P2OUT |= BIT2;                        // P2.2 = 1
    else                                    // Else if VS LED ON in TA0 ISR
      P2OUT |= BIT3;                        // P2.3 = 1
}

int16_t ir_filter(int16_t sample)
{
    static int16_t buf[32];
    static int offset = 0;
    int32_t z;
    int i;
                                            //Filter hard above a few Hertz,
                                            //using a symmetric FIR.
                                            //This has benign phase
                                            //characteristics */
    buf[offset] = sample;
    z = mul16(coeffs[11], buf[(offset - 11) & 0x1F]);
    for (i = 0;  i < 11;  i++)
        z += mul16(coeffs[i], buf[(offset - i) & 0x1F] + buf[(offset - 22 + i) & 0x1F]);
    offset = (offset + 1) & 0x1F;
    return  z >> 15;
}

int16_t vs_filter(int16_t sample)
{
    static int16_t buf[32];
    static int offset = 0;
    int32_t z;
    int i;

                                            //Filter hard above a few Hertz,
                                            //using a symmetric FIR.
                                            //This has benign phase
                                            //characteristics */
    buf[offset] = sample;
    z = mul16(coeffs[11], buf[(offset - 11) & 0x1F]);
    for (i = 0;  i < 11;  i++)
        z += mul16(coeffs[i], buf[(offset - i) & 0x1F] + buf[(offset - 22 + i) & 0x1F]);
    offset = (offset + 1) & 0x1F;
    return  z >> 15;
}



int16_t dc_estimator(register int32_t *p, register int16_t x)
{
    /* Noise shaped DC estimator. */
    *p += ((((int32_t) x << 16) - *p) >> 9);
    return (*p >> 16);
}



/* LCD Pulse Display */
void display_pulse(int on)
{     
    if (on)
    {  printm("***",1,21);                    // Heart beat detected enable "***" on LCD
       printm(" * ",2,21);
    }
    else
    {
      printm("   ",1,21);                     // Disable "***" on LCD for blinking effect 
      printm("   ",2,21);
    }
}


void main(void)
{
        
    
    WDTCTL = WDTPW | WDTHOLD; 
    SCFI0 |= FN_4;                          // x2 DCO frequency, 8MHz nominal
                                            // DCO
    SCFQCTL = 91;                           // 32768 x 2 x (91 + 1) = 6.03 MHz
    FLL_CTL0 = DCOPLUS + XCAP10PF;          // DCO+ set so freq = xtal x D x
                                            //(N + 1)
    // Loop until 32kHz crystal stabilizes
    do
    {
      IFG1 &= ~OFIFG;                       // Clear oscillator fault flag
      for (j = 50000; j; j--);              // Delay
    }
    while (IFG1 & OFIFG);                   // Test osc fault flag
    
    // Setup GPIO
    P1DIR = 0xFF;
    P1OUT = 0;
    P2DIR = 0xFF;
    P2DIR |= BIT2 + BIT3;                   // P2.2 and P2.3 o/p direction - 
                                            // drives PNP transistors in H-Bridge
    P2OUT = 0;
    P3DIR = 0xFF;
    P3OUT = 0;
    P4DIR = 0xFF;
    P4OUT = 0;
    P5DIR = 0xFF;
    P5OUT = 0;
    P6OUT = 0; 
    
    /* Setup LCD */   
    init_lcd();
    clearlcd();
    /* First amplifier stage - transconductance configuration */
    P6SEL |= (BIT0 | BIT1 | BIT2);          // Select OA0O
                                            // -ve=OA0I0, +ve=OA0I1
    OA0CTL0 = OAN_0 | OAP_1 | OAPM_3 | OAADC1;
    OA0CTL1 = 0;    
    
    /* Second amplifier stage */
    P6SEL |= (BIT3 | BIT4);                 // Select 0A1O 0A1I
                                            // -ve=OA1I0, +ve=DAC1
                                            // -ve=OA1I0, +ve=DAC1
//    OA1CTL0 = OAN_0 | OAP_3 | OAPM_3 | OAADC1;
//    OA1CTL1 = 0x00;   
                                            // Inverted input internally 
                                            // connected to OA0 output 
    OA1CTL0 = OAN_2 + OAP_3 + OAPM_3 + OAADC1;
    OA1CTL1 = OAFBR_7 + OAFC_6;             // OA as inv feedback amp, internal
                                            // gain = 15;    
    
    /* Configure DAC 1 to provide bias for the amplifier */
    P6SEL |= BIT7;
    DAC12_1CTL = DAC12CALON | DAC12IR | DAC12AMP_7 | DAC12ENC;
    DAC12_1DAT = 0; 
    
    /* Configure DAC 0 to provide variable drive to the LEDs */
    DAC12_0CTL =  DAC12CALON | DAC12IR | DAC12AMP_7 | DAC12ENC; // VRef+, high speed/current, 
                                            // DAC12OPS=0 => DAC12_0 output on P6.6 (pin 5) */
                                            // Configure P2.2 and P2.3 to 
                                            // provide variable drive to LEDs
    P2OUT |= BIT2;                          // turn off source for D2
    P2OUT &= ~BIT3;                         // turn on source for D3        
    DAC12_0DAT = 3340;    

                                            // Set initial values for the LED brightnesses 
    ir_LED_level = 1300;
    vs_LED_level = 1450;    
    
    /* Configure ADC12 */
    ADC12CTL0 &= ~ENC;                      // Enable conversions
                                            // Turn on the ADC12, and
                                            // set the sampling time
    ADC12CTL0 = ADC12ON + MSC + SHT0_4 + REFON + REF2_5V;
    ADC12CTL1 = SHP + SHS_1 + CONSEQ_1;     // Use sampling timer, single sequence,
                                            // TA1 trigger(SHS_1), start with ADC12MEM0
    ADC12MCTL0 = INCH_1 + SREF_1;           // ref+=Vref, channel = A1 = OA0
    ADC12MCTL1 = INCH_3 + SREF_1 + EOS;     // ref+=Vref, channel = A3 = OA1
    ADC12IE = BIT1;                         // ADC12MEM1 interrupt enable
    ADC12CTL0 |= ENC;                       // Enable the ADC
    ADC12CTL0 |= ADC12SC;                   // Start conversion    
    
    /* Configure Timer */
    TACTL = TASSEL0 + TACLR;                // ACLK, clear TAR,
    TACCTL1 = OUTMOD_2;
    TACCTL0 = CCIE;
                                            // This gives a sampling rate of
                                            // 512sps
    TACCR0 = 31;                            // Do two channels, at
                                            // 512sps each.
    TACCR1 = 10;                            // Allow plenty of time for the
                                            // signal to become stable before
                                            // sampling   
    TACTL |= MC_1;                          // Timer A on, up mode     
                                            
    /*Configure USART, so we can report readings to a PC */
    P2DIR |= BIT4;
    P2SEL |= BIT4;
    
    UCTL0 |= SWRST;
    ME1 |= UTXE0;                           // Enable USART1 TXD
    UCTL0 |= CHAR;                          // 8-bit char, SWRST=1
    UTCTL0 |= SSEL1;                        // UCLK = SMCLK
    UBR00 = 52;                             // 115200 from 6.02MHz = 52.33
    UBR10 = 0x00;
    UMCTL0 = 0x45;                          // Modulation = 0.375
    UCTL0 &= ~SWRST;                        // Initialise USART 

 
    menuroutine(); 
    delaym(3000);
    
    mainroutine();
}


