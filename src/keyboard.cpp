#include "keyboard.hpp"
#include "isr.hpp"
#include "monitor.hpp"

#include "keyboard.hpp"

#include "scheduler.hpp"

Keyboard keyboard;

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
  u8 scan_code = io_.inb();

  KeyboardKeyType type = mapping[scan_code & 0x7f];

  switch(type) {
  case none:
    break;
  case reg:
  case enter:
    if(scan_code <= 0x3a) {
      put(map_[scan_code][shifted_]);
    }
    break;
  case lshift:
  case rshift:
    shifted_ ^= 1;
    break;
  default:
    /* ignore */
    break;
  }
}

void Keyboard::put(char keycode) {
  if(!buffer_.put(keycode)) {
    console.alert("Keyboard buffer overflowth!");
  }
}

void Keyboard::in_thread() {
  for(;;) {
    console.printf("top of keyboard thread: %d\n", cpu::interrupts_enabled_p());
    pressed();
    ASSERT(scheduler.switch_thread());
  }
}

void Keyboard::schedule_thread() {
  console.printf("schedule keyboard thread\n");
  scheduler.schedule_hiprio(thread_);
}

static void kbd_thread() {
  keyboard.in_thread();
}

class KeyboardCallback : public interrupt::Handler {
public:
  void handle(Registers* regs) {
    keyboard.pressed();
  }
};

void Keyboard::init() {
  io_.port = 0x60;
  status_.port = 0x64;

  shifted_ = 0;
  map_ = dv_map;

  buffer_.allocate(cDefaultBufferSize);

  thread_ = scheduler.spawn_thread(kbd_thread);

  static KeyboardCallback callback;
  interrupt::register_interrupt(1, &callback);
}
