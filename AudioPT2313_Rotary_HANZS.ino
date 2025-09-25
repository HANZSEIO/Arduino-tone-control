#include <Wire.h>
#include <PT2313.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);//(Add, Total Col, Total Row)

PT2313 audioChip;

//Big Numbers Character
byte custom_num[8][8] = {
  { B00111, B01111, B11111, B11111, B11111, B11111, B11111, B11111 },
  { B11111, B11111, B11111, B00000, B00000, B00000, B00000, B00000 },
  { B11100, B11110, B11111, B11111, B11111, B11111, B11111, B11111 },
  { B11111, B11111, B11111, B11111, B11111, B11111, B01111, B00111 },
  { B00000, B00000, B00000, B00000, B00000, B11111, B11111, B11111 },
  { B11111, B11111, B11111, B11111, B11111, B11111, B11110, B11100 },
  { B11111, B11111, B11111, B00000, B00000, B00000, B11111, B11111 },
  { B11111, B11111, B11111, B11111, B11111, B11111, B11111, B11111 }
};

const int digit_width = 3;
const char custom_num_top[10][digit_width] = { 0, 1, 2, 1, 2, 32, 6, 6, 2, 6, 6, 2, 3, 4, 7,   7, 6, 6, 0, 6, 6, 1, 1, 2,   0, 6, 2, 0, 6, 2};
const char custom_num_bot[10][digit_width] = { 3, 4, 5, 4, 7, 4,  7, 4, 4, 4, 4, 5, 32, 32, 7, 4, 4, 5, 3, 4, 5, 32, 32, 7, 3, 4, 5, 4, 4, 5};

//Bar Character
byte a1[8]={0b00000,0b11011,0b11011,0b11011,0b11011,0b11011,0b11011,0b00000};
byte a2[8]={0b00000,0b11000,0b11000,0b11000,0b11000,0b11000,0b11000,0b00000};

//Mute Character
byte mute1[] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00111,
  B00111,
  B00111
};

byte mute2[] = {
  B00111,
  B00111,
  B00111,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

byte mute3[] = {
  B00000,
  B00011,
  B00011,
  B01111,
  B11111,
  B11111,
  B11111,
  B11111
};

byte mute4[] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B01111,
  B00011,
  B00011,
  B00000
};

byte mutex1[] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B01000,
  B00100,
  B00010,
  B00001
};

byte mutex2[] = {
  B10000,
  B01000,
  B00100,
  B00010,
  B00000,
  B00000,
  B00000,
  B00000
};

byte mutex3[] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00010,
  B00100,
  B01000,
  B10000
};

byte mutex4[] = {
  B00001,
  B00010,
  B00100,
  B01000,
  B00000,
  B00000,
  B00000,
  B00000
};


// usually the rotary encoders three pins have the ground pin in the middle
// tactileSwitch High Active with pulldown resistor
enum PinAssignments {
  dtPinA = 2,       // DT
  clkPinB = 3,      // CLK
  menuSwitch = 8,   // SW
  muteButton = 4,   // Tactile Switch1
  inButton   = 5    // Tactile Switch 2
};

static boolean rotating=false;      // debounce management

// interrupt service routine vars
boolean A_set = false;
boolean B_set = false;

boolean loud;

byte menu,in,w,save_eeprom;
int a,b,c,z,hit,menu_active,mute,vol,bassz,treb,balanz,gainz;

unsigned long time;

const int SHORT_PRESS_TIME = 500; // 1000 milliseconds
const int LONG_PRESS_TIME  = 500; // 1000 milliseconds

// Variables will change:
int lastState = HIGH;  // the previous state from the input pin
int currentState;     // the current reading from the input pin
unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;


void setup(){
  
  Serial.begin(9600);
  Wire.begin(); 
  lcd.begin();
  
  audioChip.initialize(0,true);
  
  pinMode(muteButton,INPUT);// Mute
  pinMode(inButton,INPUT);// Input
  
  pinMode(dtPinA, INPUT_PULLUP); 
  pinMode(clkPinB, INPUT_PULLUP);
  pinMode(menuSwitch, INPUT_PULLUP); // Menu
  // encoder pin on interrupt 0 (pin 2) 
  attachInterrupt(0, doEncoderA, CHANGE);
  // encoder pin on interrupt 1 (pin 3)
  attachInterrupt(1, doEncoderB, CHANGE); 

  eepromRead();
  start_up();
  
}

void loop(){

   //InputSelector----------------------------------/ 
   if(digitalRead(inButton)==HIGH)
   {
      in++;
      time=millis();
      save_eeprom=1;
      audio();
      delay(200);
      if(in>2)
      {
        in=0;
        }
    }
    
   //rotaryMenu-------------------------/
   rotaryE();
  
  // read the state of the switch/button:
  // High Active
  currentState = digitalRead(muteButton);

  if (lastState == LOW && currentState == HIGH)       // button is pressed
    pressedTime = millis();
  else if (lastState == HIGH && currentState == LOW) { // button is released
    releasedTime = millis();

    long pressDuration = releasedTime - pressedTime;
/*
    if ( pressDuration < SHORT_PRESS_TIME ) {
    
       //Serial.println("A short press is detected");
    }
*/
    if ( pressDuration > LONG_PRESS_TIME ) {
      mute++;  // mute
      if ( mute>1 ) {
        mute = 0 ;
      }    
      delay(200);
    }
    
    if(mute == 0) {
      menu_active=1;
      vol=0;    
      lcd.clear();
      delay(300);
      lcd.setCursor(0, 1);
      lcd.print("MUTE ");
      lcd.createChar(0, mute1);
      lcd.createChar(1, mute2);
      lcd.createChar(2, mute3);
      lcd.createChar(3, mute4);
      
      lcd.createChar(4, mutex1);
      lcd.createChar(5, mutex2);
      lcd.createChar(6, mutex3);
      lcd.createChar(7, mutex4);    
      lcd.setCursor(12, 0);
      lcd.write(0);
      lcd.setCursor(12, 1);
      lcd.write(1);    
      lcd.setCursor(13, 0);
      lcd.write(2);
      lcd.setCursor(13, 1);
      lcd.write(3);
      lcd.setCursor(14, 0);
      lcd.write(4); 
      lcd.setCursor(15, 1);
      lcd.write(5);  
      lcd.setCursor(15, 0);
      lcd.write(6);
      lcd.setCursor(14, 1);
      lcd.write(7);
      menu = 99 ;
    }
    if (mute == 1 && menu_active==1) {
      vol = EEPROM.read(0);
      lcd.clear();
      menu_active=0;
      menu = 0 ;
      //Serial.println("A long press is detected");
    }
    audio();  
  }

  // save the the last state
  lastState = currentState;

 
 //Save semua pengaturan ke EEPROM jika tombol + dan - tidak ditekan selama 10 detik  
 if(millis()-time>10000 && save_eeprom==1)
  {
     eepromUpdate();
     menu=0;
     save_eeprom=0;
     cl();
  }
 
}

void eepromRead()
{
  vol = EEPROM.read(0); 
  bassz = EEPROM.read(1)-7;
  treb = EEPROM.read(2)-7;
  balanz = EEPROM.read(3)-4;
  in = EEPROM.read(4);
  loud = EEPROM.read(5);
  gainz = EEPROM.read(6);
}

void eepromUpdate()
{
     EEPROM.update(0,vol);    
     EEPROM.update(1,bassz+7);
     EEPROM.update(2,treb+7);
     EEPROM.update(3,balanz+4);
     EEPROM.update(4,in);
     EEPROM.update(5,loud);
     EEPROM.update(6,gainz);
}

void cl()
{
  delay(50);
  lcd.clear();
}

void audio(){  
  audioChip.source(in);//select your source 0...3
  audioChip.volume(vol);//Vol 0...62 : 63 = muted
  audioChip.gain(gainz);//gain 0...3 db //11.27 db
  audioChip.bass(bassz);//bass -7...+7
  audioChip.treble(treb);//treble -7...+7
  audioChip.balance(balanz);//-31...+31
  audioChip.loudness(loud);//true or false
}

//startUp -----------------------------------------------------//
void start_up() {
  vol = 0;  
  audio();
  lcd.setCursor(0, 1);
  lcd.print("  ...HELLO...  ");
  delay(1500);
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("LOADING   ");
    for(int x=6; x<15; x++)
    {
      lcd.setCursor(x+1, 1);
      lcd.print(".");
      delay(300);
    }
  delay(1000);
  lcd.clear();
  vol = EEPROM.read(0);
  menu=0;
}


//lcd_Update-----------------------------------------------------//
void lcd_update(){ 
  if(menu_active==0)
    {
      custom_num_shape();
      
      lcd.setCursor(0, 1);
      lcd.print("MAS-VOL");
      int y;
      if (c < 0) {
        lcd.setCursor(8, 1);
        lcd.print("-");
        y = 10 - (c + 10);
      } 
      else if (c == -10) {
        lcd.setCursor(8, 1);
        lcd.print("-");
        y = 10;
      } 
      else {
        lcd.setCursor(8, 1);
        lcd.print(" ");
        y = c;
      }
      a = y / 10;
      b = y - a * 10; // y%10
  
      lcd.setCursor(9, 0);
      for (int i = 0; i < digit_width; i++)
      lcd.print(custom_num_top[a][i]);
  
      lcd.setCursor(9, 1);
      for (int i = 0; i < digit_width; i++)
      lcd.print(custom_num_bot[a][i]);
  
      lcd.setCursor(13, 0);
      for (int i = 0; i < digit_width; i++)
      lcd.print(custom_num_top[b][i]);
  
      lcd.setCursor(13, 1);
      for (int i = 0; i < digit_width; i++)
      lcd.print(custom_num_bot[b][i]);
    }
    if(menu_active==1)
    {
      //...................../
    }
   
}

//custom shape ------------------------------------------------------//
void custom_num_shape() {
  for (int i = 0; i < 8; i++)
    lcd.createChar(i, custom_num[i]);
}

//rotary menu------------------------------------------------/
void rotaryE()
{
  rotating = true;  // reset the debouncer
  if(menu==0)
  {
     if(w==1)
      {
        audio();
        //cl();
        time=millis();
        save_eeprom=1;
        w=0;       
       }   
      c = vol;
      lcd.setCursor(0, 0);
      lcd.print("IN-");
      lcd.print(in+1);
      lcd_update(); 
  }

  
  
  if(menu==1)
  {
     if(w==1)
     {
      audio();
      cl();
      time=millis();
      save_eeprom=1; 
      w=0;
     }
       
     lcd.setCursor(0,0);
     lcd.print("Bass");
     lcd.setCursor(12,0);
     lcd.print(bassz);
     lcd.setCursor(14,0);
     lcd.print("dB");
     lcd.createChar(0,a1);
     lcd.createChar(1,a2);
     
     if(bassz<0)
     {     
      for(int x=-1; x>=bassz; x--)
      {
        lcd.setCursor(7,1);
        lcd.print("<");
        lcd.setCursor(8,1);
        lcd.print(">");
        lcd.setCursor(x+7,1);//8
        lcd.write((uint8_t)0);
      }
     }
     
     if(bassz>0)
     {
       for(int x=1; x<=bassz; x++)
      {
        lcd.setCursor(7,1);
        lcd.print("<");
        lcd.setCursor(8,1);
        lcd.print(">");
        lcd.setCursor(x+8,1);//7
        lcd.write((uint8_t)0);
      }
     }
     
     if(bassz==0)
     {
      lcd.setCursor(7,1);
      lcd.print("<");
      lcd.setCursor(8,1);
       lcd.print(">");
      }
 
  }

  if(menu==2)
  {
     if(w==1)
     {
      audio();
      cl();
      time=millis();
      save_eeprom=1;
      w=0;
     }
     
     lcd.setCursor(0,0);
     lcd.print("Trebble");
     lcd.setCursor(12,0);
     lcd.print(treb);
     lcd.setCursor(14,0);
     lcd.print("dB");
     lcd.createChar(0,a1);
     lcd.createChar(1,a2);

     if(treb<0)
     {     
      for(int x=-1; x>=treb; x--)
      {
        lcd.setCursor(7,1);
        lcd.print("<");
        lcd.setCursor(8,1);
        lcd.print(">");
        lcd.setCursor(x+7,1);//8
        lcd.write((uint8_t)0);
      }
     }
     
     if(treb>0)
     {
       for(int x=1; x<=treb; x++)
      {
        lcd.setCursor(7,1);
         lcd.print("<");
        lcd.setCursor(8,1);
        lcd.print(">");
        lcd.setCursor(x+8,1);//7
        lcd.write((uint8_t)0);
      }
     }
     
     if(treb==0)
     {
      lcd.setCursor(7,1);
      lcd.print("<");
      //lcd.write((uint8_t)0);
      lcd.setCursor(8,1);
      //lcd.write((uint8_t)0);
      lcd.print(">");
      }    
  }

  if(menu==3)
  {
     lcd.setCursor(0,0);

     if(balanz>=0)
     {
      lcd.print("-");
      }
      else
      {
        lcd.print("+");
        }
        
     lcd.print(abs(balanz*2));
     lcd.print(" dB ");
     lcd.print(" <>  ");
     
     if(balanz>=0)
     {
      lcd.print("+");
      }
      else
      {
        lcd.print("-");
        }
        
     lcd.print(abs(balanz*2));
     lcd.print(" dB ");
     lcd.setCursor(0,1);
     lcd.print("R");
     lcd.setCursor(15,1);
     lcd.print("L");
     
     if(balanz<0)
     {
      lcd.setCursor(balanz+7,1);
      lcd.write((uint8_t)0);
      }
     if(balanz>0)
     {
      lcd.setCursor(balanz+8,1);
      lcd.write((uint8_t)0);
      }
     if(balanz==0)
     {
      lcd.setCursor(7,1);
      lcd.write((uint8_t)0);
      lcd.setCursor(8,1);
      lcd.write((uint8_t)0);
      }
      
     if(w==1)
     {
      audio();
      cl();
      time=millis();
      save_eeprom=1;
      w=0;
     }    
  }

  if(menu==4)
  {
    if(hit==1)
       {
        loud = true;
       }
       else
       {
        loud = false;
        
       }delay(300);


     lcd.setCursor(2, 0);
     lcd.print("- LOUDNESS -");
    
     if(loud==false)
     {
        
        lcd.setCursor(5,1);
        lcd.print(">>OFF"); 
  
     }
     else
     {
        lcd.setCursor(5,1);
        lcd.print(">>ON ");  
  
     }
 
     if(w==1)
     {
      audio();
      //cl();
      time=millis();
      save_eeprom=1;
      w=0;
     }    
  }

  if(menu==5)
  {
     if(w==1)
     {
      audio();
      cl();
      time=millis();
      save_eeprom=1;
      w=0;
     }
     
     lcd.setCursor(0,0);
     lcd.print("Gain");
     lcd.setCursor(12,0);
     lcd.print(gainz);
     lcd.setCursor(14,0);
     lcd.print("dB");
     lcd.createChar(0,a1);
     lcd.createChar(1,a2);
     for(z=0;z<=gainz;z++)
     {
      lcd.setCursor(z*3+2,1);
      lcd.write((uint8_t)0);
      lcd.write((uint8_t)0);
      lcd.write((uint8_t)0);
     }    
  }

 
  if (digitalRead(menuSwitch) == LOW )  {
      menu++;
      time=millis();
      save_eeprom=1;
      w=1;
      cl();
      if(menu>5)
      {
        menu=0;
      } 
  }

}

// Interrupt on A changing state ........................./
void doEncoderA(){
  // debounce
  if ( rotating ) delay (1);  // wait a little until the bouncing is done
  if( digitalRead(dtPinA) != A_set ) {
    A_set = !A_set;
    // adjust counter + if A leads B
    if ( A_set && !B_set )
    if(menu==0)
    {
       vol++;
       w=1;
       if(vol>62)
       {
        vol=62;
        }
    }
    else if(menu==1)
    {
      bassz++;
      w=1;
      if(bassz>7)
      {
        bassz=7;
        }  
 
    }
    else if(menu==2)
    {
      treb++;
      w=1;
      if(treb>7)
      {
        treb=7;
        }      
    }
    else if(menu==3)
    {
      balanz++;
      w=1;
      if(balanz>4)
      {
        balanz=4;
        }      
    }
    else if(menu==4)
    {
      hit++;
      w=1;
      if(hit>1)
      {
        hit=0;
        }      
    }
    else if(menu==5)
    {
      gainz++;
      w=1;
      if(gainz>3)
      {
        gainz=3;
        }      
    }
    rotating = false;  // no more debouncing until loop() hits again
  }
}

// Interrupt on B changing state, same as A above........../
void doEncoderB(){
  if ( rotating ) delay (1);
  if( digitalRead(clkPinB) != B_set ) {
    B_set = !B_set;
    //  adjust counter + if B leads A
    if( B_set && !A_set )
    if(menu==0)
    {
        vol--;
        w=1;
        if(vol<0)
        {
          vol=0;
          }
    }

    else if(menu==1)
    {
      bassz--;
      w=1;
      if(bassz<-7)
      {
        bassz=-7;
        }
     
    }
    else if(menu==2)
    {
      treb--;
      w=1;
      if(treb<-7)
      {
        treb=-7;
        }      
    }
    else if(menu==3)
    {
      balanz--;
      w=1;
      if(balanz<-4)
      {
        balanz=-4;
        }      
    }
    else if(menu==4)
    {
      hit--;
      w=1;
      if(hit<0)
      {
        hit=1;
        }      
    }
    else if(menu==5)
    {
      gainz--;
      w=1;
      if(gainz<0)
      {
        gainz=0;
        }      
    }
    rotating = false;
  }
}

//-----------------------------------------------------/
/*
 * aktif LOW
void switchRotary()
{
    // read the state of the switch/button:
  currentState = digitalRead(swButton);

  if (lastState == HIGH && currentState == LOW)       // button is pressed
    pressedTime = millis();
  else if (lastState == LOW && currentState == HIGH) { // button is released
    releasedTime = millis();

    long pressDuration = releasedTime - pressedTime;

    if ( pressDuration < SHORT_PRESS_TIME ) {
      menu++;
      time=millis();
      save_eeprom=1;
      w=1;
      cl();
      if(menu>5)
      {
        menu=0;
      }
       //Serial.println("A short press is detected");
    }

    if ( pressDuration > LONG_PRESS_TIME ) {
      mute++;  // mute
      if ( mute>1 ) {
        mute = 0 ;
      }    
      delay(200);
    }
    
    if(mute == 0) {
      menu_active=1;
      vol=0;    
      lcd.clear();
      delay(300);
      lcd.setCursor(0, 1);
      lcd.print("MUTE ");
      lcd.createChar(0, mute1);
      lcd.createChar(1, mute2);
      lcd.createChar(2, mute3);
      lcd.createChar(3, mute4);
      
      lcd.createChar(4, mutex1);
      lcd.createChar(5, mutex2);
      lcd.createChar(6, mutex3);
      lcd.createChar(7, mutex4);    
      lcd.setCursor(12, 0);
      lcd.write(0);
      lcd.setCursor(12, 1);
      lcd.write(1);    
      lcd.setCursor(13, 0);
      lcd.write(2);
      lcd.setCursor(13, 1);
      lcd.write(3);
      lcd.setCursor(14, 0);
      lcd.write(4); 
      lcd.setCursor(15, 1);
      lcd.write(5);  
      lcd.setCursor(15, 0);
      lcd.write(6);
      lcd.setCursor(14, 1);
      lcd.write(7);
      menu = 99 ;
    }
    if (mute == 1) {
      vol = EEPROM.read(0);
      lcd.clear();
      menu_active=0;
      menu = 0 ;
      //Serial.println("A long press is detected");
    }
    audio();  
  }
  // save the the last state
  lastState = currentState;
}
*/

/*
 * 
  audioChip.initialize(1,true);//source 1,mute on
  audioChip.source(1);//select your source 0...3
  audioChip.volume(33);//Vol 0...62 : 63=muted
  audioChip.gain(3);//gain 0...11.27 db
  audioChip.loudness(true);//true or false
  audioChip.bass(0);//bass -7...+7
  audioChip.treble(0);//treble -7...+7
  audioChip.balance(0);//-31...+31
 */
