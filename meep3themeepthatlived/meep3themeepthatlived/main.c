/* Name: main.c
 * Author: R. Scheidt
 * Copyright: <insert your copyright message here>
 * License: <insert your license reference here>
 */

#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include "includes.h"

#define NO_SYSTEM_ERROR						1
#define MEDIUM_PRIORITY_ERROR			2
#define HIGH_PRIORITY_ERROR				3

#define bpm_1			60
#define bpm_2			120
#define bpm_3			130
#define bpm_4			140
#define bpm_5			150
#define bpm_6			160
#define bpm_7			170
#define bpm_8			180
#define bpm_9			190


#define TASK_STK_SIZE							64		/* Size of each task's stacks (# of WORDs)            */
#define TRANSMIT_TASK_STK_SIZE		128		/* Size of the Transmit Task's stack                  */
#define TRANSMIT_BUFFER_SIZE			24	  /* Size of buffers used to store character strings    */

#define FREQUENCY 2000 //how frequently this code samples data in seconds
#define SAMPLERATE (1.0f/FREQUENCY) //how many times to sample per second
#define BLANKINGPERIOD 100 //time in ms
#define THRESHOLD 2.5
#define NSAMPLES 30

static int buffer[NSAMPLES]; //buffer for storing cardiac event timestamps
static int b_start_index =0; //buffer starting index
static int b_end_index=0; //buffer ending index

/*
 *********************************************************************************************************
 *                                               VARIABLES
 *********************************************************************************************************
 */
OS_STK        TaskStartStk[TASK_STK_SIZE];
OS_STK        TaskLedStk[TASK_STK_SIZE];
OS_STK        TaskBluetooth[TASK_STK_SIZE];
OS_STK        TaskTimerStk[TRANSMIT_TASK_STK_SIZE];
OS_STK        SerialTransmitTaskStk[TRANSMIT_TASK_STK_SIZE];

OS_EVENT     	*LedSem;
OS_EVENT     	*LedMBox;
OS_EVENT	 		*SerialTxSem;
OS_EVENT     	*SerialTxMBox;
OS_EVENT		*HeartMBox;

/*
 *********************************************************************************************************
 *                                           FUNCTION PROTOTYPES
 *********************************************************************************************************
 */

extern void InitPeripherals(void);

void  TaskStart(void *data);                  /* Function prototypes of Startup task */
void  LedTask(void *data);                    /* Function prototypes of tasks   */
void  TimerTask(void *data);                  /* Function prototypes of tasks */
void BlueToothTask(void *data);
void  USART_TX_Poll(unsigned char pdata);	   	/* Function prototypes of LedTask */
void  SerialTransmitTask(void *data);         /* Function prototypes of tasks */
void 	PostTxCompleteSem (void);               /* Function prototypes of tasks */


/*
 *********************************************************************************************************
 *                                                MAIN
 *********************************************************************************************************
 */
int main (void)
{
	InitPeripherals();

    OSInit();                                              /* Initialize uC/OS-II                      */

/* Create OS_EVENT resources here  */
LedMBox=OSMboxCreate((void *)0);
SerialTxMBox=OSMboxCreate((void *)0);
SerialTxSem=OSSemCreate(1);
HeartMBox=OSMboxCreate((void *)0);
/* END Create OS_EVENT resources   */

    OSTaskCreate(TaskStart, (void *)0, &TaskStartStk[TASK_STK_SIZE - 1], 0);

    OSStart();                                             /* Start multitasking                       */

	while (1)
	{
		;
	}
}


/*
 *********************************************************************************************************
 *                                              STARTUP TASK
 *********************************************************************************************************
 */
void TaskStart (void *pdata)
{
    pdata = pdata;                                         /* Prevent compiler warning                 */

	OSStatInit();                                          /* Initialize uC/OS-II's statistics         */

	OSTaskCreate(TimerTask, (void *)0, &TaskTimerStk[TRANSMIT_TASK_STK_SIZE - 1], 8);
	OSTaskCreate(LedTask, (void *) 0, &TaskLedStk[TASK_STK_SIZE - 1], 20);
	//PORTB |= _BV(PORTB5);
	OSTaskCreate(SerialTransmitTask, (void *) 0, &SerialTransmitTaskStk[TRANSMIT_TASK_STK_SIZE-1], 10);
	OSTaskCreate(BlueToothTask, (void *) 0, &TaskBluetooth[TASK_STK_SIZE - 1], 30);
	
    for (;;) {
        OSCtxSwCtr = 0;                         /* Clear context switch counter             */
        OSTimeDly(OS_TICKS_PER_SEC);			/* Wait one second                          */
    }
}


/*
 *********************************************************************************************************
 *                                                  TimerTASK
 *********************************************************************************************************
 */

void TimerTask (void *pdata)
{
	INT8U msg1;
	INT16U Message;
	char TextMessage[TRANSMIT_BUFFER_SIZE];
	char meep=MEDIUM_PRIORITY_ERROR;
	int *pmeep=meep;
	char smeep=HIGH_PRIORITY_ERROR;	
	char neep=NO_SYSTEM_ERROR;	
	char p;
    for (;;) {
		
 		OSMboxPost(LedMBox, (void *)neep); //No system error message (1)
		 
		strcpy(TextMessage, "No error state \r\n");
		OSMboxPost(SerialTxMBox,(void *)TextMessage);
		
 		OSTimeDly(OS_TICKS_PER_SEC*5);
		 
		OSMboxPost(LedMBox, (void *)smeep); //high priority error (3)
		strcpy(TextMessage, "High error state \r\n");
		OSMboxPost(SerialTxMBox,(void *)TextMessage);
 		OSTimeDly(OS_TICKS_PER_SEC*5);
 		OSMboxPost(LedMBox, (void *)meep); //medium priority error (2)
		 strcpy(TextMessage, "Medium error state \r\n");
		 OSMboxPost(SerialTxMBox,(void *)TextMessage);
		OSTimeDly(OS_TICKS_PER_SEC*5);
	
	}
}


/*
 *********************************************************************************************************
 *                                                 LED TASK
 *********************************************************************************************************
 */

void LedTask (void *pdata)
{
	INT8U msg1;
	void *msg;
	INT16U OnPeriodTimeout = OS_TICKS_PER_SEC/10;
	INT16U OffPeriodTimeout = OS_TICKS_PER_SEC-OnPeriodTimeout;
	//INT8U	LocalMessage = NO_SYSTEM_ERROR;
	char mwwp;
	float on;float off;

    for (;;) {
	 msg1=OSMboxAccept(LedMBox);
	 switch(msg1){
		 case NO_SYSTEM_ERROR:
			on=.1;
			off=1-.1;
			//USART_TX_Poll('p');
			strcpy(mwwp,(char *)msg);
 			PORTB |= (_BV(PORTB5));
 			OSTimeDly(OS_TICKS_PER_SEC*on);
 			PORTB &= !(_BV(PORTB5));
 			OSTimeDly(OS_TICKS_PER_SEC*(off));
		 break;
		 case HIGH_PRIORITY_ERROR:
			strcpy(mwwp,(char *)msg);
			on=.2;
			off=.2;
 			PORTB |= (_BV(PORTB5));
 			OSTimeDly(OS_TICKS_PER_SEC*on);
 			PORTB &= !(_BV(PORTB5));
 			OSTimeDly(OS_TICKS_PER_SEC*(off));
// 			 
// 			on=1.2;
// 			off=1.2;
// 			PORTB |= (_BV(PORTB5));
//  			OSTimeDly(OS_TICKS_PER_SEC*on);
//  			PORTB &= !(_BV(PORTB5));
//  			OSTimeDly(OS_TICKS_PER_SEC*(off));
		 break;
		 case MEDIUM_PRIORITY_ERROR:
		 	on=1.2;
			off=1.2;
			PORTB |= (_BV(PORTB5));
 			OSTimeDly(OS_TICKS_PER_SEC*on);
 			PORTB &= !(_BV(PORTB5));
 			OSTimeDly(OS_TICKS_PER_SEC*(off));
// 			 
// 			 strcpy(mwwp,(char *)msg);
// 			on=.2;
// 			off=.2;
//  			PORTB |= (_BV(PORTB5));
//  			OSTimeDly(OS_TICKS_PER_SEC*on);
//  			PORTB &= !(_BV(PORTB5));
//  			OSTimeDly(OS_TICKS_PER_SEC*(off));
		 break;
		 default:
			strcpy(mwwp,(char *)msg);
 			PORTB |= (_BV(PORTB5));
 			OSTimeDly(OS_TICKS_PER_SEC*on);
 			PORTB &= !(_BV(PORTB5));
 			OSTimeDly(OS_TICKS_PER_SEC*(off));
		 break;
		 
		 }

	 }

 }


/*
 *********************************************************************************************************
 *                                       Serial Transmission Task
 *********************************************************************************************************
 */
void SerialTransmitTask (void *pdata)
{
	//USART_TX_Poll('b');
	INT8U err;
//PORTB |= (_BV(PORTB5));
	void *msg;
	INT8U CharCounter=0;
	INT16U StringLength;
	char LocalMessage[TRANSMIT_BUFFER_SIZE];
	//LocalMessage=OSMboxAccept(SerialTxMBox);
	
	for(;;){
	msg=OSMboxPend(SerialTxMBox,0,&err);
	strcpy(LocalMessage,(char*)msg );
 	StringLength=sizeof(LocalMessage);
	 
		extern SerTxISR();
	SerTxISR();
 	//OSTimeDly(OS_TICKS_PER_SEC*5);
	for (CharCounter=0;CharCounter<StringLength;CharCounter++) {
		PORTB |= (_BV(PORTB4));
		OSSemPend(SerialTxSem,0,&err);
		PORTB &= !(_BV(PORTB4));
		//UDR0=LocalMessage[CharCounter];
		OSTimeDly(OS_TICKS_PER_SEC/200);
		//USART_TX_Poll(LocalMessage[CharCounter]);
		//USART_TX_Poll('a');
		//OSTimeDly(OS_TICKS_PER_SEC/10);
	}
	extern	EndISR();
	EndISR();
	}
	//return;
}

/*
 *********************************************************************************************************
 *                                       USART Transmit Helper Fcn
 *********************************************************************************************************
 */
void USART_TX_Poll (unsigned char data)
{
// 	for(;;){
// 		if (UDRE0==1){ //24.12.2
// 				UDR0=data;
// 				return;	
// 		}
// 	}
	PORTB |= (_BV(PORTB4));
	while (!(UCSR0A & (1<<UDRE0))){ //24.7.1
		//OSTimeDly(OS_TICKS_PER_SEC/2);
	}
	UDR0=data;
	PORTB &= !(_BV(PORTB4));
	return;
}


/*
 *********************************************************************************************************
 *                        Routine to Post the Transmit buffer empty semaphone
 *********************************************************************************************************
 */
void PostTxCompleteSem (void)
{
	OSSemPost(SerialTxSem);
	return;
}

void BlueToothTask(void* pdata)
{
	OS_ENTER_CRITICAL();
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // ADC control and status register, prescalar at 128 for a 125 kHz clock source
	ADMUX |= (1 << REFS0); // setup mux pin
	ADMUX &= ~(1 << REFS1); //+5 V reference voltage
	ADCSRB &= ~((1 << ADTS2) | (1 << ADTS1) | (1 << ADTS0)); //?
	ADCSRA |= (1<< ADATE);  //?
	ADCSRA |= (1<< ADEN); //Powerup ADC
	ADCSRA |= (1<< ADSC); //Start Conversion 
	static int inBlanking= 0; //is this currently in the refractory period?
	static int adcValue; //voltage value (as an int) at this moment;
	static float adcAcc = -1;//,deriv,oldAcc; //voltage value 0-5V, and the derivative voltage
	static int beats =0; //how many beats there were in the 6 second period
	static float timer = 0; //time in seconds
	static float lastEvent = 0; //last event time in seconds
	static int used = 0;
	char LocalMessage[TRANSMIT_BUFFER_SIZE];
	char b1 = bpm_1; //i only know how to mbox post these kind of messages
	char b2 = bpm_2;
	char b3 = bpm_3;
	char b4 = bpm_4;
	char b5 = bpm_5;
	char b6 = bpm_6;
	char b7 = bpm_7;
	char b8 = bpm_8;
	char b9 = bpm_9;			
	for(;;)
	{
		adcValue = ADCL | (ADCH << 8); //reads in Voltage value on Analog in, port 0 (PORTC0)
		//oldAcc = adcAcc;
		adcAcc = (float)(adcValue) *(5.0/1023.0); //converts voltage to real voltage range(5v max)
		//take derivative to find points of large change
		//deriv = (adcAcc-oldAcc)/SAMPLERATE;
		//if(deriv < 0.1) deriv = 0; //truncate small values
		if(inBlanking)
		{ //Is currently in refractory period
			if(timer - lastEvent > BLANKINGPERIOD/1000.0f ) //wait until the last event is at least (BLANKINGPERIOD) seconds old
				inBlanking = 0;
		}
		// check if event occurred
		else if(adcAcc > THRESHOLD )
		{//||((int)timer % 3) == 0 ) {//called once every 6 secs
			if(1 || used ==0)
			{
				//printf("EVENT at: %d\n",(int)timer);
				used = 1; // was this function called last sample
				inBlanking = 1; //event is called, start blanking
				lastEvent = timer;
				buffer[++b_end_index] = lastEvent; //sets current rolling buffer index to the most recent event timestamp
    
				if(b_end_index >= NSAMPLES-1)
					b_end_index = 0; //if end index is out of bounds, set it back to 0
				if(b_end_index == b_start_index)
					b_start_index++; //if end index reaches the start index (it shouldn't normally), it increases start index to prevent weird behavior
    
				while((buffer[b_start_index] +6.1 < (int)timer) && b_start_index != b_end_index)
				{ //while oldest event is at least 6 seconds old and start index hasn't reached most recent event
					b_start_index++; //increase start index, so that the buffer only contains events newer than 6 seconds old
				}
       
				if(b_start_index >= NSAMPLES) //if start index is out of bounds, set it back to 0
					b_start_index = 0;
    
				//why is the index value important? we need it to find the number of heart beats in the last 6 seconds
				if(b_end_index >= b_start_index)  
					beats = b_end_index - b_start_index; //if the end is larger than the start, take the difference
				else
					beats = (b_end_index+ NSAMPLES) - b_start_index;// if the start is larger, the end rolled over, and is effectively at the (NSAMPLE + end) index
				//we only measure for 6 seconds, but can extrapolate it to 60 seconds
				//printf("BPM: %d\n",(beats*10));
				//printf("start: %d end: %d\n",b_start_index,b_end_index);
				//printf("BPM: %d\n",(beats/(int)timer))
// 				if (beats == 0)
// 					UDR0='a';
// 				else if(beats == 1)
// 					UDR0='b';
// 				else if(beats == 2)
// 					UDR0 ='c';
// 				else if(beats == 3)
// 					UDR0='d';
// 				else if(beats == 4)
// 					UDR0='e';
// 				else if(beats == 5)
// 					UDR0='f';
// 				else if(beats == 6)
// 					UDR0='g';
// 				else
// 					UDR0='h';
//				Since beats is only a value of less than 10, it needs to be multiplied by a factor in order to determine the BPM
				//this is the only way i know how to post to an mbox so its a little wonky
				//but i define the constants up at the top and post the ones that match to the mbox
				
				if(beats == 1)
				{
					OSMboxPost(HeartMBox,(void *) b1);
				}
				else if(beats == 2)
				{
					OSMboxPost(HeartMBox,(void *) b2);
				}
				else if(beats == 3)
				{
					OSMboxPost(HeartMBox,(void *) b3);
				}
				else if(beats == 4)
				{
					OSMboxPost(HeartMBox,(void *) b4);
				}
				else if(beats == 5)
				{
					OSMboxPost(HeartMBox,(void *) b5);
				}
				else if(beats == 6)
				{
					OSMboxPost(HeartMBox,(void *) b6);
				}
				else if(beats == 7)
				{
					OSMboxPost(HeartMBox,(void *) b7);
				}
				else if(beats == 8)
				{
					OSMboxPost(HeartMBox,(void *) b8);
				}
				else if(beats == 9)
				{
					OSMboxPost(HeartMBox,(void *) b9);
				}				
			}//if(1 || used ==0)
			else
				used = 0;
		}//else if(adcAcc > THRESHOLD )
		//PORTB = 0x00; //turn off light when in delay
		_delay_ms(SAMPLERATE*1000); //delay for the time between samples (convert SMAPLERATE to ms)
		timer += (SAMPLERATE); //a bad, but simple way of telling the time. Our circuit is fast enough that this is relatively accurate
		OS_EXIT_CRITICAL();
		//PORTB =0xFF; //led goes on while doing stuff
		//UDR0='P';
		//OSTimeDly(OS_TICKS_PER_SEC/100);
		//OSTimeDly(OS_TICKS_PER_SEC*2);
		//OSTimeDly(OS_TICKS_PER_SEC/2000);
	}//for(;;)
}