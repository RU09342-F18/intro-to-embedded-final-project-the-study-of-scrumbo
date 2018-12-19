#include <msp430.h> 


/**
 * main.c
 */
#define lockPos 640
#define unlockPos 1450
#define clkfrq 1000000
#define PWMperiod .02


int inputCounter = 0;
int dataR = 0;
int dataS = 0;
char inputData[50];

void initGPIO(void){

        P1DIR &= ~BIT4;                            // Set P1.0 to input direction
        P1REN |= BIT4;                            // Enable P1.4 internal resistance
        P1SEL &= ~BIT4;

        P1IE |= BIT4;                             // P1.1 interrupt enabled
        P1IES &= ~BIT4;                           // P1.1 rising edge
        P1IFG &= ~BIT4;                           // P1.1 IFG cleared


}

void turnServo(int isVerified){
    if(isVerified == 1){
        TA0CCR2 = unlockPos;
    }else if(isVerified == 0){ //isVerified == 0
        TA0CCR2 = lockPos;
    }
    dataR = 0;

}
void cfgServo(void){
   TA0CTL = TASSEL_2 + MC_1 + TACLR; // SMCLK in UP Mode, timer cleared with interrupts enabled
   TA0CCR0 = (clkfrq * PWMperiod) - 1;
   TA0CCR2 = 0;

   TA0CCTL2 = OUTMOD_7;         // PWM output to reset/set

   P1SEL |= BIT3;               // PWM output pin
   P1DIR |= BIT3;               // PWM output pin
}

void sendData(void){
    char data[] = {'#', 'T', 'e', 's', 't', 'T', 'o', 'p', 'i', 'c', '2', ' ', 'O', 'N', '.', '\n'};
    int counter = 0;
   // for(counter = 0; counter != 16; counter++){
    while(counter <= 15){
        while(!(UCA1IFG&UCTXIFG));
        UCA1TXBUF = data[counter];
        counter++;
    }
}

void subscribeToPewDiePie(void){
    char topic[] = {'$', 'T', 'e', 's', 't', 'T', 'o', 'p', 'i', 'c', '2', '\n'};
    int topicLength = 0;
    //for(topicLength = 0; topicLength != 12; topicLength++){
    while(topicLength <= 11){
        while(!(UCA1IFG&UCTXIFG));
        UCA1TXBUF = topic[topicLength];
        topicLength++;
    }
    //}
}
void initializeUART(void){                     //initialize UART
    P4SEL |= BIT4+BIT5;                       // P4.4,5 = USCI_A1 TXD/RXD
    UCA1CTL1 |= UCSWRST;                      // **Put state machine in reset**
    UCA1CTL1 |= UCSSEL_2;                     // SMCLK
    UCA1BR0 = 9;                              // 1MHz 115200 (see User's Guide)
    UCA1BR1 = 0;                              // 1MHz 115200
    UCA1MCTL |= UCBRS_1 + UCBRF_0;            // Modulation UCBRSx=1, UCBRFx=0
    UCA1CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
    UCA1IE = UCRXIE;                         // Enable USCI_A0 RX interrupt
}

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
	initializeUART();
	cfgServo();
	subscribeToPewDiePie();
	initGPIO();
	sendData();


    _BIS_SR(LPM0_bits + GIE);

	return 0;
}

// Port 1 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
    //P1OUT^=BIT0;
    if(P1IN & BIT4){ //we got a 1 from the fingerprint sensor
        char data[] = {'#', 'T', 'e', 's', 't', 'T', 'o', 'p', 'i', 'c', '2', ' ', 'V', 'E', 'R', 'I', 'F', 'I', 'E', 'D', '\n'};
        int counter = 0;
        while(counter <= 20){
           while(!(UCA1IFG&UCTXIFG));
           UCA1TXBUF = data[counter];
           counter++;
        }
    }else{ //its a zero
        char data[] = {'#', 'T', 'e', 's', 't', 'T', 'o', 'p', 'i', 'c', '2', ' ', 'U', 'N', 'V', 'E', 'R', 'I', 'F', 'I', 'E', 'D', '\n'};
        int counter = 0;
        while(counter <= 22){
             while(!(UCA1IFG&UCTXIFG));
             UCA1TXBUF = data[counter];
             counter++;
        }
    }
    P1IFG &= ~BIT4;


}
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)          //UART interupt vector
{


       inputData[inputCounter] = UCA1RXBUF;
       if(inputData[inputCounter] == 10){
           dataR = inputData[13] - '0';
           turnServo(dataR);
           inputCounter = 0;
       }else{
           inputCounter++;
       }
    }



