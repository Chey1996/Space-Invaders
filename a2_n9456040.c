/*  CAB202
*	Assignment 2
*
*	Mitchell Bourne, May 2016
*	Queensland University of Technology
*/

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "lcd.h"
#include "graphics.h"
#include "cpu_speed.h"
#include "sprite.h"
#include <stdbool.h>
#include <stdlib.h>

/*
* Definitions for the states of the buttons
*/
#define NUM_BUTTONS 6
#define BTN_DPAD_LEFT 0
#define BTN_DPAD_RIGHT 1
#define BTN_DPAD_UP 2
#define BTN_DPAD_DOWN 3
#define BTN_LEFT 4
#define BTN_RIGHT 5
#define BTN_STATE_UP 0
#define BTN_STATE_DOWN 1
#define WALL_WIDTH 5
#define LTHRES 500
#define RTHRES 500

/*
* Global variables
*/
//wall sprites
volatile Sprite wall_1_1[WALL_WIDTH];
volatile Sprite wall_1_2[WALL_WIDTH];
volatile Sprite wall_2_1[WALL_WIDTH];
volatile Sprite wall_2_2[WALL_WIDTH];
volatile Sprite wall_3_1[WALL_WIDTH];
volatile Sprite wall_3_2[WALL_WIDTH];
//alien direction changes
volatile int direction = 1;
volatile int direction2 = 1;
volatile int direction3 = 1;
volatile int y_direction1 = 0;
volatile int y_direction2 = 0;
volatile int y_direction3 = 0;
volatile int row_down_count = 0;
//menu & other counters
volatile int alien_rows = 0;
volatile int alien_bullet_number = 0;
volatile int bullet_place = 0;
volatile unsigned int press_count = 0;
volatile int lives_num = 3;
volatile int int_count = 0;
volatile int score_num = 0;
volatile int run_count = 0;
volatile int level_count = 0;
volatile int bullet_count = 0;
//player,bullet, alien sprites
volatile Sprite player;
volatile Sprite alien_row1[5];
volatile Sprite alien_row2[5];
volatile Sprite alien_row3[5];
volatile Sprite bullet[15];
volatile Sprite alien_bullet[15];
//button debouncing declarations
volatile unsigned char btn_hists[NUM_BUTTONS];
volatile unsigned char btn_states[NUM_BUTTONS];
//Inital player variables
volatile int heroX = LCD_X/2; //Movement of player x
volatile int heroY = LCD_Y-5; //Movement of player y
//bollean to run shoot bullet code
volatile bool shoot = false;
uint16_t adc_result0, adc_result1;
/*
* Global bitmaps
*/
unsigned char bullet_bitmap[] = {
  0b00000010,
  0b00000010,
  0b00000010
};
unsigned char wall_bitmap[] = {
  0b00000001,
};
unsigned char player_bitmap[] = {
  0b00011010,
  0b00111100,
  0b01100110,
  0b11000011,
  0b00111100
};
unsigned char aliens[] = {
  0b01000010,
  0b10011001,
  0b01111110,
  0b10011001,
  0b01000010
};

/*
* Function declarations
*/
//wall functions
void wall_draw(void);
void init_walls(void);
void wall_collision_player(void);
void wall_collision_alien(void);
//level functions
void level_3_run(void);
void level_2_run(void);
void level_1_run(void);
//setup functions
void init_hardware(void);
void screen_setup(void);
//movement, collision
void alien_collision(void);
void alien_movement_1(void);
void alien_movement_2(void);
//draw, movement
void bullet_curve(void);
void draw_move_sprites(void);
void draw_move_sprites_3(void);
//init sprites
void init_player_alien_sprite(void);

void adc_init();
void adcresults();


/*
* Main
*/
int main() {
    //set clock speed
    set_clock_speed(CPU_8MHz);

    adc_init();
    //init lcd screen
    lcd_init(LCD_LOW_CONTRAST);
    //clear pixels
    clear_screen();
    //init player, alien sprite
    init_player_alien_sprite();
    //init wall sprites
    init_walls();

    // Setup the hardware/set up clock variables
    init_hardware();

    // Run the main loop
    while (1){
      //reset menu/level navigation variables when maxed
      if(press_count == 15){
        press_count = 0;
      }
      if(level_count == 3){
        level_count = 0;
      }
      if(run_count == 4){
        run_count = 1;
      }

      clear_screen();
      if (level_count == 0) { //draw level 1
        clear_screen();
        draw_string(20 , 3, "Level 1");
        draw_string(8,11, "Press LDPAD to");
        draw_string(8,19, "change level.");
        draw_string(8,27, "Press RDPAD to");
        draw_string(16, 35, "start.");
      }
      if (level_count == 1) { //draw level 2
        clear_screen();
        draw_string(20 , 3, "Level 2");
        draw_string(8,11, "Press LDPAD to");
        draw_string(8,19, "change level.");
        draw_string(8,27, "Press RDPAD to");
        draw_string(16, 35, "start.");
      }
      if (level_count == 2) { //draw level 3
        clear_screen();
        draw_string(20 , 3, "Level 3");
        draw_string(8,11, "Press LDPAD to");
        draw_string(8,19, "change level.");
        draw_string(8,27, "Press RDPAD to");
        draw_string(16, 35, "start.");
      }
      if (level_count == 0 && run_count == 1) { //run level 1
          clear_screen();
          level_1_run();
          screen_setup();
      }
      if (level_count == 1 && run_count == 2) { //run level 2
          clear_screen();
          level_2_run();
          screen_setup();
      }
      if (level_count == 2 && run_count == 3) { //run level 3
         clear_screen();
         level_3_run();
         screen_setup();
      }
      if (score_num == 15 || lives_num == 0) {
        clear_screen();
        while (1) {
          draw_string(8, 11, "Game Over");
          draw_string(8, 19, "Restart teensy");
          draw_string(8, 27, "to play agian.");
          show_screen();
        }
        }

      show_screen();
      // Have a rest
      _delay_ms(25);

    }

    // We'll never get here...
    return 0;
}

/*
* Functions
*/

void adc_init()
{
    // AREF = AVcc
    ADMUX = (1<<REFS0);

    // ADC Enable and pre-scaler of 128
    // 8000000/128 = 62500
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}

// read adc value
uint16_t adc_read(uint8_t ch)
{
    // select the corresponding channel 0~7
    // ANDing with '7' will always keep the value
    // of 'ch' between 0 and 7
    ch &= 0b00000111;  // AND operation with 7
    ADMUX = (ADMUX & 0xF8)|ch;     // clears the bottom 3 bits before ORing

    // start single conversion
    // write '1' to ADSC
    ADCSRA |= (1<<ADSC);

    // wait for conversion to complete
    // ADSC becomes '0' again
    // till then, run loop continuously
    while(ADCSRA & (1<<ADSC));

    return (ADC);
}



 void level_1_run(void) {
 shoot = false;
 if(btn_states[BTN_LEFT] == BTN_STATE_DOWN ){ //left button player movement
   player.x = player.x - 1;
     if (player.x == 0) {
       player.x = player.x + 1;
     }
   }
  if(btn_states[BTN_RIGHT] == BTN_STATE_DOWN ){ //right button player movement
     player.x = player.x + 1;
     if (player.x == LCD_X - 8) {
       player.x = player.x - 1;
     }
   }
   if(btn_states[BTN_DPAD_UP] == BTN_STATE_DOWN){ //shoot
      shoot = true;
   }
   if(shoot == true){ //draw bullets
      init_sprite((Sprite*)&bullet[press_count],player.x,(player.y+1),8,3,bullet_bitmap);
   }
   alien_movement_1();
   alien_collision();
   draw_move_sprites();
}

void level_2_run(void) {
  adcresults();
  if(player.x > 75){
  player.x--;
}
if(player.x<1){
  player.x++;
}
if (adc_result1 >= 450 ){
  player.x = player.x +1;
} else {
  player.x = player.x;
}
if(adc_result1 <= 50){
  player.x = player.x -1;
}else{
  player.x = player.x;
}

  shoot = false;

  // player.x = player.x - 1; //default move left
  // if (PINF & (1<<1)) { //if potentiometer move right
  //   player.x = player.x + 2;
  // }
  // if (player.x == 0) {
  //   player.x = player.x + 1;
  // }
  // if (player.x == LCD_X - 8) {
  //   player.x = player.x - 1;
  // }
  if(btn_states[BTN_DPAD_UP] == BTN_STATE_DOWN){
    shoot = true;
  }
  if(shoot == true){
    init_sprite((Sprite*)&bullet[press_count],player.x,(player.y+1),8,3,bullet_bitmap);
  }
  alien_movement_2();
  alien_collision();
  wall_collision_player();
  wall_collision_alien();
  draw_move_sprites();
  wall_draw();
}

void level_3_run(void) {
adcresults();
if(player.x > 75){
player.x--;
}
if(player.x<1){
player.x++;
}
if (adc_result1 >= 450 ){
player.x = player.x +1;
} else {
player.x = player.x;
}
if(adc_result1 <= 50){
player.x = player.x -1;
}else{
player.x = player.x;
}

  shoot = false;
  // player.x = player.x - 1;
  // if (PINF & (1<<1)) {
  //   player.x = player.x + 2;
  // }
  // if (player.x == 0) {
  //   player.x = player.x + 1;
  // }
  // if (player.x == LCD_X - 8) {
  //   player.x = player.x - 1;
  //}
  if(btn_states[BTN_DPAD_UP] == BTN_STATE_DOWN){
    shoot = true;
  }
  if(shoot == true){
    init_sprite((Sprite*)&bullet[press_count],player.x,(player.y+1),8,3,bullet_bitmap);
  }
  alien_movement_1();
  alien_collision();
  wall_collision_player();
  wall_collision_alien();
  for (int i = 0; i <5; i++) {
    if (row_down_count == 0) {
      y_direction3 = 1;
      direction2 = 1;
    }
    if (row_down_count == 1) {
      y_direction2 = 1;
      direction = 1;
    }
    if (row_down_count == 2) {
      y_direction1 = 1;
      direction3 = 1;
    }
    if (row_down_count == 3) {
      y_direction1 = -1;
      direction = -1;
    }
    if (row_down_count == 4) {
      y_direction2 = -1;
      direction2 = -1;
    }
    if (row_down_count == 5) {
      y_direction3 = -1;
      direction3 = 1;
    }
    alien_row1[i].y =   alien_row1[i].y + y_direction1;
    alien_row2[i].y =   alien_row2[i].y + y_direction2;
    alien_row3[i].y =   alien_row3[i].y + y_direction3;
    if (alien_row1[i].y == 8) {
      alien_row1[i].y = alien_row1[i].y + 1;
    }
    if (alien_row1[i].y == (LCD_Y/2) - 10) {
      alien_row1[i].y = alien_row1[i].y - 1;
    }
    if (alien_row2[i].y == 13) {
      alien_row2[i].y = alien_row2[i].y + 1;
    }
    if (alien_row2[i].y == (LCD_Y/2) - 5) {
      alien_row2[i].y = alien_row2[i].y - 1;
    }
    if (alien_row3[i].y == 18) {
      alien_row3[i].y = alien_row3[i].y + 1;
    } //5 above mid
    if (alien_row3[i].y == (LCD_Y/2) - 0) {
      alien_row3[i].y = alien_row3[i].y - 1;
  }
  }
  draw_move_sprites_3();
  wall_draw();
}

void adcresults(){
  adc_result0 = adc_read(0);      // read adc value at PA0
  adc_result1 = adc_read(1);      // read adc value at PA1


// condition for led to glow
  if (adc_result0 < LTHRES)
      PORTD |= 1<<PIND6;
  else
      PORTD = 0x00;

  //clear pixels
}


//draw player/aliens
void init_player_alien_sprite() {
  init_sprite((Sprite*) &player, heroX -4, heroY, 8, 5, player_bitmap); //draw player sprite

  //draw initial aliens
  int xcor[] = {5,15,25,35,45};
  int xcor2[] = {5+10,15+10,25+10,35+10,45+10};
  int xcor3[] = {5+30,15+30,25+30,35+30,45+30};
  for(int i = 0 ; i < 5 ; i ++ ){
  init_sprite((Sprite*)&alien_row1[i],xcor2[i],8,8,5,aliens);
  init_sprite((Sprite*)&alien_row2[i],xcor[i],13,8,5,aliens);
  init_sprite((Sprite*)&alien_row3[i],xcor3[i],18,8,5,aliens);
  }
}

//UI setup & update
void screen_setup(){
//Sprite setups
Sprite score;
Sprite level;
Sprite lives;
unsigned char score_bitmap[] = {
   0b01110011, 0b10011100, 0b11110011, 0b11011000,
   0b10000100, 0b00100010, 0b10001010, 0b00011000,
   0b01100100, 0b00100010, 0b11110011, 0b11000000,
   0b00010100, 0b00100010, 0b10010010, 0b00011000,
   0b11100011, 0b10011100, 0b10001011, 0b11011000
};
unsigned char lives_bitmap[] = {
  0b10000000,
  0b10000000,
  0b10000000,
  0b10000000,
  0b11111000
};
unsigned char level_bitmap[] = {
  0b10000100,
  0b10000100,
  0b10000100,
  0b10000100,
  0b11100111
};

//draw score label
init_sprite(&score, 0, 0, 29, 5, score_bitmap);
draw_sprite(&score);

//update score value
if (score_num == 0) {
  draw_string(30, 0, "0");
}
if (score_num == 1) {
  draw_string(30, 0, "1");
}
if (score_num == 2) {
  draw_string(30, 0, "2");
}
if (score_num == 3) {
  draw_string(30, 0, "3");
}
if (score_num == 4) {
  draw_string(30, 0, "4");
}
if (score_num == 5) {
  draw_string(30, 0, "5");
}
if (score_num == 6) {
  draw_string(30, 0, "6");
}
if (score_num == 7) {
  draw_string(30, 0, "7");
}
if (score_num == 8) {
  draw_string(30, 0, "8");
}
if (score_num == 9) {
  draw_string(30, 0, "9");
}
if (score_num == 10) {
  draw_string(30, 0, "10");
}
if (score_num == 11) {
  draw_string(30, 0, "11");
}
if (score_num == 12) {
  draw_string(30, 0, "12");
}
if (score_num == 13) {
  draw_string(30, 0, "13");
}
if (score_num == 14) {
  draw_string(30, 0, "14");
}

//draw lives label
init_sprite(&lives, 42, 0, 6, 5, lives_bitmap);
draw_char(48,-1,':');
draw_sprite(&lives);

//update lives value
if (lives_num == 3) {
  draw_string(52, 0, "3");
}
if (lives_num == 2) {
  draw_string(52, 0, "2");
}
if (lives_num == 1) {
  draw_string(52, 0, "1");
}

//draw level label
init_sprite(&level, 62, 0, 8, 5, level_bitmap);
draw_char(70,-1,':');
draw_sprite(&level);

//update level value
if (level_count == 0) {
  draw_string(74,0, "1");
}
if (level_count == 1) {
  draw_string(74,0, "2");
}
if (level_count == 2) {
  draw_string(74,0, "3");
}

//draw ui line
draw_line(0,7,84,7);
}

void draw_move_sprites_3 () {
  //move player bullets & draw
  for(int i = 0 ; i < 15 ; i ++ ){
    if(bullet[i].y == 6){
      bullet[i].x = 100;
    }
    draw_sprite((Sprite*)&bullet[i]);
    }

  //move alien bullets & draw
  for(int i = 0 ; i < 15 ; i ++ ){
    if(  alien_bullet[i].y > 5  ) {
      //move alien bullets down
    alien_bullet[i].y = alien_bullet[i].y + 1;
    }
    //remove alien bullets
    if(alien_bullet[i].y == LCD_Y){
    alien_bullet[i].x = 100;
    }
    draw_sprite((Sprite*)&alien_bullet[i]);
    }

   //draw aliens
   for(int i = 0 ; i < 5 ; i ++ ){
     if ((alien_row1[i].x) < 0 || (alien_row1[i].x) > 85) {
       alien_row1[i].x = -100;
     }
     if ((alien_row2[i].x) < 0 || (alien_row2[i].x) > 85) {
       alien_row2[i].x = -100;
     }
     if ((alien_row3[i].x < 0) || (alien_row3[i].x) > 85) {
       alien_row3[i].x = -100;
     }
     draw_sprite((Sprite*)&alien_row1[i]);
     draw_sprite((Sprite*)&alien_row2[i]);
     draw_sprite((Sprite*)&alien_row3[i]);
   }

   for (int i = 0; i < 15; i++) {
     if (adc_result0 >= 450 ){
       bullet[i].y = bullet[i].y - .25;
       bullet[i].x = bullet[i].x + 1;
     } else {
       bullet[i].y = bullet[i].y - .25;
       bullet[i].x = bullet[i].x;
     }
     if(adc_result0 <= 50){
       bullet[i].y = bullet[i].y - .25;
       bullet[i].x = bullet[i].x - 2;
     }else{
       bullet[i].y = bullet[i].y - .25;
       bullet[i].x = bullet[i].x;
   }
 }

  //  //default bullets movement
  //  for (int i = 0; i < 15; i++) {
  //    bullet[i].x = bullet[i].x - .5;
  //    bullet[i].y = bullet[i].y - 1;
  //  }
  //  //potentiometer change curvature
  //  if (PINF & (1<<0)) {
  //    for (int i = 0; i < 15; i++) {
  //      bullet[i].x = bullet[i].x + 1;
  //      bullet[i].y = bullet[i].y - 1;
  //    }
  //  }

  draw_sprite((Sprite*) &player);
}

void draw_move_sprites() { //minus potentiometer
  //move player bullets & draw
    for(int i = 0 ; i < 15 ; i ++ ){
      if(  bullet[i].y > 5  ) {
        bullet[i].y = bullet[i].y -1;
    }
    if(bullet[i].y == 6){
      bullet[i].x = 100;
    }
    draw_sprite((Sprite*)&bullet[i]);
    }

  //move alien bullets & draw
  for(int i = 0 ; i < 15 ; i ++ ){
    if(  alien_bullet[i].y > 5  ) {
    alien_bullet[i].y = alien_bullet[i].y + 1;
    }
    if(alien_bullet[i].y == LCD_Y){
    alien_bullet[i].x = 100;
    }
    draw_sprite((Sprite*)&alien_bullet[i]);
    }

   //draw aliens
   for(int i = 0 ; i < 5 ; i ++ ){
     if ((alien_row1[i].x) < 0 || (alien_row1[i].x) > 84) {
       alien_row1[i].x = -100;
     }
     if ((alien_row2[i].x) < 0 || (alien_row2[i].x) > 84) {
       alien_row2[i].x = -100;
     }
     if ((alien_row3[i].x) < 0 || (alien_row3[i].x) > 84) {
       alien_row3[i].x = -100;
     }
     draw_sprite((Sprite*)&alien_row1[i]);
     draw_sprite((Sprite*)&alien_row2[i]);
     draw_sprite((Sprite*)&alien_row3[i]);
   }


  draw_sprite((Sprite*) &player);
  }

void init_walls() {
      //init wall sprites
      init_sprite((Sprite*)&wall_1_1[0], 12, 35, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_1_1[1], 13, 35, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_1_1[2], 14, 35, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_1_1[3], 15, 35, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_1_1[4], 16, 35, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_1_2[0], 12, 34, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_1_2[1], 13, 34, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_1_2[2], 14, 34, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_1_2[3], 15, 34, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_1_2[4], 16, 34, 8, 1, wall_bitmap);

      init_sprite((Sprite*)&wall_2_1[0], 32, 35, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_2_1[1], 33, 35, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_2_1[2], 34, 35, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_2_1[3], 35, 35, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_2_1[4], 36, 35, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_2_2[0], 32, 34, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_2_2[1], 33, 34, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_2_2[2], 34, 34, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_2_2[3], 35, 34, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_2_2[4], 36, 34, 8, 1, wall_bitmap);

      init_sprite((Sprite*)&wall_3_1[0], 52, 35, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_3_1[1], 53, 35, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_3_1[2], 54, 35, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_3_1[3], 55, 35, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_3_1[4], 56, 35, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_3_2[0], 52, 34, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_3_2[1], 53, 34, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_3_2[2], 54, 34, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_3_2[3], 55, 34, 8, 1, wall_bitmap);
      init_sprite((Sprite*)&wall_3_2[4], 56, 34, 8, 1, wall_bitmap);
}

void alien_movement_1() {
  ///define alien directions (keep on screen)
      for (int i = 0; i < 5; i++) { //set side way limits for each seperate sprite
        if (i == 0) {
           if (alien_row1[i].x == 1) {
             direction = 1;
           }
           if (alien_row1[i].x == 44) {
             direction = -1;
           }
           if (alien_row2[i].x == 1) {
             direction2 = 1;
           }
           if (alien_row2[i].x == 44) {
             direction2 = -1;
           }
           if (alien_row3[i].x == 1) {
             direction3 = 1;
           }
           if (alien_row3[i].x == 44) {
             direction3 = -1;
           }
        }
        if (i == 1) {
           if (alien_row1[i].x == 9) {
             direction = 1;
           }
           if (alien_row1[i].x == 52) {
             direction = -1;
           }
           if (alien_row2[i].x == 9) {
             direction2 = 1;
           }
           if (alien_row2[i].x == 52) {
             direction2 = -1;
           }
           if (alien_row3[i].x == 9) {
             direction3 = 1;
           }
           if (alien_row3[i].x == 52) {
             direction3 = -1;
           }
        }
        if (i == 2) {
           if (alien_row1[i].x == 17) {
             direction = 1;
           }
           if (alien_row1[i].x == 60) {
             direction = -1;
           }
           if (alien_row2[i].x == 17) {
             direction2 = 1;
           }
           if (alien_row2[i].x == 60) {
             direction2 = -1;
           }
           if (alien_row3[i].x == 17) {
             direction3 = 1;
           }
           if (alien_row3[i].x == 60) {
             direction3 = -1;
           }
        }
        if (i == 3) {
           if (alien_row1[i].x == 25) {
             direction = 1;
           }
           if (alien_row1[i].x == 68) {
             direction = -1;
           }
           if (alien_row2[i].x == 25) {
             direction2 = 1;
           }
           if (alien_row2[i].x == 68) {
             direction2 = -1;
           }
           if (alien_row3[i].x == 25) {
             direction3 = 1;
           }
           if (alien_row3[i].x == 68) {
             direction3 = -1;
           }
        }
        if (i == 4) {
           if (alien_row1[i].x == 33) {
             direction = 1;
           }
           if (alien_row1[i].x == 76) {
             direction = -1;
           }
           if (alien_row2[i].x == 33) {
             direction2 = 1;
           }
           if (alien_row2[i].x == 76) {
             direction2 = -1;
           }
           if (alien_row3[i].x == 33) {
             direction3 = 1;
           }
           if (alien_row3[i].x == 76) {
             direction3 = -1;
           }
           }
         }

          //use the changed direction to change x variable of alien
  for(int i = 0 ; i < 5 ; i ++ ){
    alien_row1[i].x =   alien_row1[i].x + direction;
    alien_row2[i].x =   alien_row2[i].x + direction2;
    alien_row3[i].x =   alien_row3[i].x + direction3;
  }
}

void alien_movement_2() { //above with y movement
  alien_movement_1();
  for (int i = 0; i < 5; i++) {
      if (alien_row1[i].y == 8) {
        y_direction1 = 1;
      }
      if (alien_row1[i].y == (LCD_Y/2) - 10) {
        y_direction1 = -1;
      }
      if (alien_row2[i].y == 13) {
        y_direction2 = 1;
      }
      if (alien_row2[i].y == (LCD_Y/2) - 5) {
        y_direction2 = -1;
      }
      if (alien_row3[i].y == 18) {
        y_direction3 = 1;
      } //5 above mid
      if (alien_row3[i].y == (LCD_Y/2) - 0) {
        y_direction3 = -1;
    }
    alien_row1[i].y =   alien_row1[i].y + y_direction1;
    alien_row2[i].y =   alien_row2[i].y + y_direction2;
    alien_row3[i].y =   alien_row3[i].y + y_direction3;
}
}

void wall_collision_player() {
  for (int i = 0; i < 15; i++) { //bullets
    for (int j = 0; j < 5; j++) { //walls
       if (((bullet[i].y)) <= (wall_1_1[j].y) && ((bullet[i].y)) >= (wall_1_1[j].y - 1)) {
         if (((bullet[i].x) + 7) <= ((wall_1_1[j].x) + 8) && ((bullet[i].x) + 7) >= ((wall_1_1[j].x) + 7)) {
           bullet[i].x = -300;
           bullet[i].y = 300;
           wall_1_1[j].x = 500;
         }
         if (((bullet[i].x) + 7) <= ((wall_2_1[j].x) + 8) && ((bullet[i].x) + 7) >= ((wall_2_1[j].x) + 7)) {
           bullet[i].x = -300;
           bullet[i].y = 300;
           wall_2_1[j].x = 500;
         }
         if (((bullet[i].x) + 7) <= ((wall_3_1[j].x) + 8) && ((bullet[i].x) + 7) >= ((wall_3_1[j].x) + 7)) {
           bullet[i].x = -300;
           bullet[i].y = 300;
           wall_3_1[j].x = 500;
         }
       }
    if (((bullet[i].y)) <= (wall_1_2[j].y) && ((bullet[i].y)) >= (wall_1_2[j].y - 1)) {
      if (((bullet[i].x) + 7) <= ((wall_1_2[j].x) + 8) && ((bullet[i].x) + 7) >= ((wall_1_2[j].x) + 7)) {
        bullet[i].x = -300;
        bullet[i].y = 300;
        wall_1_2[j].x = 500;
      }
      if (((bullet[i].x) + 7) <= ((wall_2_2[j].x) + 8) && ((bullet[i].x) + 7) >= ((wall_2_2[j].x) + 7)) {
        bullet[i].x = -300;
        bullet[i].y = 300;
        wall_2_2[j].x = 500;
      }
      if (((bullet[i].x) + 7) <= ((wall_3_2[j].x) + 8) && ((bullet[i].x) + 7) >= ((wall_3_2[j].x) + 7)) {
        bullet[i].x = -300;
        bullet[i].y = 300;
        wall_3_2[j].x = 500;
      }
    }
   }
  }
}

  void wall_collision_alien() { //alien bullet collision start
    for (int i = 0; i < 15; i++) { //bullets
      for (int j = 0; j < 5; j++) { //walls
         if (((alien_bullet[i].y) + 2) <= (wall_1_1[j].y) && ((alien_bullet[i].y) + 2) >= (wall_1_1[j].y)) {
           if (((alien_bullet[i].x) + 7) <= ((wall_1_1[j].x) + 8) && ((alien_bullet[i].x) + 7) >= ((wall_1_1[j].x) + 8)) {
             alien_bullet[i].x = -300;
             alien_bullet[i].y = 300;
             wall_1_1[j].x = 500;
           }
           if (((alien_bullet[i].x) + 7) <= ((wall_2_1[j].x) + 8) && ((alien_bullet[i].x) + 7) >= ((wall_2_1[j].x) + 8)) {
             alien_bullet[i].x = -300;
             alien_bullet[i].y = 300;
             wall_2_1[j].x = 500;
           }
           if (((alien_bullet[i].x) + 7) <= ((wall_3_1[j].x) + 8) && ((alien_bullet[i].x) + 7) >= ((wall_3_1[j].x) + 8)) {
             alien_bullet[i].x = -300;
             alien_bullet[i].y = 300;
             wall_3_1[j].x = 500;
           }
         }
      if (((alien_bullet[i].y) + 2) <= (wall_1_2[j].y) && ((alien_bullet[i].y) + 2) >= (wall_1_2[j].y)) {
        if (((alien_bullet[i].x) + 7) <= ((wall_1_2[j].x) + 8) && ((alien_bullet[i].x) + 7) >= ((wall_1_2[j].x) + 8)) {
          alien_bullet[i].x = -300;
          alien_bullet[i].y = 300;
          wall_1_2[j].x = 500;
        }
        if (((alien_bullet[i].x) + 7) <= ((wall_2_2[j].x) + 8) && ((alien_bullet[i].x) + 7) >= ((wall_2_2[j].x) + 8)) {
          alien_bullet[i].x = -300;
          alien_bullet[i].y = 300;
          wall_2_2[j].x = 500;
        }
        if (((alien_bullet[i].x) + 7) <= ((wall_3_2[j].x) + 8) && ((alien_bullet[i].x) + 7) >= ((wall_3_2[j].x) + 8)) {
          alien_bullet[i].x = -300;
          alien_bullet[i].y = 300;
          wall_3_2[j].x = 500;
        }
      }
     }
    }
  }

void alien_collision() {
  //for all bullets and aleins
  for (int i = 0; i < 15; i++) {
    if (bullet[i].y > LCD_Y || bullet[i].y < 0) {
      bullet[i].x = -300;
    }
    for (int j = 0; j < 5; j++) {
      //player killing aliens
      if ((bullet[i].y) >= (alien_row1[j].y) && (bullet[i].y) <= (alien_row1[j].y) + 5) { //bullet sprite on alien sprite
        if ((bullet[i].x) + 7 >= (alien_row1[j].x) && (bullet[i].x) + 7 <= (alien_row1[j].x) + 8) {
          bullet[i].x = -300; //remove bullets and alien
          bullet[i].y = 300;
          alien_row1[j].x = 500;
          alien_row1[j].y = -500;
          score_num = score_num + 1; //add to score
        }
      }
      if ((bullet[i].y) >= (alien_row2[j].y) && (bullet[i].y) <= (alien_row2[j].y) + 5) {
        if ((bullet[i].x) + 7 >= (alien_row2[j].x) && (bullet[i].x) + 7 <= (alien_row2[j].x) + 8) {
          bullet[i].x = -300;
          bullet[i].y = 300;
          alien_row2[j].x = 500;
          alien_row2[j].y = -500;
          score_num = score_num + 1;
        }
      }
      if ((bullet[i].y) >= (alien_row3[j].y) && (bullet[i].y) <= (alien_row3[j].y) + 5) {
        if ((bullet[i].x) + 7 >= (alien_row3[j].x) && (bullet[i].x) + 7 <= (alien_row3[j].x) + 8) {
          bullet[i].x = -500;
          bullet[i].y = 500;
          alien_row3[j].x = 500;
          alien_row3[j].y = -500;
          score_num = score_num + 1;
        }
      }
    }
    //player dieing to aliens
    if ((alien_bullet[i].y + 2 >= player.y) && (alien_bullet[i].y + 2 <= player.y + 5)) { //alien bullet on player sprite
      if ((alien_bullet[i].x + 7 >= player.x) && (alien_bullet[i].x + 7 <= player.x + 8)) {
        lives_num = lives_num - 1; //minus a life
        alien_bullet[i].x = -300;
        alien_bullet[i].y = 300;
        _delay_ms(500); //pause so they can react
      }
    }
  }
}

void wall_draw() {

//draw first wall
  draw_sprite((Sprite*)&wall_1_2[4]);
  draw_sprite((Sprite*)&wall_1_2[3]);
  draw_sprite((Sprite*)&wall_1_2[2]);
  draw_sprite((Sprite*)&wall_1_2[1]);
  draw_sprite((Sprite*)&wall_1_2[0]);
  draw_sprite((Sprite*)&wall_1_1[4]);
  draw_sprite((Sprite*)&wall_1_1[3]);
  draw_sprite((Sprite*)&wall_1_1[2]);
  draw_sprite((Sprite*)&wall_1_1[1]);
  draw_sprite((Sprite*)&wall_1_1[0]);

//draw middle wall
  draw_sprite((Sprite*)&wall_2_2[4]);
  draw_sprite((Sprite*)&wall_2_2[3]);
  draw_sprite((Sprite*)&wall_2_2[2]);
  draw_sprite((Sprite*)&wall_2_2[1]);
  draw_sprite((Sprite*)&wall_2_2[0]);
  draw_sprite((Sprite*)&wall_2_1[4]);
  draw_sprite((Sprite*)&wall_2_1[3]);
  draw_sprite((Sprite*)&wall_2_1[2]);
  draw_sprite((Sprite*)&wall_2_1[1]);
  draw_sprite((Sprite*)&wall_2_1[0]);

//draw right wall
  draw_sprite((Sprite*)&wall_3_2[4]);
  draw_sprite((Sprite*)&wall_3_2[3]);
  draw_sprite((Sprite*)&wall_3_2[2]);
  draw_sprite((Sprite*)&wall_3_2[1]);
  draw_sprite((Sprite*)&wall_3_2[0]);
  draw_sprite((Sprite*)&wall_3_1[4]);
  draw_sprite((Sprite*)&wall_3_1[3]);
  draw_sprite((Sprite*)&wall_3_1[2]);
  draw_sprite((Sprite*)&wall_3_1[1]);
  draw_sprite((Sprite*)&wall_3_1[0]);
}

void init_hardware(void) {

    // TIMER0 CODE
    TCCR0B &= ~((1<<WGM02)); // Making sure timer0 is in normal mode
    TCCR0B |= (1<<CS12); //setting pre scaler to 1024
    //enable interuupt
    TIMSK0 |= (1 << TOIE1);

    //TIMER1 CODE
    TCCR1B &= ~((1<<WGM12));  //Set to Normal Timer Mode using TCCR1B
    // Set the prescaler for TIMER1 so that the clock overflows every ~2.1 seconds
    TCCR1B |= ((1<<CS02));
    // enable interrupt overflow
    TIMSK1 |= (1 << TOIE1);

    sei(); //Global interrupts

    //Turn lights on teensy
    DDRC |=(1<<7); PORTC|=(1<<7);

}

//Interupt timer ISR for timer 0
ISR(TIMER0_OVF_vect){
// Counters for buttons down
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if(btn_hists[i]==0b11111111){
      btn_states[i] = BTN_STATE_DOWN;
    }else{btn_states[i] = BTN_STATE_UP;}
      btn_hists[i] = (btn_hists[i]<<1);//shifts each byte left 1 placed
    }
    //Up dpad down add to press count
    if(btn_hists[BTN_DPAD_UP]==0b10000000){
      press_count++;
      alien_rows = alien_rows + 1;
      if (alien_rows == 3) {
        alien_rows = 0;
      }
      alien_bullet_number = alien_bullet_number + 1;
      if (alien_bullet_number == 5) {
        alien_bullet_number = 0;
      }
      bullet_place = bullet_place +1;
      if (bullet_place == 8) {
        bullet_place = 0;
      }
    }
    //Left dpad down add to level count
    if(btn_hists[BTN_DPAD_LEFT]==0b10000000){
      level_count++;
    }
    //Adjust run count to level selection
    if(btn_hists[BTN_DPAD_RIGHT]==0b10000000){
      if (level_count == 1) {
        run_count = 1;
      }
      if (level_count == 2) {
        run_count = 2;
      }
      run_count++;
    }

//Button debouncing
  if((PINF>>PIN5)&1){
    btn_hists[BTN_RIGHT] ^= (1<<0);
  }
  if((PINF>>PIN6)&1){
    btn_hists[BTN_LEFT] ^= (1<<0);
  }
  if((PINB>>PIN7)&1){
    btn_hists[BTN_DPAD_DOWN] ^= (1<<0);
  }
  if((PINB>>PIN1)&1){
    btn_hists[BTN_DPAD_LEFT] ^= (1<<0);
  }
  if((PIND>>PIN1)&1){
    btn_hists[BTN_DPAD_UP] ^= (1<<0);
  }
  if((PIND>>PIN0)&1){
    btn_hists[BTN_DPAD_RIGHT] ^= (1<<0);
  }
}

ISR(TIMER1_OVF_vect) {
  //reset independant movement counter
  if (level_count == 2 && run_count == 3) {
    if (row_down_count == 6) {
      row_down_count = -1;
    }

  row_down_count =  row_down_count + 1;
  }


  if (alien_rows == 2) { //init a random bullet location from values above
    init_sprite((Sprite*)&alien_bullet[int_count],alien_row1[alien_bullet_number].x + bullet_place,alien_row1[alien_bullet_number].y + 5 ,8,3,bullet_bitmap);
    init_sprite((Sprite*)&alien_bullet[int_count + 1],alien_row2[alien_bullet_number].x + bullet_place,alien_row2[alien_bullet_number].y + 5,8,3,bullet_bitmap);
  }
  if (alien_rows == 1) {
      init_sprite((Sprite*)&alien_bullet[int_count],alien_row3[alien_bullet_number].x + bullet_place,alien_row3[alien_bullet_number].y + 5,8,3,bullet_bitmap);
      init_sprite((Sprite*)&alien_bullet[int_count + 1],alien_row1[alien_bullet_number].x + bullet_place,alien_row1[alien_bullet_number].y + 5 ,8,3,bullet_bitmap);
  }
  if (alien_rows == 0) {
      init_sprite((Sprite*)&alien_bullet[int_count],alien_row2[alien_bullet_number].x + bullet_place,alien_row2[alien_bullet_number].y + 5 ,8,3,bullet_bitmap);
      init_sprite((Sprite*)&alien_bullet[int_count + 1],alien_row3[alien_bullet_number].x + bullet_place,alien_row3[alien_bullet_number].y + 5 ,8,3,bullet_bitmap);
  }
  int_count = int_count + 3;
  if (int_count >= 15) {
    int_count = 0;
  }
}
