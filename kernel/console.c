//
// Console input and output, to the uart.
// Reads are line at a time.
// Implements special input characters:
//   newline -- end of line
//   control-h -- backspace
//   control-u -- kill line
//   control-d -- end of file
//   control-p -- print process list
//

#include <stdarg.h>

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

#define BACKSPACE 0x100
#define C(x)  ((x)-'@')  // Control-x

//
// send one character to the uart.
// called by printf(), and to echo input characters,
// but not from write().
//
void
consputc(int c)
{
  if(c == BACKSPACE){
    // if the user typed backspace, overwrite with a space.
    uartputc_sync('\b'); uartputc_sync(' '); uartputc_sync('\b');
  } else {
    uartputc_sync(c);
  }
}

struct {
  struct spinlock lock;
  
  // input
  char buf[INPUT_BUF_SIZE];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
} cons;

struct historyBufferArray  commandsHistoryBuffer;

int top_disabled_at = -10000;

#define HISTORY "history"
void saveCommand();

//
// user write()s to the console go here.
//
int
consolewrite(int user_src, uint64 src, int n)
{
  int i;

  for(i = 0; i < n; i++){
    char c;
    if(either_copyin(&c, user_src, src+i, 1) == -1)
      break;
    uartputc(c);
  }

  return i;
}

//
// user read()s from the console go here.
// copy (up to) a whole input line to dst.
// user_dist indicates whether dst is a user
// or kernel address.
//
int
consoleread(int user_dst, uint64 dst, int n)
{
  uint target;
  int c;
  char cbuf;

  target = n;
  acquire(&cons.lock);
  while(n > 0){
    // wait until interrupt handler has put some
    // input into cons.buffer.
    while(cons.r == cons.w){
      if(killed(myproc())){
        release(&cons.lock);
        return -1;
      }
      sleep(&cons.r, &cons.lock);
    }

    c = cons.buf[cons.r++ % INPUT_BUF_SIZE];

    if(c == C('D')){  // end-of-file
      if(n < target){
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        cons.r--;
      }
      break;
    }

    // copy the input byte to the user-space buffer.
    cbuf = c;
    if(either_copyout(user_dst, dst, &cbuf, 1) == -1)
      break;

    dst++;
    --n;

    if(c == '\n'){
      // a whole line has arrived, return to
      // the user-level read().
      break;
    }
  }
  release(&cons.lock);

  return target - n;
}

int areStringsEqual(char* a, char* b, uint size)
{
    for(int i = 0; i < size; i++)
        if(a[i] != b[i])
            return 0;
    return 1;
}


//
// the console input interrupt handler.
// uartintr() calls this for input character.
// do erase/kill processing, append to cons.buf,
// wake up consoleread() if a whole line has arrived.
//

void deleteCurrentCommand() {
    while (cons.e != cons.r - 2) {
        cons.e--;
        consputc(BACKSPACE);
    }
    cons.e = cons.w = cons.r;
}

void replaceCommand() {
  for (int i = 0; i < commandsHistoryBuffer.commandsLength[commandsHistoryBuffer.currentCommand] - 1; i++)
  {
      cons.buf[cons.r + i] = commandsHistoryBuffer.commands[commandsHistoryBuffer.currentCommand][i];
      consputc(cons.buf[cons.r + i]);
      cons.e++;
  }
}

void
consoleintr(int c)
{
  acquire(&cons.lock);

  char arrow_up[] = {'\x1b', '[', 'A'};
  char arrow_down[] = {'\x1b', '[', 'B'};

  if (c == 'A' &&
          ((cons.e - cons.r) + 1 >= 3) && areStringsEqual(arrow_up, &cons.buf[cons.e - 2], 2))
  {
      deleteCurrentCommand();

      int currentIndex = commandsHistoryBuffer.currentCommand;
      int isFull = commandsHistoryBuffer.numOfCommandsInMem == MAX_HISTORY;

      if (currentIndex > 0 || isFull)
      {
          if(isFull && currentIndex == 0)
              commandsHistoryBuffer.currentCommand = MAX_HISTORY - 1;
          else
              commandsHistoryBuffer.currentCommand--;
          replaceCommand();
      }
  }

  else if (c == 'B' &&
          ((cons.e - cons.r) + 1 >= 3) && areStringsEqual(arrow_down, &cons.buf[cons.e - 2], 2))
  {
      deleteCurrentCommand();

      int currentIndex = commandsHistoryBuffer.currentCommand;
      int lastIndex = commandsHistoryBuffer.lastCommandIndex;
      int isFull = commandsHistoryBuffer.numOfCommandsInMem == MAX_HISTORY;

      if (lastIndex >= currentIndex + 1 || isFull)
      {
          if(isFull && currentIndex == MAX_HISTORY - 1)
              commandsHistoryBuffer.currentCommand = 0;
          else
              commandsHistoryBuffer.currentCommand++;
          replaceCommand();
      }
  }

  else {
      switch (c) {
          case C('P'):  // Print process list.
              procdump();
              break;
          case C('U'):  // Kill line.
              while (cons.e != cons.w &&
                     cons.buf[(cons.e - 1) % INPUT_BUF_SIZE] != '\n') {
                  cons.e--;
                  consputc(BACKSPACE);
              }
              break;
          case C('H'): // Backspace
          case '\x7f': // Delete key
              if (cons.e != cons.w) {
                  cons.e--;
                  consputc(BACKSPACE);
              }
              break;
          case 3:
              top_disabled_at = uptime();
              break;
          default:
              if (c != 0 && cons.e - cons.r < INPUT_BUF_SIZE) {
                  c = (c == '\r') ? '\n' : c;

                  // echo back to the user.
                  consputc(c);

                  // store for consumption by consoleread().
                  cons.buf[cons.e++ % INPUT_BUF_SIZE] = c;

                  if (c == '\n' || c == C('D') || cons.e - cons.r == INPUT_BUF_SIZE) {
                      // wake up consoleread() if a whole line (or end-of-file)
                      // has arrived.
                      cons.w = cons.e;
                      saveCommand();
                      wakeup(&cons.r);
                  }
              }
              break;
      }
  }
  
  release(&cons.lock);
}

void
consoleinit(void)
{
  initlock(&cons.lock, "cons");

  uartinit();

  // connect read and write system calls
  // to consoleread and consolewrite.
  devsw[CONSOLE].read = consoleread;
  devsw[CONSOLE].write = consolewrite;
}

void saveCommand()
{
    if (areStringsEqual(cons.buf + cons.r, HISTORY, 7))
        return;

    uint currentNumberOfCommands = commandsHistoryBuffer.numOfCommandsInMem;
    uint currentIndex = currentNumberOfCommands == 0 ? 0 : (commandsHistoryBuffer.lastCommandIndex + 1) % MAX_HISTORY;

    for(int i = 0; i < cons.w - cons.r; i++) {
        commandsHistoryBuffer.commands[currentIndex][i] = cons.buf[cons.r + i];
        consputc(cons.buf[cons.r + i]);
    }

    commandsHistoryBuffer.commandsLength[currentIndex] = cons.w - cons.r;
    commandsHistoryBuffer.lastCommandIndex = currentIndex;
    commandsHistoryBuffer.numOfCommandsInMem =  currentNumberOfCommands + 1 > MAX_HISTORY ? MAX_HISTORY: currentNumberOfCommands + 1;
    commandsHistoryBuffer.currentCommand = currentIndex;
}

