#define _ESP32
//#define _ARDUINO


/*
  Runner

  (C) 2019 Dipl. Phys. Helmut Weber

  smallest (and fastest?) Cooperative Multitasking ever

  ESP32-Version, Arduino-Version


  Is it possible to use a cooperative OS as an RTOS ?
 
  What is an RTOS? It garantees a Job to be done just in a predefined time. Same is true for a reaction on an
  Sensor-Action at the outside  - interrupts are inventented for this. 

  If the mouse pointer on the screen reacts on mouse movements without a delay a human can recognize the OS (Windows, Linux)
  is an RTOS in this sense.

  Most RTOS have a ticktime (mostly 1 ms) after which the OS saves the actual task state and starts the task which is READY and
  has the highest priority.

  But changing tasks only every 1ms is not enough. A Task with highest priority, which is allways READY, would suppress any 
  other task.
  For that the YIELD is invented. This allows a task to initiate a taskswitch long before its 1 ms timeslice is over.
  In reality most tasks give up their timeslices with YIELD or DELAY!
  That means, most tasks are cooperative in an RTOS.

  A real RTOS has a MMU to protect Program- an Data-Space of a task! And that is the most important feature of an RTOS.
  If a CPU does not have this feature a cooperative system can be used as an RTOS as well.

  A lot of RTOS tasks have the structure:
    while(1) {
      ...              Some code
      DELAY
      ...
      YIELD
      ...
      DELAY
    }

  My "CoopOS" is build to rebuild such structures!

  But very often you have to do a task from the beginning to the end - running in determined intervals.

  This program RUNNER is built for such systems.
  It is extreme compact (about 100 lines). There are only 3 functions:

    >>>>>>>>>> INITRUN()
    This function is used to build the tasklist.

  
    >>>>>>>>>> RUNNER
      This function is called as often as possible to start tasks which are READY (interval has passed by).
      while(1) {
        RUNNER();
      }
      RUNNER calls the task, which is READY and is the first in the tasklist.
      RUNNER is called (average) every 2.3 µs.
      The longest time gap between 2 calls is measured 20 µs.

    >>>>>>>>>> DELAY()
    Is used in tasks at points, where a delay is possible. Delay calls RUNNER recursively. It is used to give 
    high priority tasks the chance to run. Running (and delayed) tasks are not started again.

    

  The advantages:
  
    - PURE ANSI C (well, without micros() and IO-operations)
    - TRANSPORTABLE SOURCES
    - VERY SIMPLE, BUT VERSATILE
    - NO STACKSWICTH
    - NO SETJMP/LONGJMP
    - VERY FAST TASKSWITCHES
    - TASKSWITCHING "DELAY" IS NOT ONLY USABLE IN TASKS BUT ALSO IN ALL CALLED FUNCTIONS
    - TASKS ARE NORMAL FUNCTIONS WITHOUT CALLING SUCH FUNCTIONS/DEFINES LIKE "BEGIN_TASK/END_TASK"
    - NO TIMERS/INTERRUPTS ARE USED
    - NO LIBRARIES USED
    - ABOUT 100 LINES OF SOURCE CODE
    
  A Task should run complete from start to end - but it may contain as much DELAYS as you want !

  A lot of people think, an ESP32 is mostly used as an interacting device using BLE or WLAN. That may be
  true for most users, but I use it often without these features - just like a superfast Arduino.
  But with the 2 cores of the ESP32 it is possible to use BLE/WLAN together with RUNNER with no introduced delays.

  This program is thought as an example.
  

  Here are the 8 tasks:

    - ID_Fast=      InitRun(Fast,     50-13,   PRI_KERNEL);       // Send 2. DAC value every 38µs (max.: 53µs)
    Marks start of task (digitalWrite) and sends a value to DAC channel 1 to build s sawtooth
    This task is called ever 38µs. The deadline of this task is 55µs which is never reached or exceeded.
      
    
    - ID_Fast2=     InitRun(Fast2,    50-12,   PRI_KERNEL);       // Send DAC value every 38µs (max.: 53µs)
    Like FAST, but using DAC channel2 and build a sine function. It is independent of FAST and the interval time could be
    changed indepentently.
    
        
    - ID_Blink=     InitRun(Blink,    100000,  PRI_USER1);        // Toggle "LED" every 100 ms
    A blinking LED is the "Hello world" of each multitasker ;)
    
    
    - ID_Count=     InitRun(Count,    1000000, PRI_USER1);        // Count and print every second
    Counts and print seconds (and the longest time gap between 2 RUNNER-calls - never reaching or exeeding 20 µs)

    
    - ID_Runs=      InitRun(Runs,     1000000, PRI_USER1);        // Show Taskswitch calls every second
    Shows system-information every second:
     1)      2)  3) 4) 5)     6)
    438099 25980 53 52 103200 1
        1) number of RUNNER calls per second.    The average is one call every 2.3 µs !!!!!
        2) number of FAST is called per second.  The average is one call every 38 µs !
        3) longest time from call to call of FAST.  53 µs
        4) longest time from call to call of FAST2.  53 µs
        5) number of total Lotto-draws ( 20 per second )
        6) level of recursion of RUNNER
    
    - ID_SerOut=    InitRun(SerOut,   1000,    PRI_USER2);        // Send buffered characters to serial line
    Serial output of strings is a break for all systems. Here the serial output is buffered an the characters
    are sent to the line with an interval of 1 ms. So serial output induces much less system-delay.

    
    - ID_WaitEvent= InitRun(WaitEvent,1000,    PRI_EVENT);        // Wait for an event (1000µs deadtime after an event)
    For an RTOS it is important to react fast on external or internal events.
    A task can set to state WAITEVENT. Another task (or an interrupt) can activate a waiting task.
    Here WAITEVENT is activated from BLINK every 10 seconds.
    The delay from activating(BLINK) to run WAITEVENT is 2-3 µs!
    This is may considered as VERY FAST. 

    
    - ID_Lotto=     InitRun(Lottery,  50000,   PRI_USER2);        // Check Lotto translucent  20 times a second and show results (hits>=4)
    This task should show how to build state machine tasks and longer delays.
    LOTTO draws 6 numbers out of 49. This is the winning combination.
    Then LOTTO draws 20 times 6/49 combinations per second and compares them with the winning combination.
    If 4 or more numbers are correct the combination is print out.

    Here we have 8 tasks are running very deterministic!
    Even 2 tasks running every 38 µs are determinstic in the way, they are never delayed more than 15 µs.
    Once again: No interrupts are needed.

    Using serial output and achieving these results show, how a very simple cooperative system running from AtTiny over
    Arduino, ESP32 to supercomputers can be used as an RTOS. 

*/
#ifdef _ESP32
#include <driver/dac.h>
#include <math.h>
#endif




typedef void Task;



#define SERMAX 200                                  // 200 characters buffer
char SerString[SERMAX];                             // Buffer for serial output
int SerHead, SerTail;                               // Pointer in Buffer


unsigned long runs;                                  // counting Scheduler calls (Runner)
unsigned int  ID_Fast, ID_Fast2, ID_Blink, ID_Count, ID_Runs, ID_SerOut, ID_WaitEvent, ID_Lotto;
int           FastCnt,FastCnt2;;
unsigned long lcounts;
unsigned long EventStart;
unsigned long LottoStart;
unsigned long LastMics, MaxMics;




// ===================== Start of operating system ======================


#define       NUMRUNS       10                      // Max. number of tasks
#define       MAXPRIORITY   0x0f                    // Lowest priority to handle by Runner
#define       MAXLEVEL      50                      // Max. recursion level in Runner


// Definition for states

// states                                           // high nibble = state
#define       STOPPED       0x80                      
#define       WAITING       0x40
#define       RUNNING       0x20
#define       WAITEVENT     0x10
#define       READY         0x00

// low nibble = priority
#define       PRI_KERNEL    0x0                   
#define       PRI_SYSTEM    0x1
#define       PRI_EVENT     0x2
#define       PRI_USER0     0x3
#define       PRI_USER1     0x4
#define       PRI_USER2     0x5
#define       PRI_USER3     0x6
#define       PRI_USER4     0x7




unsigned char     numruns;                          // countings tasks
void              (*runfunction[NUMRUNS])(void);    // functions of tasks
unsigned long     runinterval[NUMRUNS];             // interval for each task
unsigned long     lastrun[NUMRUNS];                 // last time called
unsigned char     priorities[NUMRUNS];              // priorities of tasks


/*
    Init a Task (function, intervalin microseconds, Priority)
*/
int InitRun(void (*userfunction)(void), unsigned long interval, unsigned char priority) {
  int i, j;

  i = numruns;

  // replace STOPPED tasks
  for (j = 0; j < numruns; j++) {
    if (priorities[j] & STOPPED)  {
      i = j;
      break;
    }
  }

  runfunction[i] = userfunction;
  //runinterval[i] = interval-4; // ARDUINO
  runinterval[i] = interval;     // ESP32
  
  lastrun[i] = micros();
  priorities[i] = priority;
  if (i >= numruns) numruns = ++i;
  if (numruns >= NUMRUNS) {
    SerPrint("numruns >= NUMRUNS\n");
    while(1);
  }
  return (numruns-1);
}

char  level, irlevel, thisTask;





/*
     Schedule Tasks
*/
void Runner(unsigned char maxPrio) {            // call this from loop: Runner(MAXPRIORITY);
unsigned char *p;
register unsigned char *pp;
unsigned long mics;
int c;
int prioHigh;
int nextJob;

  runs++;

  
  if (level >= MAXLEVEL) {
    SerPrint("LEVEL");
    return;
  }

 
  mics=micros();
  // to test longest time between 2 RUNNER calls   Arduino: 156 µs, ESP32: 20µs
  //  if ((mics-LastMics) > MaxMics) {
  //    MaxMics=mics-LastMics;
  //  }
  //  LastMics=mics;

  
  prioHigh=0x0f;
  nextJob=-1;
  
  level++;  

  // priority is tasknumber
  //mics=micros();
  for (c = 0; c < numruns; c++) {
    if ( (mics - lastrun[c]) >= runinterval[c]) {           // interval passed?
      p = &priorities[c];
      if (*p <=maxPrio) {        // test priority
        lastrun[c] = mics;                                  // remember start time
        *p |= RUNNING;                                      // set RUNNING
        thisTask = c;                                       // set global  thisTask
        (*runfunction[c])();                                // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< run the task
        *p &= ~RUNNING;                                     // reset ŔUNNING
        break;
      }
    }
  }

  level--;
 
}





// call this from inside of long working functions
inline void Delay(unsigned long dely, unsigned char maxPrio) {
  volatile unsigned long m;
  uint8_t mycurtask; //  must be local !
  
  irlevel++;
  mycurtask = thisTask;
  priorities[(int)thisTask] |= WAITING;             // do not start the running task again

  m = micros();
  do {
    Runner(maxPrio);                            // start tasks with priority = PRI_EVENT 
  } while ((micros() - m) < dely);

  priorities[mycurtask] &= ~WAITING;                // restore calling task
  thisTask=mycurtask;
  irlevel--;
}


// ===================== End of operating system ======================?=


// ===================== Utility ========================================
void SerPrint(char *pt) {                           // print a string to buffer
  while (*pt != 0) {
    SerString[SerHead++]=*pt++;
    Delay(100,PRI_EVENT);
    if (SerHead>=SERMAX) SerHead=0;
  }
}







// ===================== Tasks ==========================================



Task SerOut() {                                     // Max 1000 characters/s with interval 1000
  if (SerHead != SerTail) {
#ifdef _ARDUINO
    UDR0=SerString[SerTail++];                    // Arduino: no test of free buffer neccessary
#else    
    Serial.write(SerString[SerTail++]);             // ESP32
#endif
    if(SerTail==SERMAX) SerTail=0;
  }
}


Task Blink() {
static uint8_t On;
static int cnt;
  
  if(On) {
#ifdef _ARDUINO
    PORTB |= B00100000;
#else
    digitalWrite(13,1);
#endif
    On=0;
  }
  
  else {
#ifdef _ARDUINO
    PORTB &= ~B00100000;
#else
    digitalWrite(13,0);
#endif
  
    On=1;
  }

  // Example of sending an event to a task
  // could be done by interrupt either
  
  cnt++;
  if ((cnt % 100)==0) {
    priorities[ID_WaitEvent] &= ~WAITEVENT;         // removes WAITEVENT, RUNNING is left
    EventStart=micros();
  }
}




Task Count() {
char bf[20];
static unsigned int Cnt;
    SerPrint(itoa(Cnt++,bf, 10));
//    Delay(100,PRI_EVENT);                         // show longest interval between RUNNER-calls
//    SerPrint(" ");
//    Delay(100,PRI_EVENT);
//    SerPrint(ltoa(MaxMics,bf, 10));
//    MaxMics=0;
//    Delay(100,PRI_EVENT);
    SerPrint("\n");

}



long longest; 


Task Fast() {                                       // should: every 38 µs, max.: 53µs
static long m, last; 
  m=micros();
  if(last==0) last=m;
  if((m-last) > longest) longest = m-last; 
  last=m;

#ifdef _ARDUINO  
  // to mark the running of this task 
  //digitalWrite(12,1);
  //digitalWrite(12,0);
  PORTB |= B00010000;
  PORTB &= ~B00010000;
  
#endif

#ifdef _ESP32
  // DAC: output sawtooth
  dac_output_voltage((dac_channel_t)1, (uint8_t)(FastCnt&0xff));

  // to show start and end of counting 0-127, 127-255
  digitalWrite(12,(FastCnt&0x80));

  // to measure resolution: 38 µs
  //dac_output_voltage((dac_channel_t)1, (uint8_t)((FastCnt&0xff)<<7));
#endif

  FastCnt++;
}


long longest2; 
unsigned char sine[256];


Task Fast2() {                                       // should: every 200 µs,  is: ever 211-216 µs
static long m, last; 
  m=micros();
  if(last==0) last=m;
  if((m-last) > longest2) longest2 = m-last; 
  last=m;

#ifdef _ARDUINO
  digitalWrite(14,1);
  digitalWrite(14,0);
#endif


#ifdef _ESP32
// DAC: output sine
  dac_output_voltage((dac_channel_t)2, (uint8_t)sine[(unsigned int)(FastCnt2&0xff)]);
#endif

  FastCnt2++;
}


Task Runs() {
char bf[20];
int f;
    //Serial.print(runs);
    SerPrint(ltoa(runs,bf,10));           // 1
    runs=0;
    Delay(100,PRI_EVENT);
    //Serial.print(" ");
    SerPrint(" ");
    Delay(100,PRI_EVENT);
    f=FastCnt; FastCnt=0;                 // 2
    //Serial.println(FastCnt);
    SerPrint(itoa(f,bf,10));              // 3
    Delay(100,PRI_EVENT);
    SerPrint(" ");
    Delay(100,PRI_EVENT);
    SerPrint(itoa(longest,bf,10));        // 4
    SerPrint(" ");
    Delay(100,PRI_EVENT);
    SerPrint(itoa(longest2,bf,10));        // 4
    Delay(100,PRI_EVENT);
    SerPrint(" ");
    SerPrint(ltoa(lcounts,bf,10));        // 5
    Delay(100,PRI_EVENT);
    SerPrint(" ");
    SerPrint(itoa(level,bf,10));          // 5
    Delay(100,PRI_EVENT);
    SerPrint("\n");
    //longest=0;
}


Task WaitEvent() {
char bf[20];
unsigned long l;
  l=micros();
  SerPrint("\nWaitEvent ");
  Delay(100,PRI_EVENT);
  SerPrint(ltoa(l-EventStart,bf,10));     // ESP32: 2-3 µs, Arduino: 36-68 µs
  Delay(100,PRI_EVENT);
  SerPrint(" us\n");
  Delay(100,PRI_EVENT);
  priorities[ID_WaitEvent] |= WAITEVENT;  // gives RUNNING + WAITEVENT, reset by Blink,
}




// ===================== Lottery help functions =======================

void sort(int a[], int size) {
    for(int i=0; i<(size-1); i++) {
        Delay(100,PRI_EVENT);
        for(int o=0; o<(size-(i+1)); o++) {
                Delay(100,PRI_EVENT);
                if(a[o] > a[o+1]) {
                    int t = a[o];
                    a[o] = a[o+1];
                    a[o+1] = t;
                }
        }
    }
}



void Draw( int lotto[]) {
static int index;
  index=0;
next:  
  Delay(100,PRI_EVENT);
  while (index<6) {  
    long r=random(1,49);
    for (int i=0; i<6; i++) {
      Delay(100,PRI_EVENT);
      if(lotto[i]==(int)r) {
        goto next;
      }
    }
    lotto[index++]=(int)r;
  }
}


int lottoCompare(int lotto[], int lottoTest[]) {
int hits=0;
  for (int i=0; i<6; i++) {
    for (int j=0; j<6; j++) {
      Delay(100,PRI_EVENT);
      if (lotto[i]==lottoTest[j]) hits++;
    }
  }
  return hits;
}

// ===================== End of Lottery help functions ================



Task Lottery() {
static int state=0;
static int lotto[6];
static int lottoTest[6];
char bf[20];
int hits;
static int WinTimes;

  LottoStart=micros();
  digitalWrite(11,1);
  digitalWrite(11,0);
  
  lcounts++;
 
  if (state==0) {                                   // draw 6 winning numbers out of 49
    randomSeed(analogRead(0));
    Delay(100,PRI_EVENT);
    Draw(lotto);
    Delay(1000,PRI_EVENT);
    sort(lotto,6);
    if (state==0) { state=1; return; }
  }

  Draw(lottoTest);                                  // new Lottery translucent
  Delay(100,PRI_EVENT);
  sort(lottoTest,6);
  Delay(100,PRI_EVENT);
  hits=lottoCompare(lotto, lottoTest);
  Delay(100,PRI_EVENT);
  
  if(hits>=4) {                                     // 4 hits: perfect value: Counts/Wins == 1030
    WinTimes++;
    SerPrint("\n                               Wins: "); 
    Delay(100,PRI_EVENT);
    SerPrint(itoa(WinTimes,bf,10)); 
    Delay(100,PRI_EVENT);
    SerPrint("   Hits: "); 
    Delay(100,PRI_EVENT);
    SerPrint(itoa(hits,bf,10)); 
    Delay(100,PRI_EVENT);
    SerPrint("   Counts: "); 
    Delay(100,PRI_EVENT);
    SerPrint(ltoa(lcounts,bf,10)); 
    Delay(100,PRI_EVENT);
    SerPrint("\n");
    for (int i=0; i<6; i++) {
      Delay(100,PRI_EVENT);
      SerPrint(itoa(lotto[i],bf,10)); SerPrint(" ");
    }
    SerPrint("\n");
    Delay(100,PRI_EVENT);

    for (int i=0; i<6; i++) {
      Delay(100,PRI_EVENT);
      SerPrint(itoa(lottoTest[i],bf,10)); SerPrint(" ");
    }
    SerPrint("   µs: ");
    SerPrint(ltoa(micros()-LottoStart,bf,10));      // about 23000 µs
    SerPrint("\n");
  }
  
  
}




int mymain(void) {
    Serial.println("Intro to RUNNER multitasking      (C) 2015 H. Weber\n");       // Green text   
    Lottery();                                                  // Draw the winning numbers
   
#ifdef _ESP32
    ID_Fast=      InitRun(Fast,     50-13,   PRI_KERNEL);       // Send 2. DAC value every 38µs (max.: 53µs)
    ID_Fast2=     InitRun(Fast2,    50-14,   PRI_KERNEL);       // Send DAC value every 38µs (max.: 53µs)
#endif
#ifdef _ARDUINO
    ID_Fast=      InitRun(Fast,     500-150,  PRI_KERNEL);       // Send 2. DAC value every 38µs (max.: 53µs) (ESp32)
    ID_Fast2=     InitRun(Fast2,    500-170,  PRI_KERNEL);       // Send DAC value every 38µs (max.: 53µs) (ESP32)
#endif

        
    ID_Blink=     InitRun(Blink,    100000,  PRI_USER1);        // Toggle "LED" every 100 ms
    ID_Count=     InitRun(Count,    1000000, PRI_USER1);        // Count and print every second
    ID_Runs=      InitRun(Runs,     1000000, PRI_USER1);        // Show Taskswitch calls every second
    ID_SerOut=    InitRun(SerOut,   1000,    PRI_USER2);        // Send buffered characters to serial line
    ID_WaitEvent= InitRun(WaitEvent,1000,    PRI_EVENT);        // Wait for an event (1000µs deadtime after an event)
    ID_Lotto=     InitRun(Lottery,  50000,   PRI_USER2);        // Check Lotto translucent  20 times a second and show results (hits>=4)
    
    while(1) Runner(MAXPRIORITY);                 // endless loop

   return(0);
 }




void setup() {
  Serial.begin(921600);
  pinMode(13,OUTPUT);
  pinMode(12,OUTPUT);
  pinMode(11,OUTPUT);
  pinMode(14,OUTPUT);

#ifdef _ESP32
  dac_output_enable((dac_channel_t) 1);
#endif

  for (int i=0;i<256;i++) {
    sine[i]= (int)(128.0 + ( 127.0*sin((float)i/255.0*6.28) ));
    //Serial.println(sine[i]);
  }
  
#ifdef _ESP32
  dac_output_enable((dac_channel_t) 2);
#endif
  
  mymain();                                       // init and run tasks
}




// never used:

void loop() {
  // put your main code here, to run repeatedly:
}
