#include <msp430.h> 


#define lockPos 640                     //lock position macro
#define unlockPos 1450                  //unlock position macro
#define clkfrq 1000000                  //clock frequency macro
#define PWMperiod .02                   //Pulse Width Modulation macro


int inputCounter = 0;                   //counter for the inputData array
int dataR = 0;
char inputData[50];

void initGPIO(void){                    //Initializing the GPIO Pin 1.4 for input

        P1DIR &= ~BIT4;                 // Set P1.4 to input direction
        P1REN |= BIT4;                  // Enable P1.4 internal resistance
        P1SEL &= ~BIT4;                 // Set P1.4 to input
        P1IE |= BIT4;                   // P1.1 interrupt enabled
        P1IES &= ~BIT4;                 // P1.1 rising edge
        P1IFG &= ~BIT4;                 // P1.1 IFG cleared
}
void turnServo(int isVerified){         //Code to turn servo from unlocked to locked depending on if the fingerprint is verified or not
    if(isVerified == 1){                //If we are sent a 1 for unlock
        TA0CCR2 = unlockPos;            //Change the PWM to the unlock position
    }else if(isVerified == 0){          //If we are sent a 0 for lock
        TA0CCR2 = lockPos;              //Change the PWM for the lock position
    }
    dataR = 0;                          //Reset the data
}
void cfgServo(void){
   TA0CTL = TASSEL_2 + MC_1 + TACLR;    // SMCLK in UP Mode, timer cleared with interrupts enabled
   TA0CCR0 = (clkfrq * PWMperiod) - 1;  //Sets the pulse width based on the clock frequency and PWM period
   TA0CCR2 = 0;                         //Start PWM at 0
   TA0CCTL2 = OUTMOD_7;                 // PWM output to reset/set
   P1SEL |= BIT3;                       // PWM output pin
   P1DIR |= BIT3;                       // PWM output pin
}

void sendData(void){                    //Initial send data function
                                        //Below is the command to publish to the topic TestTopic2. It sends ON.
    char data[] = {'#', 'T', 'e', 's', 't', 'T', 'o', 'p', 'i', 'c', '2', ' ', 'O', 'N', '.', '\n'};
    int counter = 0;                    //Counter that starts at 0 and counts to the length of the array
    while(counter <= 15){               //While the counter is less than the length of the array
        while(!(UCA1IFG&UCTXIFG));      //While the flags have gone off
        UCA1TXBUF = data[counter];      //The TX Buffer gets the next element in the array
        counter++;                      //Increment the counter
    }
}

void subscribeToPewDiePie(void){        //Subscribes to a certian topic: TestTopic2
                                        //Below is the command to subscribe to TestTopic2
    char topic[] = {'$', 'T', 'e', 's', 't', 'T', 'o', 'p', 'i', 'c', '2', '\n'};
    int topicLength = 0;                //topic Length counter that counts from 0 - the max length
    while(topicLength <= 11){           //While the counter is less than the max length
        while(!(UCA1IFG&UCTXIFG));      //Wile the flags have gone off
        UCA1TXBUF = topic[topicLength]; //The TX buffer gets the next element in the array
        topicLength++;                  //Increment the counter
    }
}
void initializeUART(void){               //initialize UART
    P4SEL |= BIT4+BIT5;                  // P4.4,5 = USCI_A1 TXD/RXD
    UCA1CTL1 |= UCSWRST;                 // **Put state machine in reset**
    UCA1CTL1 |= UCSSEL_2;                // SMCLK
    UCA1BR0 = 9;                         // 1MHz 115200 (see User's Guide)
    UCA1BR1 = 0;                         // 1MHz 115200
    UCA1MCTL |= UCBRS_1 + UCBRF_0;       // Modulation UCBRSx=1, UCBRFx=0
    UCA1CTL1 &= ~UCSWRST;                // **Initialize USCI state machine**
    UCA1IE = UCRXIE;                     // Enable USCI_A0 RX interrupt
}
int main(void)                           //Main function
{
	WDTCTL = WDTPW | WDTHOLD;	         //stop watchdog timer
	initializeUART();                    //Runs the intialize UART function
	cfgServo();                          //Runs the configure Servo PWM function
	subscribeToPewDiePie();              //Runs the function to subscribe to TestTopic2
	initGPIO();                          //Runs the initialize the GPIO pin
	sendData();                          //Runs the function to publish the opening message
    _BIS_SR(LPM0_bits + GIE);            //Turns on low power mode and enables global interrupts
	return 0;                            //Returns default 0
}
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)            // Port 1 interrupt service routine
{
    if(P1IN & BIT4){                     //we got a 1 from the fingerprint sensor
        char data[] = {'#', 'T', 'e', 's', 't', 'T', 'o', 'p', 'i', 'c', '2', ' ', 'V', 'E', 'R', 'I', 'F', 'I', 'E', 'D', '\n'};
        int counter = 0;                 //Initialize counter
        while(counter <= 20){            //Loop while the counter is less than the length
           while(!(UCA1IFG&UCTXIFG));    //While the flags have gone off
           UCA1TXBUF = data[counter];    //TX buffer gets the next element in the array
           counter++;                    //Increment counter
        }
    }else{ //its a zero
        char data[] = {'#', 'T', 'e', 's', 't', 'T', 'o', 'p', 'i', 'c', '2', ' ', 'U', 'N', 'V', 'E', 'R', 'I', 'F', 'I', 'E', 'D', '\n'};
        int counter = 0;                 //Initialize counter
        while(counter <= 22){            //Loop while the counter is less than the length
             while(!(UCA1IFG&UCTXIFG));  //While the flags have gone off
             UCA1TXBUF = data[counter];  //TX buffer gets the next element in the array
             counter++;                  //Increment counter
        }
    }
    P1IFG &= ~BIT4;                      //Clear interrupt flag for GPIO
}
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)          //UART interupt vector
{
       inputData[inputCounter] = UCA1RXBUF; //When the interrupt goes off, save whatever is in the RX buffer to the array
       if(inputData[inputCounter] == 10){   //If the next line character hasn't been reached
           dataR = inputData[13] - '0';     //Have the dataR variable get the specific element where the verification bit is stored minus the ascii value '0'
           turnServo(dataR);                //Turn the servo to whatever the dataR variable sets it to
           inputCounter = 0;                //Reset the counter
       }else{                               //Else
           inputCounter++;                  //Increment counter
       }
    }
