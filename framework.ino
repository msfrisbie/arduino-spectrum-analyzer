#include "LPD8806.h"
#include "SPI.h"
#include <PS2Keyboard.h>
//#include "DMA.h"
//#include "LEDarray.h"

// pins
#define CLK 2
#define CLK2 13
#define DATA0 3
#define DATA1 4
#define DATA2 5
#define DATA3 6
#define DATA4 7
#define DATA5 8
#define DATA6 9
#define DATA7 10
#define DATA8 11
#define DATA9 12
// local sizes
#define STRIPLEN 32
#define NUM_LED_STRIPS 10
// MSGEQ7 pins
#define MSGEQ7_STROBE 46
#define MSGEQ7_RESET 13



///////////////// CLASS DEFINITIONS /////////////////



class RGB_LED {

        public:
                // 4 pointers to N,S,E,W LED's
                RGB_LED *north,*south,*east,*west;
                // 3 pointers to r,g,b values in individual pixels[] array
                uint8_t *r,*g,*b;

};

class LEDarray {

        public:
                // array of RGB LED strip pointers
                LPD8806 *LEDstrip[NUM_LED_STRIPS];
                // RGB LED strip members
                LPD8806 strip0, strip1, strip2, strip3, strip4, 
                        strip5, strip6, strip7, strip8, strip9, 
                        right_buf, bottom_buf;
                // 2D LED array
                RGB_LED RGBarray[STRIPLEN+1][NUM_LED_STRIPS+1];
                
                // constructor
                LEDarray();
                
                // routine to be run in setup()
                void init(void);
                
                void circular_shift_up(void);
                void circular_shift_down(void);
                void circular_shift_right(void);
                void circular_shift_left(void);
                
                void assign(int x, int y, int r, int g, int b);
                void assign(int x, int y, char c[]);
                uint32_t color(byte r, byte g, byte b);
                
                void show(void);
                void quickshow(void);
                
                void erase(void);
                
                void testfn(void);

};

class foregroundShape {
        
        public:
                // global coordinates
                int Xcenter,Ycenter;
                // coordinates used to clear old pixel
                int Xcenter_old,Ycenter_old;
                // offset values
                int Xoff,Yoff;
                
                uint8_t pixels[8];
                
                // array of pixels, by offset from center
                // [0,0] is NOT assumed to exist
                //int **pixelArray;
                
                // color
                int r,g,b;
                
                // velocity
                int Xvelocity,Yvelocity;
                // acceleration
                int Xaccel,Yaccel;
                
                // constructor
                foregroundShape(int Xcenter, int Ycenter, uint8_t pixels[8], int r, int g, int b, 
                                int Xvelocity, int Yvelocity, int Xaccel, int Yaccel);
                
                // update member values based on velocity,acceleration,position in array
                void move(void);
                
                // draw into pixel array
                void draw(void);
                
};

class channelSpectrum {
        
        public:
                // left or right channel selection
                int channelID;
                // array of analog frequency values
                int freqLeft[7];
                int freqRight[7];
                
                // constructor
                channelSpectrum();
                
                // initialize
                void init(void);
                
                // collect analog values from MSGEQ7
                void collect(void);
                
                // draw into pixel array
                void draw(void);
};

class charWord {
        
        public:
                int len;
                foregroundShape letters[6];
        
};

///////////////// MAIN THREAD OF EXECUTION /////////////////

int incomingByte = 0;

void (*main_func)(void);

//int MSGEQ7_RESET=13;
//int MSGEQ7_STROBE=46;
int spectrumAnalog=1;  //0 for left channel, 1 for right.

int Spectrum[7];

//int logScale[10] =     {0,200,425,550,700,850,920,940,960,980};
//int logScale[10] =     {0,260,345,430,515,610,690,780,870,960};
int logScale[10] =     {0,220,385,480,595,710,770,840,910,960};


uint8_t block[8] =     {0b11111000,
                        0b10001000,
                        0b10001000,
                        0b10001000,
                        0b11111000,
                        0b00000000,
                        0b00000000,
                        0b00000000};
                        
uint8_t triangle[8] =  {0b11111110,
                        0b10000100,
                        0b10001000,
                        0b10010000,
                        0b10100000,
                        0b11000000,
                        0b10000000,
                        0b00000000};
                        
uint8_t penis[8] =     {0b00000000,
                        0b00000011,
                        0b00000111,
                        0b00001110,
                        0b00011100,
                        0b01111000,
                        0b01111000,
                        0b00111000};
                        
                        
uint8_t char_a[8] =    {0b01110000,
                        0b10001000,
                        0b11111000,
                        0b10001000,
                        0b10001000,
                        0b00000000,
                        0b00000000,
                        0b00000000};
                        
uint8_t char_b[8] =    {0b11110000,
                        0b10001000,
                        0b11110000,
                        0b10001000,
                        0b11110000,
                        0b00000000,
                        0b00000000,
                        0b00000000};

LEDarray LEDarray;

channelSpectrum channelSpectrum;

// foregroundShape(int Xcenter, int Ycenter, uint8_t pixels[8], 
// int r, int g, int b, int Xvelocity, int Yvelocity, int Xaccel, int Yaccel);

//                                       x, y,array,   r,  g,  b,   vel, acc
//foregroundShape shape1 = foregroundShape(0, 0,block ,  0,127,  0, 2,1, 0,0);
//foregroundShape shape2 = foregroundShape(14,3,triangle,127,  0,0, -1,1, 0,0);
//foregroundShape shape3 = foregroundShape(2, 2,penis,   0,    0,127,1,1,0,0);

//foregroundShape textArray[6];

//int x = 3;
//int y = 8;

//int dir = 1;
        
void setup() {

        //Serial.begin(9600);
        Serial.begin(9600);
        Serial2.begin(9600);

        channelSpectrum.init();

        /*
        uint8_t Counter;

          //Setup pins to drive the spectrum analyzer. 
          pinMode(MSGEQ7_RESET, OUTPUT);
          pinMode(MSGEQ7_STROBE, OUTPUT);
        
          //Init spectrum analyzer
          digitalWrite(MSGEQ7_STROBE,LOW);
            delay(1);
          digitalWrite(MSGEQ7_RESET,HIGH);
            delay(1);
          digitalWrite(MSGEQ7_STROBE,HIGH);
            delay(1);
          digitalWrite(MSGEQ7_STROBE,LOW);
            delay(1);
          digitalWrite(MSGEQ7_RESET,LOW);
            delay(5);
        */

        LEDarray.init();
        

        // initialize entire array to blank
        LEDarray.erase();

        LEDarray.show();

        // 1: up, right
        // 2: down, right
        // 3: down, left
        // 4: up, left
        
        main_func = &spectrumAnalyzer;
        
        //LEDarray.assign(3,8,"blue");

}

void loop() {
        
        if(Serial2.available()>0) {
                main_func = &textInput;
                //Serial2.flush();
	}
        
        main_func();
           
        
        //LEDarray.erase();
        
        //shape1.draw();
        //shape2.draw();
        //shape3.draw();
        
        //shape1.move();
        //shape2.move();
        //shape3.move();
        
        //LEDarray.show();
        
        //int Counter, Counter2, Counter3;
            
        //showSpectrum();
        //readSpectrum();
          
        //channelSpectrum.collect();
        //channelSpectrum.draw();
          
        //delay(15);  //We wait here for a little while until all the values to the LEDs are written out.
                      //This is being done in the background by an interrupt.
                      
        //LEDarray.quickshow();
        
        //delay(40);
        

}



///////////// FUNCTIONS ///////////////



LEDarray::LEDarray() {

        // declare 5x32 strips
        strip0 = LPD8806(STRIPLEN, DATA0, CLK);
        strip1 = LPD8806(STRIPLEN, DATA1, CLK);
        strip2 = LPD8806(STRIPLEN, DATA2, CLK);
        strip3 = LPD8806(STRIPLEN, DATA3, CLK);
        strip4 = LPD8806(STRIPLEN, DATA4, CLK);
        strip5 = LPD8806(STRIPLEN, DATA5, CLK);
        strip6 = LPD8806(STRIPLEN, DATA6, CLK);
        strip7 = LPD8806(STRIPLEN, DATA7, CLK);
        strip8 = LPD8806(STRIPLEN, DATA8, CLK);
        strip9 = LPD8806(STRIPLEN, DATA9, CLK);
        right_buf = LPD8806(NUM_LED_STRIPS);
        bottom_buf = LPD8806(STRIPLEN);
        
        // assign pointer array to strips
        LEDstrip[0] = &strip0;
        LEDstrip[1] = &strip1;
        LEDstrip[2] = &strip2;
        LEDstrip[3] = &strip3;
        LEDstrip[4] = &strip4;
        LEDstrip[5] = &strip5;
        LEDstrip[6] = &strip6;
        LEDstrip[7] = &strip7;
        LEDstrip[8] = &strip8;
        LEDstrip[9] = &strip9;
        
        // recall, circular_shift buffer RGB_LED objects are on the bottom & right of the main array
        
        // iterate through all LEDs on the strip
        for(int i=0; i<=STRIPLEN; i++) {
                // iterate through all strips
                for(int j=0; j<=NUM_LED_STRIPS; j++) {
                        
                        if(i==STRIPLEN && j==NUM_LED_STRIPS)
                                break;
                        
                        if(i==STRIPLEN) {
                                RGBarray[i][j].r = &right_buf.pixels[j*3];
                                RGBarray[i][j].g = &right_buf.pixels[j*3+1];
                                RGBarray[i][j].b = &right_buf.pixels[j*3+2];
                        }
                        else if(j==NUM_LED_STRIPS) {
                                RGBarray[i][j].r = &bottom_buf.pixels[i*3];
                                RGBarray[i][j].g = &bottom_buf.pixels[i*3+1];
                                RGBarray[i][j].b = &bottom_buf.pixels[i*3+2];
                        }
                        else {
                                RGBarray[i][j].r = &LEDstrip[j]->pixels[i*3];
                                RGBarray[i][j].g = &LEDstrip[j]->pixels[i*3+1];
                                RGBarray[i][j].b = &LEDstrip[j]->pixels[i*3+2];
                        }
                        
                        // north
                        if(j==0)
                                RGBarray[i][j].north = &RGBarray[i][NUM_LED_STRIPS];
                        else
                                RGBarray[i][j].north = &RGBarray[i][j-1];
                        // south
                        if(j==NUM_LED_STRIPS)
                                RGBarray[i][j].south = &RGBarray[i][0];
                        else
                                RGBarray[i][j].south = &RGBarray[i][j+1];
                        // east
                        if(i==STRIPLEN)
                                RGBarray[i][j].east = &RGBarray[0][j];
                        else
                                RGBarray[i][j].east = &RGBarray[i+1][j];
                        // west
                        if(i==0)
                                RGBarray[i][j].west = &RGBarray[STRIPLEN][j];
                        else
                                RGBarray[i][j].west = &RGBarray[i-1][j];
                } 
        }
}

// initializes all LED strips, both physical and virtual
void LEDarray::init(void) {

        for(int i=0; i<NUM_LED_STRIPS; i++) {
                LEDstrip[i]->begin();
                LEDstrip[i]->show();
        }
        
        right_buf.begin();
        right_buf.show();
        
        bottom_buf.begin();
        bottom_buf.show();

}

// shifts entire array one pixel up
void LEDarray::circular_shift_up(void) {
        // iterate through all LEDs on the strip
        for(int i=0; i<STRIPLEN; i++) {
                // iterate through all strips
                for(int j=0; j<=NUM_LED_STRIPS; j++) {
                        *RGBarray[i][j].north->r = *RGBarray[i][j].r;
                        *RGBarray[i][j].north->g = *RGBarray[i][j].g;
                        *RGBarray[i][j].north->b = *RGBarray[i][j].b;
                        
                        *RGBarray[i][j].r = *RGBarray[i][j].south->r;
                        *RGBarray[i][j].g = *RGBarray[i][j].south->g;
                        *RGBarray[i][j].b = *RGBarray[i][j].south->b;
                }
        }
}

// shifts entire array one pixel down
void LEDarray::circular_shift_down(void) {
        // iterate through all LEDs on the strip
        for(int i=0; i<STRIPLEN; i++) {
                // iterate through all strips
                for(int j=NUM_LED_STRIPS; j>-1; j--) {
                        *RGBarray[i][j].south->r = *RGBarray[i][j].r;
                        *RGBarray[i][j].south->g = *RGBarray[i][j].g;
                        *RGBarray[i][j].south->b = *RGBarray[i][j].b;
                        
                        *RGBarray[i][j].r = *RGBarray[i][j].north->r;
                        *RGBarray[i][j].g = *RGBarray[i][j].north->g;
                        *RGBarray[i][j].b = *RGBarray[i][j].north->b;
                }
        }
}

// shifts entire array one pixel to the left
void LEDarray::circular_shift_left(void) {
        // iterate through all LEDs on the strip
        for(int i=0; i<STRIPLEN; i++) {
                // iterate through all strips
                for(int j=0; j<=NUM_LED_STRIPS; j++) {
                        *RGBarray[i][j].west->r = *RGBarray[i][j].r;
                        *RGBarray[i][j].west->g = *RGBarray[i][j].g;
                        *RGBarray[i][j].west->b = *RGBarray[i][j].b;
                        
                        *RGBarray[i][j].r = *RGBarray[i][j].east->r;
                        *RGBarray[i][j].g = *RGBarray[i][j].east->g;
                        *RGBarray[i][j].b = *RGBarray[i][j].east->b;
                }
        }
}

// shifts entire array one pixel to the right
// BROKEN
void LEDarray::circular_shift_right(void) {
        // iterate through all LEDs on the strip
        for(int i=STRIPLEN; i>-1; i--) {
                // iterate through all strips
                for(int j=0; j<=NUM_LED_STRIPS; j++) {
                        *RGBarray[i][j].east->r = *RGBarray[i][j].r;
                        *RGBarray[i][j].east->g = *RGBarray[i][j].g;
                        *RGBarray[i][j].east->b = *RGBarray[i][j].b;
                        
                        *RGBarray[i][j].r = *RGBarray[i][j].west->r;
                        *RGBarray[i][j].g = *RGBarray[i][j].west->g;
                        *RGBarray[i][j].b = *RGBarray[i][j].west->b;
                }
        }
}

// sets pixel at (x,y) to color defined by (r,g,b)
void LEDarray::assign(int x, int y, int r, int g, int b) {
        
        this->LEDstrip[y]->setPixelColor(x, color(r,g,b));

}

// sets pixel at (x,y) to predefined color 'c'
void LEDarray::assign(int x, int y, char c[]) {
        
        // existing values:
        
        // blank      0,  0,  0
        // red        255,0,  0
        // orange     255,165,0
        // yellow     255,255,0
        // green      0,  255,0
        // blue       0,  0,  255
        // purple     255,0,  255
        // white      255,255,255
        
        if(c=="red") {
                this->LEDstrip[y]->setPixelColor(x, color(255,0,0));
                return;
        }
        else if(c=="orange") {
                this->LEDstrip[y]->setPixelColor(x, color(255,165,0));
                return;
        }   
        else if(c=="yellow") {
                this->LEDstrip[y]->setPixelColor(x, color(255,255,0));
                return;
        }
        else if(c=="green") {
                this->LEDstrip[y]->setPixelColor(x, color(0,255,0));
                return;
        }
        else if(c=="blue") {
                this->LEDstrip[y]->setPixelColor(x, color(0,0,255));
                return;
        }
        else if(c=="purple") {
                this->LEDstrip[y]->setPixelColor(x, color(238,130,238));
                return;
        }
        else if(c=="white") {
                this->LEDstrip[y]->setPixelColor(x, color(255,255,255));
                return;
        }
        else if(c=="blank") {
                this->LEDstrip[y]->setPixelColor(x, color(0,0,0));
                return;
        }
                
}

// returns 32-bit integer of desired color
uint32_t LEDarray::color(byte r, byte g, byte b) {
        return 0x808080 | ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b;
}

// calls show() on all existing LED strips
void LEDarray::show(void) {
        for(int i=0; i < NUM_LED_STRIPS; i++)
                LEDstrip[i]->show();       
}

// calls show() on all existing LED strips
void LEDarray::quickshow(void) {
        for(int i=0; i < NUM_LED_STRIPS; i++)
                LEDstrip[i]->quickshow();       
}

// clears all LED values
void LEDarray::erase(void) {
        for(int i=0;i<STRIPLEN;i++)
                for(int j=0; j<NUM_LED_STRIPS; j++)
                        this->assign(i,j,"blank");
}

// shape constructor, takes all values at once
foregroundShape::foregroundShape(int Xcenter, int Ycenter, uint8_t img_pixels[], 
                                 int r, int g, int b, int Xvelocity, int Yvelocity, 
                                 int Xaccel, int Yaccel) {
        
        int k;
        
        this->Xcenter = Xcenter;
        this->Ycenter = Ycenter;
        
        for(int i=0; i<8; i++) {
                this->pixels[i] = img_pixels[i];
                if(this->pixels[i]>0) {
                        this->Yoff = i;
                }
                k=0;
                for(uint8_t j=0x80; j>0x00; j=j/2) {
                        if(((this->pixels[i])&j)>0) {
                                if(this->Xoff < k) {
                                        this->Xoff = k;
                                }
                        }
                        k++;
                }
        }
                
        this->r = r;
        this->g = g;
        this->b = b;
        this->Xvelocity = Xvelocity;
        this->Yvelocity = Yvelocity;
        this->Xaccel = Xaccel;
        this->Yaccel = Yaccel;
        
}

void foregroundShape::move() {
        
        this->Xcenter_old = this->Xcenter;
        this->Ycenter_old = this->Ycenter;
        
        if(this->Xvelocity > 0) {
                // object is moving right
                // shift it until it hits a wall
                for(int i=0; i<this->Xvelocity; i++) {
                        // if it hasn't hit a wall, move Xvelocity spaces right or 
                        // until it hits a wall
                        if(this->Xcenter+this->Xoff != 31) {
                                this->Xcenter++;
                        }
                        // if it hits a wall, reverse velocity and exit loop
                        else {
                                this->Xvelocity *= -1;
                                break;
                        }
                }
        }
        else if(this->Xvelocity < 0) {
                // object is moving left
                // shift it until it hits a wall
                for(int i=0; i<-1*this->Xvelocity; i++) {
                        // if it hasn't hit a wall, move Xvelocity spaces left or until 
                        // it hits a wall
                        if(this->Xcenter != 0) {
                                this->Xcenter--;
                        }
                        // if it hits a wall, reverse velocity and exit loop
                        else {
                                this->Xvelocity *= -1;
                                break;
                        }
                }
        }
        if(this->Yvelocity > 0) {
                // object is moving down
                // shift it until it hits a wall
                for(int i=0; i<this->Yvelocity; i++) {
                        // if it hasn't hit a wall, move Yvelocity spaces down or until 
                        // it hits a wall
                        if(this->Ycenter+this->Yoff != 9) {
                                this->Ycenter++;
                        }
                        // if it hits a wall, reverse velocity and exit loop
                        else {
                                this->Yvelocity *= -1;
                                break;
                        }
                }
        }
        else if(this->Yvelocity < 0) {
                // object is moving up
                // shift it until it hits a wall
                for(int i=0; i<-1*this->Yvelocity; i++) {
                        // if it hasn't hit a wall, move Yvelocity spaces up or until 
                        // it hits a wall
                        if(this->Ycenter != 0) {
                                this->Ycenter--;
                        }
                        // if it hits a wall, reverse velocity and exit loop
                        else {
                                this->Yvelocity *= -1;
                                break;
                        }
                }
        }
        // calculate acceleration
        this->Xvelocity += this->Xaccel;
        this->Yvelocity += this->Yaccel;
}

void foregroundShape::draw() {
        // use k as alternate iterator
        // probably a cleaner solution than this, don't care
        int k;
        //uint8_t j = 0x80;
        for(int i=0; i<8; i++) {
                k=0;
                for(uint8_t j=0x80; j>0x00; j=j/2) {
                        if(((this->pixels[i])&j)>0) {
                                if(isValidPixelCoordinate(this->Xcenter+k, this->Ycenter+i)) {
                                //if(isValidPixelCoordinate((this->Xcenter)+(7-k), this->Ycenter+i)) {
                                        //LEDarray.assign((this->Xcenter)+(7-k), this->Ycenter+i, "red");
                                        //LEDarray.assign(this->Xcenter_old, this->Ycenter_old, "blank");
                                        LEDarray.assign(this->Xcenter+k, this->Ycenter+i, this->r,this->g,this->b);
                                }
                        }
                        k++;
                }
        }
        
        // we no longer care about restoring the background
}

channelSpectrum::channelSpectrum(void) {
        
}

void channelSpectrum::init() {
        uint8_t Counter;

        //Setup pins to drive the spectrum analyzer. 
        pinMode(MSGEQ7_RESET, OUTPUT);
        pinMode(MSGEQ7_STROBE, OUTPUT);
        
        //Init spectrum analyzer
        digitalWrite(MSGEQ7_STROBE,LOW);
        delay(1);
        digitalWrite(MSGEQ7_RESET,HIGH);
        delay(1);
        digitalWrite(MSGEQ7_STROBE,HIGH);
        delay(1);
        digitalWrite(MSGEQ7_STROBE,LOW);
        delay(1);
        digitalWrite(MSGEQ7_RESET,LOW);
        delay(5);
}

void channelSpectrum::collect() {
        // Band 0 = Lowest Frequencies.
        uint8_t Band;
        
        for(Band=0; Band<7; Band++) {
                //Read twice and take the average by dividing by 2
                this->freqLeft[Band] = (analogRead(0) + analogRead(0))>>1; 
                digitalWrite(MSGEQ7_STROBE,HIGH);
                digitalWrite(MSGEQ7_STROBE,LOW);     
        }
        
        for(Band=0; Band<7; Band++) {
                //Read twice and take the average by dividing by 2
                this->freqRight[Band] = (analogRead(1) + analogRead(1))>>1; 
                digitalWrite(MSGEQ7_STROBE,HIGH);
                digitalWrite(MSGEQ7_STROBE,LOW);     
        }
}

void channelSpectrum::draw() {
        
          for(int j=0; j<7; j++) {
                  for(int i=0; i<10; i++) {
                          if(logScale[i] < maximum(freqLeft[j],freqRight[j])) {
                                  if(i<5) {
                                          LEDarray.assign((4*j)+1, 9-i, 0,((i+1)*25),127-((i+1)*25));
                                          LEDarray.assign((4*j)+2, 9-i, 0,((i+1)*25),127-((i+1)*25));
                                          LEDarray.assign((4*j)+3, 9-i, 0,((i+1)*25),127-((i+1)*25));
                                          //LEDarray.assign((3*j)+2, 9-i, "blue");
                                  }
                                  else if(i<10) {
                                          LEDarray.assign((4*j)+1, 9-i, (i-4)*25,127-((i-4)*25),0);
                                          LEDarray.assign((4*j)+2, 9-i, (i-4)*25,127-((i-4)*25),0);
                                          LEDarray.assign((4*j)+3, 9-i, (i-4)*25,127-((i-4)*25),0);
                                          //LEDarray.assign((3*j)+2, 9-i, "green");
                                  }
                                  /*else if(i<9) {
                                          LEDarray.assign((2*j)+1, 9-i, "yellow");
                                          LEDarray.assign((2*j)+2, 9-i, "yellow");
                                          //LEDarray.assign((3*j)+2, 9-i, "yellow");
                                  }
                                  else if(i<10) {
                                          LEDarray.assign((2*j)+1, 9-i, "red");
                                          LEDarray.assign((2*j)+2, 9-i, "red");
                                          //LEDarray.assign((3*j)+2, 9-i, "red");
                                  }*/
                          }
                          else {
                                          LEDarray.assign((4*j)+1, 9-i, "blank");
                                          LEDarray.assign((4*j)+2, 9-i, "blank");
                                          LEDarray.assign((4*j)+3, 9-i, "blank");
                                          //LEDarray.assign((3*j)+2, 9-i, "blank");
                          }
                  }
          }
        
          /*for(int j=0; j<7; j++) {
                  for(int i=0; i<10; i++) {
                          if(logScale[i]-120 < freqRight[j]) {
                                  if(i<5) {
                                          LEDarray.assign(30-((2*j)+1), 9-i, 0,((i+1)*25),127-((i+1)*25));
                                          LEDarray.assign(30-((2*j)+2), 9-i, 0,((i+1)*25),127-((i+1)*25));
                                          //LEDarray.assign((3*j)+2, 9-i, "blue");
                                  }
                                  else if(i<10) {
                                          LEDarray.assign(30-((2*j)+1), 9-i, (i-4)*25,127-((i-4)*25),0);
                                          LEDarray.assign(30-((2*j)+2), 9-i, (i-4)*25,127-((i-4)*25),0);
                                          //LEDarray.assign((3*j)+2, 9-i, "green");
                                  }
                                  //if(i<7) {
                                          LEDarray.assign(30-(2*j), 9-i, "blue");
                                          LEDarray.assign(30-(2*j)+1, 9-i, "blue");
                                          //LEDarray.assign(31-(3*j)+2, 9-i, "blue");
                                  }
                                  else if(i<8) {
                                          LEDarray.assign(30-(2*j), 9-i, "green");
                                          LEDarray.assign(30-(2*j)+1, 9-i, "green");
                                          //LEDarray.assign(31-(3*j)+2, 9-i, "green");
                                  }
                                  else if(i<9) {
                                          LEDarray.assign(30-(2*j), 9-i, "yellow");
                                          LEDarray.assign(30-(2*j)+1, 9-i, "yellow");
                                          //LEDarray.assign(31-(3*j)+2, 9-i, "yellow");
                                  }
                                  else if(i<10) {
                                          LEDarray.assign(30-(2*j), 9-i, "red");
                                          LEDarray.assign(30-(2*j)+1, 9-i, "red");
                                          //LEDarray.assign(31-(3*j)+2, 9-i, "red");
                                  }
                          }
                          else {
                                          LEDarray.assign(30-(2*j), 9-i, "blank");
                                          LEDarray.assign(30-(2*j)+1, 9-i, "blank");
                                          //LEDarray.assign(31-(3*j)+2, 9-i, "blank");
                          }
                  }
          }*/
          
}

///////////// GENERAL FUNCTIONS //////////////////

int isValidPixelCoordinate(int x, int y) {
        if(x>-1 && x<STRIPLEN && y>-1 && y<NUM_LED_STRIPS)
                return 1;
        else
                return 0;
}

// Read 7 band equalizer.
void readSpectrum()
{
  // Band 0 = Lowest Frequencies.
  uint8_t Band;
  for(Band=0;Band <7; Band++)
  {
    //Read twice and take the average by dividing by 2
    Spectrum[Band] = (analogRead(spectrumAnalog) + analogRead(spectrumAnalog) ) >>1; 
    digitalWrite(MSGEQ7_STROBE,HIGH);
    digitalWrite(MSGEQ7_STROBE,LOW);     
  }
}

// return maximum value
int maximum(int i, int j) {
        if(i<j)
                return j;
        else
                return i;
}

/////////////////////////////////////////////////
//                MAIN ROUTINES                //
/////////////////////////////////////////////////

void spectrumAnalyzer() {
        
        LEDarray.erase();
            
        //showSpectrum();
        //readSpectrum();
        channelSpectrum.collect();
        channelSpectrum.draw();
        //delay(15);  //We wait here for a little while until all the values to the LEDs are written out.
                      //This is being done in the background by an interrupt.
                      
        LEDarray.quickshow();
        
}

void textInput() {
        
        // read the incoming byte:
        incomingByte = Serial2.read();
        //Serial2.flush();
        
        if(incomingByte == 0xFFFFFFFF)
                return;

	// say what you got:
	Serial.print("I received: ");
        Serial.println(incomingByte, HEX);
        Serial.write(incomingByte);
        Serial.println();
        
                
        if(incomingByte == 0xEC || incomingByte == 0x76) {
                main_func = &spectrumAnalyzer;
                return;
        }
        
        foregroundShape shape1 = foregroundShape(0, 0,char_b ,  127,127,127, 0,0, 0,0);
        
        //while(1) {
        
                LEDarray.erase();


                
                shape1.draw();
                
                LEDarray.show();
                
                //delay(1000);
                
                LEDarray.erase();
                
                //delay(1000);
 
        //}     
        
        
}


