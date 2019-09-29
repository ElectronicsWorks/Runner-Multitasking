/*

  Runner

  (C) 2019 Dipl. Phys. Helmut Weber

  Preemptive Multitasking

*/


#include <sys/time.h>
#include <stdio.h>
#include <stdint.h>



/*
 * Returns the current time in microseconds.
 */
long micros(){
	struct timeval currentTime;
	gettimeofday(&currentTime, NULL);
	return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
}

typedef void Task;


// defines for baudrate
#define BAUD115200 (16) 
#define BAUD76800 (25) 
#define BAUD57600 (34) 
#define BAUD38400 (51) 
#define BAUD9600 (207)

#define SERMAX 100
char SerString[SERMAX];
int SerHead, SerTail;
unsigned int runs;




// ===================== Start of operating system ======================


#define 			NUMRUNS			10
#define 			MAXPRIORITY		0x0f
#define       		MAXLEVEL        50


// Definition for states

// states
#define				STOPPED			0x80
#define				WAITING			0x40
#define				RUNNING			0x20
#define				WAITEVENT		0x10

#define				PRI_KERNEL		0x0
#define       		PRI_SYSTEM  	0x1
#define				PRI_DISP		0x2
#define				PRI_USER0		0x3
#define				PRI_USER1		0x4
#define				PRI_USER2		0x5
#define				PRI_USER3		0x6
#define       		PRI_USER4  		0x7



unsigned char			numruns;
void			        (*runfunction[NUMRUNS])(void);
unsigned long			runinterval[NUMRUNS];
unsigned long			lastrun[NUMRUNS];
unsigned char			priorities[NUMRUNS];


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
  runinterval[i] = interval-4;
  lastrun[i] = micros();
  priorities[i] = priority;
  if (i >= numruns) numruns = ++i;
  if (numruns >= NUMRUNS) {
    printf("numruns >= NUMRUNS\n");
    while(1);
  }
  return (numruns-1);
}

char	level, irlevel, thisTask;


unsigned long m;
unsigned long m2;
//unsigned int curtask;



/*
     Schedule Tasks
*/
void Runner(unsigned char maxPriority) {    			  // call this from loop: Runner(MAXPRIORITY);
unsigned char *p;
register unsigned char *pp;
unsigned long mics;
int c;
int prioHigh;
int nextJob;

  runs++;
  
  if (level >= MAXLEVEL) {
    printf("LEVEL");
    return;
  }

  prioHigh=0x0f;
  nextJob=-1;
  
  level++;  

  for (c = 0; c < numruns; c++) {

    p = &priorities[c];                                   // --- check READY and PRIORITY
    if ((*p & 0xf0) == 0) {                               // excluding STOPPED and WAITING, RUNNING jobs         
      if (*p <= maxPriority) {                            // run only tasks with priority >= maxPriority           
        mics=micros();
        if ( (mics - lastrun[c]) >= runinterval[c]) {     // interval passed?
          if (*p<prioHigh) {                              // get task with highest priority
            prioHigh=*p;
            pp=p;
            nextJob=c;
            if (prioHigh==0) goto DoKernel;               // Kernel-Task: do it NOW
          }
        }        
      }
    }
  } // for

DoKernel:

  if (nextJob>=0) {                                       // is a task ready?
    //p = &priorities[nextJob];
    lastrun[nextJob] = mics;                              // remember start time
    *pp |= RUNNING;                                       // set RUNNING
    thisTask = nextJob;                                   // set global  thisTask
    (*runfunction[nextJob])();                            // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< run the task
    *pp &= ~RUNNING;                                      // reset ŔUNNING
  } // if

  level--;
 
}





// call this from inside of long working functions
inline void Delay(unsigned long dely, unsigned char maxPriority) {
  volatile unsigned long m;
  uint8_t mycurtask; //  must be local !
  
  irlevel++;
  mycurtask = thisTask;
  priorities[(int)thisTask] |= WAITING;						  // do not start the running task again

  m = micros();
  do {
    Runner(maxPriority);								  // start tasks with priority = maxPriority 
  } while ((micros() - m) < dely);

  priorities[mycurtask] &= ~WAITING;					  // restore calling task
  thisTask=mycurtask;
  irlevel--;
}


#define ESC 27
#define CLEARSCREEN()        printf("\x1b[2J\033[1;1H");    
#define GOTOXY(x,y)          printf("%c[%d;%dH",ESC,y,x);
#define RED()                printf("\x1b[1;31m");
#define GREEN()              printf("\x1b[1;32m");
#define YELLOW()             printf("\x1b[1;33m");
#define BLUE()               printf("\x1b[1;34m");
#define MAGENTA()            printf("\x1b[1;35m");
#define CYAN()               printf("\x1b[1;36m");
#define WHITE()              printf("\x1b[1;37m");

#define CURSOROFF()          printf("\x1b[?25l");


#define RESET()              printf("\x1b[0m");

// ===================== End of operating system ======================

Task Blink() {
static uint8_t On;
  if(On) {
    GOTOXY(1,5);
    GREEN();
    printf("O\n");
    On=0;
  }
  else {
    GOTOXY(1,5);
    RED();
    printf("O\n");
    On=1;
  }

}



Task Count() {
static unsigned long Cnt;
    GOTOXY(10,5);
    WHITE();
    printf("%lu",Cnt++);
}


Task Runs() {
    GOTOXY(20,5);
    GREEN();
    printf("%d",runs);
    runs=0;
}




int main(void) {
    CLEARSCREEN(); 
    GREEN();
    printf("Intro to RUNNER multitasking      (C) 2015 H. Weber\n");       // Green text   
    RESET();
    CURSOROFF();

    InitRun(Blink,    100000, PRI_USER1);    // Toggle "LED" every 100 ms
    InitRun(Count,    1000, PRI_USER1);      // Count every 1 ms
    InitRun(Runs,     1000000, PRI_USER1);   // Show Taskswitch calls

    while(1) Runner(MAXPRIORITY);

   return(0);
 }
