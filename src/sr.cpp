#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <getopt.h>
#include <ctype.h>
#include <string.h>
/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
 **********************************************************************/

#define BIDIRECTIONAL 0    /* change to 1 if you're doing extra credit */
/* and write a routine called B_output */

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
	char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
	int seqnum;
	int acknum;
	int checksum;
	char payload[20];
};

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* Statistics
 * Do NOT change the name/declaration of these variables
 * You need to set the value of these variables appropriately within your code.
 * */
int A_application = 0;
int A_transport = 0;
int B_application = 0;
int B_transport = 0;

/* Globals
 * Do NOT change the name/declaration of these variables
 * They are set to zero here. You will need to set them (except WINSIZE) to some proper values.
 * */
float TIMEOUT = 20.0;
int WINSIZE;         //This is supplied as cmd-line parameter; You will need to read this value but do NOT modify it;
int SND_BUFSIZE = 0; //Sender's Buffer size
int RCV_BUFSIZE = 0; //Receiver's Buffer size
float time_local = 0;

//TANVIS variable
struct msg buffer[1000];
int base=0,nextseqnum=0;
void tolayer3(int AorB,struct pkt packet);
void tolayer5(int AorB,char *datasent);
void starttimer(int AorB,float increment);
void stoptimer(int AorB);
int timer_started=0;



struct pack_details
{
	struct pkt my_packet; //stores packet received
	float starttime_packet;//stores time at which packet arrived
	float time_stop;  //Stores time at which timer for packet is stopped
	int pk_timer_started;//tells whether timer for packet is started or not
	int ack;//tells whether ack is received or not for that packet

};
struct pack_details pd[1000];
int my_packet_no=0;
/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message) //ram's comment - students can change the return type of the function from struct to pointers if necessary
{


	//Buffer 1000 packets, used it to send when window moves ahead
	if(SND_BUFSIZE<1000)
	{
		A_application++;

		strncpy(buffer[SND_BUFSIZE].data, message.data,20);
		SND_BUFSIZE++;
	}
	//send if is n window
	if(nextseqnum<(base+WINSIZE)){


		struct pkt packet_a;
		strncpy(packet_a.payload,buffer[nextseqnum].data,20);

		packet_a.seqnum=nextseqnum;
		packet_a.acknum=-1;//it is not acknowledgment
		packet_a.checksum=packet_a.seqnum+packet_a.acknum;
		for(int i=0;i<20;i++)
			packet_a.checksum+=packet_a.payload[i];

		pd[my_packet_no].my_packet=packet_a;
		tolayer3(0,packet_a);
		A_transport++;

		pd[my_packet_no].ack=0;
		pd[my_packet_no].pk_timer_started=0;
		pd[my_packet_no].starttime_packet=time_local;//the time packet was sent, used to calculate timeout
		pd[my_packet_no].pk_timer_started==0;
		if(timer_started==0)
		{

			if(my_packet_no==0)//for first packet no need to find min
					{

				starttimer(0,TIMEOUT);

				timer_started=1;
				pd[my_packet_no].pk_timer_started=1;

					}
			else
			{
				//find packet with smallest timeout
				float min_start_time=999999;
				int min_index=0;
				int min_found=0;

				for(int j=0;j<=my_packet_no;j++)
				{

					if(pd[j].starttime_packet<min_start_time &&pd[j].pk_timer_started==0 && pd[
j].ack==0)
					{
						min_start_time=pd[j].starttime_packet;
						min_index=j;
						min_found=1;
					}
				}
				if(min_found==1)
				{
					pd[min_index].pk_timer_started=1;

					int timer_val=TIMEOUT-(time_local-pd[min_index].starttime_packet);
					starttimer(0,timer_val);

					timer_started=1;
				}
			}

		}

		nextseqnum++;
		my_packet_no++;
	}

}

void B_output(struct msg message)  /* need be completed only for extra credit */
// ram's comment - students can change the return type of this function from struct to pointers if needed
{

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	printf("\nIn a_input");
	int i,j;

	int my_checksum=packet.seqnum+packet.acknum;
	for( i=0;i<20;i++)

		my_checksum+=packet.payload[i];
	int check_window=0;
	check_window=(packet.seqnum >= base) && (packet.seqnum< (base+WINSIZE)) ;

	if ((my_checksum==packet.checksum)&&(check_window==1) )
	{

		for(i=base;i<my_packet_no;i++)
		{
			if(pd[i].my_packet.seqnum==packet.seqnum )
			{
				pd[i].ack=1;
				if(pd[i].pk_timer_started==1)
				{
					stoptimer(0);
					pd[i].time_stop=time_local;

					timer_started=0;
					pd[i].pk_timer_started=0;
				}
				break;
			}
		}

		if(packet.seqnum==base)//change base
				{

			int base_changed=0;

			//make base as first uncknowledged packed
			for(i=base+1;i<my_packet_no;i++)
			{
				if(pd[i].ack==0)
				{
					base=i;
					base_changed=1;
					break;
				}
			}

			if (base_changed==0)
				base=my_packet_no;//Means no unacknowledged packet now, but will come in future so increase base
			//send packets which fall in window

			while((nextseqnum<(base+WINSIZE))&& (nextseqnum< SND_BUFSIZE)){
				printf("\n sending previously buffered packet which fall in window");

				struct pkt packet_a;
				strncpy(packet_a.payload,buffer[nextseqnum].data,20);

				packet_a.seqnum=nextseqnum;
				packet_a.acknum=-1;//it is not acknowledgment
				packet_a.checksum=packet_a.seqnum+packet_a.acknum;
				for(int i=0;i<20;i++)
					packet_a.checksum+=packet_a.payload[i];

				pd[my_packet_no].my_packet=packet_a;
				tolayer3(0,packet_a);
				A_transport++;

				pd[my_packet_no].ack=0;
				pd[my_packet_no].pk_timer_started=0;
				pd[my_packet_no].starttime_packet=time_local;
				pd[my_packet_no].pk_timer_started==0;
				if(timer_started==0)
				{

					if(my_packet_no==0)//for first packet no need to find min
							{

						starttimer(0,TIMEOUT);

						timer_started=1;
						pd[my_packet_no].pk_timer_started=1;

							}
					else
					{
						//find packet with smallest timeout
						float min_start_time=999999;
						int min_index=0;
						int min_found=0;

						for(int j=0;j<=my_packet_no;j++)
						{

							if(pd[j].starttime_packet<min_start_time &&pd[j].pk_timer_started==0 && pd[j].ack==0)
							{
								min_start_time=pd[j].starttime_packet;
								min_index=j;
								min_found=1;
							}
						}
						if(min_found==1)
						{
							pd[min_index].pk_timer_started=1;

							int timer_val=TIMEOUT-(time_local-pd[min_index].starttime_packet);
							starttimer(0,timer_val);
							timer_started=1;
						}
					}

				}
				

				nextseqnum++;
				my_packet_no++;
			}


				}
		//Find packet which has cminimum startime, calculate timeout and then start timer for it
		//float min_start_time=pd[0].starttime_packet+1;
		if(timer_started==0)
		{
			float min_start_time=999999;
			int min_index=0;
			int min_found=0;
			for(j=0;j<my_packet_no;j++)
			{
				
				if(pd[j].starttime_packet<min_start_time &&pd[j].pk_timer_started==0 && pd[j].ack==0)
				{
					min_start_time=pd[j].starttime_packet;
					min_index=j;
					min_found=1;
				}
			}
			if(min_found==1)
			{
				pd[min_index].pk_timer_started=1;

				int timer_val=TIMEOUT-(time_local-pd[min_index].starttime_packet);
				starttimer(0,timer_val);
				//printf("\nstarting timer for seqnum %d",pd[min_index].my_packet.seqnum);
				timer_started=1;
			}

		}

		//printf("\n Checksum matched.. base is %d",base);

	}
}

/* called when A's timer goes off */
void A_timerinterrupt() //ram's comment - changed the return type to void.
{
	int i,j;
	timer_started=0;
	//printf("\nIn A_timerinterrupt");
	for(i=0;i<my_packet_no;i++)
	{
		if(pd[i].pk_timer_started==1)
		{

			pd[i].pk_timer_started=0;
			tolayer3(0,pd[i].my_packet);
			A_transport++;
			pd[i].starttime_packet=time_local;
			pd[i].time_stop=time_local;
			break;
		}

	}

	//Find next packet for timer
	float min_start_time=999999;
	int min_index=0;
	int min_found=0;
	for(j=0;j<my_packet_no;j++)
	{

		if(pd[j].starttime_packet<min_start_time &&pd[j].pk_timer_started==0 && pd[j].ack==0)
		{
			min_start_time=pd[j].starttime_packet;
			min_index=j;
			min_found=1;
		}
	}
	if( min_found==1)
	{
		pd[min_index].pk_timer_started=1;
		//Calculate timeout
		int timer_val=TIMEOUT-(time_local-pd[min_index].starttime_packet);
		starttimer(0,timer_val);

		timer_started=1;
	}

}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() //ram's comment - changed the return type to void.
{
}


/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
int rcv_base=0,rcv_end=0;
class receiver_buffer
{
public:
	struct pkt packet;
	int received;
	int delivered5;//Tells whether deliver to layer5
	receiver_buffer()
	{
		received=0;
		//delivered5=0;
	}
};
receiver_buffer rcv_buf [1000];
void B_input(struct pkt packet)
{
	struct pkt new_packet;
	int my_checksum=0;
	B_transport++;
	my_checksum=packet.seqnum+packet.acknum;
	for(int i=0;i<20;i++)
		my_checksum+=packet.payload[i];
	memset(new_packet.payload,'\0',20);
	//printf("\n calculated checksum is %d ",my_checksum);
	new_packet.acknum=1;
	//new_packet.payload[0]='\0';//is not data
	new_packet.seqnum=packet.seqnum;
	new_packet.checksum=new_packet.acknum+new_packet.seqnum;
	for(int i=0;i<20;i++)
		new_packet.checksum+=new_packet.payload[i];

	if((my_checksum==packet.checksum )&& (packet.seqnum>=rcv_base) && (packet.seqnum<=(rcv_base
+WINSIZE-1)))
	{

		//Buffer packet if not received before(do not buffer retransmitted packets)

		int check_packet=0,i,temp;
		for(i=0;i<rcv_end;i++)
		{
			if((rcv_buf[i].received==1) &&(rcv_buf[i].packet.seqnum==packet.seqnum))
				check_packet=1;

		}
		//buffer if packet not received previously
		if(check_packet==0)
		{
			rcv_buf[rcv_end].packet=packet;
			rcv_buf[rcv_end].received=1;
			rcv_end++;
			receiver_buffer temp;
			//sort
			for (i = rcv_base; i<rcv_end; i++)
			{
				for(int j=0; j<(rcv_end-1);j++)
				{
					if (rcv_buf[j].packet.seqnum > rcv_buf[j + 1].packet.seqnum) {
						temp = rcv_buf[j];
						rcv_buf[j] = rcv_buf[j + 1];
						rcv_buf[j + 1] = temp;
					}
				}
			}

		}
		/* printf("\nRcv Buffer");
               for(int m=0;m<rcv_end;m++)
                 printf("\n seq no is %d",rcv_buf[m].packet.seqnum);*/
		//send packrets to layer5
		if(packet.seqnum==rcv_base)
		{
			int pac_deliv=0;
			//consecutive packets starting with sequence no equal to base should be delivered
			for(int p=rcv_base;p<rcv_end;p++)
			{
				int found_seq=0;
				for(i=rcv_base;i<rcv_end;i++)
				{

					
					if(p==rcv_buf[i].packet.seqnum)
					{
						tolayer5(1,rcv_buf[i].packet.payload);
						B_application++;
						found_seq=1;
						pac_deliv++;
					}

				}
				if(found_seq==0)
					break;
			}
			rcv_base=rcv_base+pac_deliv;// window should move
			//tolayer5(1,packet.payload);
		}


		tolayer3(1,new_packet);
		



	}
	else if((my_checksum==packet.checksum )&& (packet.seqnum>=(rcv_base-WINSIZE)) && (packet.
seqnum<=rcv_base+WINSIZE-1))
	{


		tolayer3(1,new_packet);
		
	}

}

/* called when B's timer goes off */
void B_timerinterrupt() //ram's comment - changed the return type to void.
{
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() //ram's comment - changed the return type to void.
{
}

int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */
int nsimmax = 0;           /* number of msgs to generate, then stop */
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */
int   ntolayer3;           /* number sent into layer 3 */
int   nlost;               /* number lost in media */
int ncorrupt;              /* number corrupted by media*/

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand()
{
	double mmm = 2147483647;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
	float x;                   /* individual students may need to change mmm */
	x = rand()/mmm;            /* x should be uniform in [0,1] */
	return(x);
}


/*****************************************************************
 ***************** NETWORK EMULATION CODE IS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
 ******************************************************************/



/* possible events: */
#define  TIMER_INTERRUPT 0
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1
#define   A    0
#define   B    1


struct event {
	float evtime;           /* event time */
	int evtype;             /* event type code */
	int eventity;           /* entity where event occurs */
	struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
	struct event *prev;
	struct event *next;
};
struct event *evlist = NULL;   /* the event list */


void insertevent(struct event *p)
{
	struct event *q,*qold;

	if (TRACE>2) {
		printf("            INSERTEVENT: time is %lf\n",time_local);
		printf("            INSERTEVENT: future time will be %lf\n",p->evtime);
	}
	q = evlist;     /* q points to header of list in which p struct inserted */
	if (q==NULL) {   /* list is empty */
		evlist=p;
		p->next=NULL;
		p->prev=NULL;
	}
	else {
		for (qold = q; q !=NULL && p->evtime > q->evtime; q=q->next)
			qold=q;
		if (q==NULL) {   /* end of list */
			qold->next = p;
			p->prev = qold;
			p->next = NULL;
		}
		else if (q==evlist) { /* front of list */
			p->next=evlist;
			p->prev=NULL;
			p->next->prev=p;
			evlist = p;
		}
		else {     /* middle of list */
			p->next=q;
			p->prev=q->prev;
			q->prev->next=p;
			q->prev=p;
		}
	}
}





/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

void generate_next_arrival()
{
	double x,log(),ceil();
	struct event *evptr;
	//    //char *malloc();
	float ttime;
	int tempint;

	if (TRACE>2)
		printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

	x = lambda*jimsrand()*2;  /* x is uniform on [0,2*lambda] */
	/* having mean of lambda        */

	evptr = (struct event *)malloc(sizeof(struct event));
	evptr->evtime =  time_local + x;
	evptr->evtype =  FROM_LAYER5;
	if (BIDIRECTIONAL && (jimsrand()>0.5) )
		evptr->eventity = B;
	else
		evptr->eventity = A;
	insertevent(evptr);
}





void init(int seed)                         /* initialize the simulator */
{
	int i;
	float sum, avg;
	float jimsrand();


	printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
	printf("Enter the number of messages to simulate: ");
	scanf("%d",&nsimmax);
	printf("Enter  packet loss probability [enter 0.0 for no loss]:");
	scanf("%f",&lossprob);
	printf("Enter packet corruption probability [0.0 for no corruption]:");
	scanf("%f",&corruptprob);
	printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
	scanf("%f",&lambda);
	printf("Enter TRACE:");
	scanf("%d",&TRACE);

	srand(seed);              /* init random number generator */
	sum = 0.0;                /* test random number generator for students */
	for (i=0; i<1000; i++)
		sum=sum+jimsrand();    /* jimsrand() should be uniform in [0,1] */
	avg = sum/1000.0;
	if (avg < 0.25 || avg > 0.75) {
		printf("It is likely that random number generation on your machine\n" );
		printf("is different from what this emulator expects.  Please take\n");
		printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
		exit(0);
	}

	ntolayer3 = 0;
	nlost = 0;
	ncorrupt = 0;

	time_local=0;                    /* initialize time to 0.0 */
	generate_next_arrival();     /* initialize event list */
}






//int TRACE = 1;             /* for my debugging */
//int nsim = 0;              /* number of messages from 5 to 4 so far */
//int nsimmax = 0;           /* number of msgs to generate, then stop */
//float time = 0.000;
//float lossprob;            /* probability that a packet is dropped  */
//float corruptprob;         /* probability that one bit is packet is flipped */
//float lambda;              /* arrival rate of messages from layer 5 */
//int   ntolayer3;           /* number sent into layer 3 */
//int   nlost;               /* number lost in media */
//int ncorrupt;              /* number corrupted by media*/

/**
 * Checks if the array pointed to by input holds a valid number.
 *
 * @param  input char* to the array holding the value.
 * @return TRUE or FALSE
 */
int isNumber(char *input)
{
	while (*input){
		if (!isdigit(*input))
			return 0;
		else
			input += 1;
	}

	return 1;
}

int main(int argc, char **argv)
{
	struct event *eventptr;
	struct msg  msg2give;
	struct pkt  pkt2give;

	int i,j;
	char c;

	int opt;
	int seed;

	//Check for number of arguments
	if(argc != 5){
		fprintf(stderr, "Missing arguments\n");
		printf("Usage: %s -s SEED -w WINDOWSIZE\n", argv[0]);
		return -1;
	}

	/*
	 * Parse the arguments
	 * http://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html
	 */
	while((opt = getopt(argc, argv,"s:w:")) != -1){
		switch (opt){
		case 's':   if(!isNumber(optarg)){
			fprintf(stderr, "Invalid value for -s\n");
			return -1;
		}
		seed = atoi(optarg);
		break;

		case 'w':   if(!isNumber(optarg)){
			fprintf(stderr, "Invalid value for -w\n");
			return -1;
		}
		WINSIZE = atoi(optarg);
		break;

		case '?':   break;

		default:    printf("Usage: %s -s SEED -w WINDOWSIZE\n", argv[0]);
		return -1;
		}
	}

	init(seed);
	A_init();
	B_init();

	while (1) {
		eventptr = evlist;            /* get next event to simulate */
		if (eventptr==NULL)
			goto terminate;
		evlist = evlist->next;        /* remove this event from event list */
		if (evlist!=NULL)
			evlist->prev=NULL;
		if (TRACE>=2) {
			printf("\nEVENT time: %f,",eventptr->evtime);
			printf("  type: %d",eventptr->evtype);
			if (eventptr->evtype==0)
				printf(", timerinterrupt  ");
			else if (eventptr->evtype==1)
				printf(", fromlayer5 ");
			else
				printf(", fromlayer3 ");
			printf(" entity: %d\n",eventptr->eventity);
		}
		time_local = eventptr->evtime;        /* update time to next event time */
		if (nsim==nsimmax)
			break;                        /* all done with simulation */
		if (eventptr->evtype == FROM_LAYER5 ) {
			generate_next_arrival();   /* set up future arrival */
			/* fill in msg to give with string of same letter */
			j = nsim % 26;
			for (i=0; i<20; i++)
				msg2give.data[i] = 97 + j;
			if (TRACE>2) {
				printf("          MAINLOOP: data given to student: ");
				for (i=0; i<20; i++)
					printf("%c", msg2give.data[i]);
				printf("\n");
			}
			nsim++;
			if (eventptr->eventity == A)
				A_output(msg2give);
			else
				B_output(msg2give);
		}
		else if (eventptr->evtype ==  FROM_LAYER3) {
			pkt2give.seqnum = eventptr->pktptr->seqnum;
			pkt2give.acknum = eventptr->pktptr->acknum;
			pkt2give.checksum = eventptr->pktptr->checksum;
			for (i=0; i<20; i++)
				pkt2give.payload[i] = eventptr->pktptr->payload[i];
			if (eventptr->eventity ==A)      /* deliver packet by calling */
				A_input(pkt2give);            /* appropriate entity */
			else
				B_input(pkt2give);
			free(eventptr->pktptr);          /* free the memory for packet */
		}
		else if (eventptr->evtype ==  TIMER_INTERRUPT) {
			if (eventptr->eventity == A)
				A_timerinterrupt();
			else
				B_timerinterrupt();
		}
		else  {
			printf("INTERNAL PANIC: unknown event type \n");
		}
		free(eventptr);
	}

	terminate:
	//Do NOT change any of the following printfs
	printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n",time_local,
nsim);

	printf("\n");
	printf("Protocol: ABT\n");
	printf("[PA2]%d packets sent from the Application Layer of Sender A[/PA2]\n", A_application
);
	printf("[PA2]%d packets sent from the Transport Layer of Sender A[/PA2]\n", A_transport);
	printf("[PA2]%d packets received at the Transport layer of Receiver B[/PA2]\n", B_transport
);
	printf("[PA2]%d packets received at the Application layer of Receiver B[/PA2]\n", 
B_application);
	printf("[PA2]Total time: %f time units[/PA2]\n", time_local);
	printf("[PA2]Throughput: %f packets/time units[/PA2]\n", B_application/time_local);
	return 0;
}


/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

/*void generate_next_arrival()
{
   double x,log(),ceil();
   struct event *evptr;
    //char *malloc();
   float ttime;
   int tempint;

   if (TRACE>2)
       printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

   x = lambda*jimsrand()*2;  // x is uniform on [0,2*lambda]
                             // having mean of lambda
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + x;
   evptr->evtype =  FROM_LAYER5;
   if (BIDIRECTIONAL && (jimsrand()>0.5) )
      evptr->eventity = B;
    else
      evptr->eventity = A;
   insertevent(evptr);
} */




void printevlist()
{
	struct event *q;
	int i;
	printf("--------------\nEvent List Follows:\n");
	for(q = evlist; q!=NULL; q=q->next) {
		printf("Event time: %f, type: %d entity: %d\n",q->evtime,q->evtype,q->eventity);
	}
	printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(int AorB)
//AorB;  /* A or B is trying to stop timer */
{
	struct event *q,*qold;

	if (TRACE>2)
		printf("          STOP TIMER: stopping timer at %f\n",time_local);
	/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
	for (q=evlist; q!=NULL ; q = q->next)
		if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) {
			/* remove this event */
			if (q->next==NULL && q->prev==NULL)
				evlist=NULL;         /* remove first and only event on list */
			else if (q->next==NULL) /* end of list - there is one in front */
				q->prev->next = NULL;
			else if (q==evlist) { /* front of list - there must be event after */
				q->next->prev=NULL;
				evlist = q->next;
			}
			else {     /* middle of list */
				q->next->prev = q->prev;
				q->prev->next =  q->next;
			}
			free(q);
			return;
		}
	printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


void starttimer(int AorB,float increment)
// AorB;  /* A or B is trying to stop timer */

{

	struct event *q;
	struct event *evptr;
	////char *malloc();

	if (TRACE>2)
		printf("          START TIMER: starting timer at %f\n",time_local);
	/* be nice: check to see if timer is already started, if so, then  warn */
	/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
	for (q=evlist; q!=NULL ; q = q->next)
		if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) {
			printf("Warning: attempt to start a timer that is already started\n");
			return;
		}

	/* create future event for when timer goes off */
	evptr = (struct event *)malloc(sizeof(struct event));
	evptr->evtime =  time_local + increment;
	evptr->evtype =  TIMER_INTERRUPT;
	evptr->eventity = AorB;
	insertevent(evptr);
}


/************************** TOLAYER3 ***************/
void tolayer3(int AorB,struct pkt packet)
{
	struct pkt *mypktptr;
	struct event *evptr,*q;
	////char *malloc();
	float lastime, x, jimsrand();
	int i;


	ntolayer3++;

	/* simulate losses: */
	if (jimsrand() < lossprob)  {
		nlost++;
		if (TRACE>0)
			printf("          TOLAYER3: packet being lost\n");
		return;
	}

	/* make a copy of the packet student just gave me since he/she may decide */
	/* to do something with the packet after we return back to him/her */
	mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
	mypktptr->seqnum = packet.seqnum;
	mypktptr->acknum = packet.acknum;
	mypktptr->checksum = packet.checksum;
	for (i=0; i<20; i++)
		mypktptr->payload[i] = packet.payload[i];
	if (TRACE>2)  {
		printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
				mypktptr->acknum,  mypktptr->checksum);
		for (i=0; i<20; i++)
			printf("%c",mypktptr->payload[i]);
		printf("\n");
	}

	/* create future event for arrival of packet at the other side */
	evptr = (struct event *)malloc(sizeof(struct event));
	evptr->evtype =  FROM_LAYER3;   /* packet will pop out from layer3 */
	evptr->eventity = (AorB+1) % 2; /* event occurs at other entity */
	evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
	/* finally, compute the arrival time of packet at the other end.
   medium can not reorder, so make sure packet arrives between 1 and 10
   time units after the latest arrival time of packets
   currently in the medium on their way to the destination */
	lastime = time_local;
	/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
	for (q=evlist; q!=NULL ; q = q->next)
		if ( (q->evtype==FROM_LAYER3  && q->eventity==evptr->eventity) )
			lastime = q->evtime;
	evptr->evtime =  lastime + 1 + 9*jimsrand();



	/* simulate corruption: */
	if (jimsrand() < corruptprob)  {
		ncorrupt++;
		if ( (x = jimsrand()) < .75)
			mypktptr->payload[0]='Z';   /* corrupt payload */
		else if (x < .875)
			mypktptr->seqnum = 999999;
		else
			mypktptr->acknum = 999999;
		if (TRACE>0)
			printf("          TOLAYER3: packet being corrupted\n");
	}

	if (TRACE>2)
		printf("          TOLAYER3: scheduling arrival on other side\n");
	insertevent(evptr);
}

void tolayer5(int AorB,char *datasent)
{

	int i;
	if (TRACE>2) {
		printf("          TOLAYER5: data received: ");
		for (i=0; i<20; i++)
			printf("%c",datasent[i]);
		printf("\n");
	}

}
