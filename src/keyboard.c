#include "keyboard.h"
#include "isr.h"
#include "monitor.h"

#include "keyboard.h"

Keyboard keyboard = {{0x60}, {0x64}};

static void keyboard_callback(registers_t* regs) {
  keyboard.pressed();
}

KeyboardKeyType Keyboard::mapping[128] = 
{
/* 0x0 - 0xe */
/* unk   esc   1    2   3    4    5    6    7    8    9    0     -   +    del */
   none, none, reg, reg, reg, reg, reg, reg, reg, reg, reg, reg, reg, reg, reg,

/* 0xf - 0x1c */
/* tab,  q,   w,   e,   r,   t,   y,   u,   i,   o,   p,   {,   },  enter */
   reg,  reg, reg, reg, reg, reg, reg, reg, reg, reg, reg, reg, reg, enter,

/* 0x1d - 0x29 */
/* ctrl, a,   s,   d,   f,   g,   h,   j,   k,   l,   :,   ',   ` */
   ctrl, reg, reg, reg, reg, reg, reg, reg, reg, reg, reg, reg, reg,

/* 0x2a - 0x36 */
/* lshift, |,   z,   x,   c,   v,   b,   n,   m,   <,   >,   ?, rshift */
   lshift, reg, reg, reg, reg, reg, reg, reg, reg, reg, reg, reg, rshift,

/* 0x37 - 0x46 */
/* none, leftalt, space, caps, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12 */
   none, leftalt, reg,   caps, fc, fc, fc, fc, fc, fc, fc, fc, fc, fc,  fc,  fc,

/* 0x48 - 0x51 */
/* home, up, page up, none, left, none, right, none, end, down, page down */
   home, up, page_up, none, left, none, right, none, end, down, page_down,

/* 0x52 - 0x5c */
   none, none, none, none, none, none, none, none, none, none,
/* 0x5d - 0x67 */
   none, none, none, none, none, none, none, none, none, none,
/* 0x68 - 0x72 */
   none, none, none, none, none, none, none, none, none, none,
/* 0x73 - 0x7d */
   none, none, none, none, none, none, none, none, none, none,
/* 0x7e - 0x7f */
   none, none
};

const char Keyboard::us_map[0x3a][2] = {
/* 0x0 - 0xe */
/* unk   esc   1    2   3    4    5    6    7    8    9    0     -   +    del */
  {0,0}, {0,0},
  {'1','!'}, {'2','@'}, {'3','#'}, {'4','$'}, {'5','%'},
  {'6','^'}, {'7','&'}, {'8','*'}, {'9','('}, {'0',')'},
  {'-','_'}, {'=','+'}, {'\b','\b'},

/* 0xf - 0x1c */
/* tab,  q,   w,   e,   r,   t,   y,   u,   i,   o,   p,   {,   },  enter */
   {'\t','\t'}, {'q','Q'}, {'w','W'}, {'e','E'}, {'r','R'}, {'t','T'}, {'y','Y'},
   {'u','U'}, {'i','I'}, {'o','O'}, {'p','P'}, {'[','{'}, {']','}'}, {'\n','\n'},

/* 0x1d - 0x29 */
/* ctrl, a,   s,   d,   f,   g,   h,   j,   k,   l,   :,   ',   ' */
   {0,0}, {'a','A'}, {'s','S'}, {'d','D'}, {'f','F'}, {'g','G'}, {'h','H'},
   {'j','J'}, {'k','K'}, {'l','L'}, {';',':'}, {'\'', '"'}, {'`','~'},

/* 0x2a - 0x36 */
/* lshift, |,   z,   x,   c,   v,   b,   n,   m,   <,   >,   ?, rshift */
   {0,0}, {'\\','|'}, {'z','Z'}, {'x','X'}, {'c','C'}, {'v','V'}, {'b','B'},
   {'n','N'}, {'m','M'}, {',','<'}, {'.','>'}, {'/', '?'}, {0,0},

/* 0x37 - 0x39 */
/* none, leftalt, space */
   {0,0}, {0,0}, {' ', ' '}
};

const char Keyboard::dv_map[0x3a][2] = {
/* 0x0 - 0xe */
/* unk   esc   1    2   3    4    5    6    7    8    9    0     -   +    del */
  {0,0}, {0,0},
  {'1','!'}, {'2','@'}, {'3','#'}, {'4','$'}, {'5','%'},
  {'6','^'}, {'7','&'}, {'8','*'}, {'9','('}, {'0',')'},
  {'-','_'}, {'=','+'}, {'\b','\b'},

/* 0xf - 0x1c */
/* tab,  q,   w,   e,   r,   t,   y,   u,   i,   o,   p,   {,   },  enter */
   {'\t','\t'}, {'\'','"'}, {',','<'}, {'.','>'}, {'p','P'}, {'y','Y'}, {'f','F'},
   {'g','G'}, {'c','C'}, {'r','R'}, {'l','L'}, {'/','?'}, {'=','+'}, {'\n','\n'},

/* 0x1d - 0x29 */
/* ctrl, a,   s,   d,   f,   g,   h,   j,   k,   l,   :,   ',   ' */
   {0,0}, {'a','A'}, {'o','O'}, {'e','E'}, {'u','U'}, {'i','I'}, {'d','D'},
   {'h','H'}, {'t','T'}, {'n','N'}, {'s','S'}, {'-', '_'}, {'`','~'},

/* 0x2a - 0x36 */
/* lshift, |,   z,   x,   c,   v,   b,   n,   m,   <,   >,   ?, rshift */
   {0,0}, {'\\','|'}, {';',':'}, {'q','Q'}, {'j','J'}, {'k','K'}, {'x','X'},
   {'b','B'}, {'m','M'}, {'w','W'}, {'v','V'}, {'z', 'Z'}, {0,0},

/* 0x37 - 0x39 */
/* none, leftalt, space */
   {0,0}, {0,0}, {' ', ' '}
};


void Keyboard::pressed() {
  u8 scan_code = io.inb();

  KeyboardKeyType type = mapping[scan_code & 0x7f];

  switch(type) {
  case none:
    break;
  case reg:
  case enter:
    if(scan_code <= 0x7f) {
      if(scan_code > 0x3a) {
        console.printf("bad scan_code: %d\n", scan_code);
      } else {
        console.put(map[scan_code][shifted]);
      }
    }
    break;
  case lshift:
  case rshift:
    shifted ^= 1;
    break;
  default:
    /* ignore */
    break;
  }
}

void Keyboard::init() {
  shifted = 0;
  map = dv_map;
  register_interrupt_handler(1, &keyboard_callback);
}
