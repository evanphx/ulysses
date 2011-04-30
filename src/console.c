// monitor.c -- Defines functions for writing to the monitor.
//             heavily based on Bran's kernel development tutorials,
//             but rewritten for JamesM's kernel tutorials.

#include "monitor.h"

Console console = { (u16*)0xB8000, {0x3D4}, {0x3D5}, 0, 0 };

void Console::move_cursor() {
  // The screen is 80 characters wide...
  u16 loc = y * 80 + x;

  // Tell the VGA board we are setting the high cursor byte.
  ctrl.outb(14);

  // Send the high cursor byte.
  data.outb(loc >> 8);

  // Tell the VGA board we are setting the low cursor byte.
  ctrl.outb(15);

  // Send the low cursor byte.
  data.outb(loc);
}

// Scrolls the text on the screen up by one line.
void Console::scroll() {
  // Get a space character with the default colour attributes.
  u8  attributeByte = (0 /*black*/ << 4) | (15 /*white*/ & 0x0F);
  u16 blank = 0x20 /* space */ | (attributeByte << 8);

  // Row 25 is the end, this means we need to scroll up
  if(y >= 25) {
    // Move the current text chunk that makes up the screen
    // back in the buffer by a line
    for(int i = 0*80; i < 24*80; i++) {
      video_memory[i] = video_memory[i+80];
    }

    // The last line should now be blank. Do this by writing
    // 80 spaces to it.
    for(int i = 24*80; i < 25*80; i++) {
      video_memory[i] = blank;
    }

    // The cursor should now be on the last line.
    y = 24;
  }
}

// Writes a single character out to the screen.
void Console::put(char c) {
  // The background colour is black (0), the foreground is white (15).
  u8int backColour = 0;
  u8int foreColour = 15;

  // The attribute byte is made up of two nibbles - the lower being the 
  // foreground colour, and the upper the background colour.
  u8int  attributeByte = (backColour << 4) | (foreColour & 0x0F);
  // The attribute byte is the top 8 bits of the word we have to send to the
  // VGA board.
  u16int attribute = attributeByte << 8;
  u16int *location;

  // Handle a backspace, by moving the cursor back one space
  if(c == 0x08 && x) {
    x--;
  }

  // Handle a tab by increasing the cursor's X, but only to a point
  // where it is divisible by 8.
  else if(c == 0x09) {
    x = (x+8) & ~(8-1);
  }

  // Handle carriage return
  else if(c == '\r') {
    x = 0;
  }

  // Handle newline by moving cursor back to left and increasing the row
  else if(c == '\n') {
    x = 0;
    y++;
  }
  // Handle any other printable character.
  else if(c >= ' ') {
    location = video_memory + (y*80 + x);
    *location = c | attribute;
    x++;
  }

  // Check if we need to insert a new line because we have reached the end
  // of the screen.
  if(x >= 80) {
    x = 0;
    y ++;
  }

  // Scroll the screen if needed.
  scroll();
  // Move the hardware cursor.
  move_cursor();
}

void Console::setup() {
  x = 0;
  y = 10;
  move_cursor();
}

// Clears the screen, by copying lots of spaces to the framebuffer.
void Console::clear() {
  // Make an attribute byte for the default colours
  u8 attributeByte = (0 /*black*/ << 4) | (15 /*white*/ & 0x0F);
  u16 blank = 0x20 /* space */ | (attributeByte << 8);

  for(int i = 0; i < 80*25; i++) {
    video_memory[i] = blank;
  }

  // Move the hardware cursor back to the start.
  x = 0;
  y = 0;
  move_cursor();
}

// Outputs a null-terminated ASCII string to the monitor.
void Console::write(const char *c) {
  int i = 0;
  while(c[i]) {
    put(c[i++]);
  }
}

void Console::puts(const char* c) {
  write(c);
}

void Console::write_hex(int n) {
  s32int tmp;

  write("0x");

  char noZeroes = 1;

  for(int i = 28; i > 0; i -= 4)
  {
    tmp = (n >> i) & 0xF;
    if(tmp == 0 && noZeroes != 0) continue;

    if(tmp >= 0xA) {
      noZeroes = 0;
      put(tmp-0xA+'a' );
    } else {
      noZeroes = 0;
      put( tmp+'0' );
    }
  }

  tmp = n & 0xF;
  if(tmp >= 0xA) {
    put(tmp-0xA+'a');
  } else {
    put(tmp+'0');
  }
}

void Console::write_hex_np(int n) {
  s32int tmp;

  char noZeroes = 1;

  for(int i = 28; i > 0; i -= 4) {
    tmp = (n >> i) & 0xF;
    if(tmp == 0 && noZeroes != 0) continue;

    if(tmp >= 0xA) {
      noZeroes = 0;
      put(tmp-0xA+'a' );
    } else {
      noZeroes = 0;
      put(tmp+'0');
    }
  }

  tmp = n & 0xF;
  if(tmp >= 0xA) {
    put(tmp-0xA+'a');
  } else {
    put(tmp+'0');
  }

}

void Console::write_hex_byte(u8 byte) {
  if(byte < 0x10) {
    put('0');
  }
  write_hex_np(byte);
}

void Console::write_dec(int n) {
  if(n == 0) {
    put('0');
    return;
  } else if(n < 0) {
    put('-');
    n = -n;
  }

  s32int acc = n;
  char c[32];
  int i = 0;
  while(acc > 0) {
    c[i] = '0' + acc%10;
    acc /= 10;
    i++;
  }
  c[i] = 0;

  char c2[32];
  c2[i--] = 0;
  int j = 0;
  while(i >= 0) {
    c2[i--] = c[j++];
  }
  write(c2);
}

void Console::write_dec_ll(u64 n) {
  if(n == 0) {
    put('0');
    return;
  } else if(n < 0) {
    put('-');
    n = -n;
  }

  s64 acc = n;
  char c[32];
  int i = 0;
  while(acc > 0) {
    c[i] = '0' + acc%10;
    acc /= 10;
    i++;
  }
  c[i] = 0;

  char c2[32];
  c2[i--] = 0;
  int j = 0;
  while(i >= 0) {
    c2[i--] = c[j++];
  }
  write(c2);
}

void Console::printf(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  vprintf(fmt, ap);

  va_end(ap);
}

void Console::vprintf(const char* fmt, va_list ap) {
  char cur;
  char c;
  char* s;
  int d;
  long l;
  long long ll;

  while(*fmt) {
    cur = *fmt++;
    if(cur == '%') {
      int zero_pad = 0;
      int space_pad = 0;
      int width = 0;
      int is_long = 0;

retry:
      switch(*fmt++) {
      case '%':
        put('%');
        break;
      case 's':
      case 'S':
        s = va_arg(ap, char*);
        write(s);
        break;
      case 'd':
      case 'i':
      case 'o':
      case 'u':
      case 'D':
      case 'O':
      case 'U':
        switch(is_long) {
        default:
        case 0:
          d = va_arg(ap, int);
          write_dec(d);
          break;
        case 1:
          l = va_arg(ap, long);
          write_dec(l);
          break;
        case 2:
          ll = va_arg(ap, long long);
          write_dec_ll(ll);
          break;
        }
        break;
      case 'q':
        write_dec_ll(*va_arg(ap, u64*));
        break;
      case 'x':
      case 'X':
      case 'p':
        d = va_arg(ap, int);
        write_hex_np(d);
        break;
      case 'C':
      case 'c':
        c = va_arg(ap, int);
        put(c);
        break;
      case 'e':
      case 'E':
      case 'f':
      case 'F':
      case 'g':
      case 'G':
      case 'a':
      case 'A':
        write("<some float>");
        break;
      case 'l':
        is_long++;
        goto retry;
      // Ignore these for now.
      case 'h':
      case 'j':
      case 't':
      case 'z':
        goto retry;
      case '0':
        zero_pad = 1;
        goto retry;
      case ' ':
        space_pad = 1;
        goto retry;
      case '1':
        width = 1;
        goto retry;
      case '2':
        width = 2;
        goto retry;
        break;
      case '3':
        width = 4;
        goto retry;
        break;
      case '4':
        width = 4;
        goto retry;
        break;
      case '5':
        width = 5;
        goto retry;
        break;
      case '6':
        width = 6;
        goto retry;
        break;
      case '7':
        width = 7;
        goto retry;
        break;
      case '8':
        width = 8;
        goto retry;
        break;
      case '9':
        width = 9;
        goto retry;
        break;
      default:
        break;
      }
    } else {
      put(cur);
    }
  }
}

extern "C" {
  void kputs(const char* c) {
    console.write(c);
  }

  void kprintf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    console.printf(fmt, ap);

    va_end(ap);
  }
}
