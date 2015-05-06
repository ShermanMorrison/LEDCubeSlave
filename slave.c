/*
 * PWM Control of an LED Matrix layer.
 */
#include <stdint.h>
#include <msp430g2553.h>		//must include so compiler knows what each variable means

const int led_num_cycles = 12;
const uint8_t col_array[5] = {BIT0, BIT3, 0x00, BIT6, BIT7};
const uint8_t row_array[5] = {BIT0, BIT1, BIT2, BIT3, BIT4};
uint8_t pwm_array[5][5] = {
		{0,			0,		 0,		0,		0},
		{0,			0,		 0,		0,		0},
		{0,			0,		 0, 	0,		0},
		{0,			0,		 0,		0,		0},
		{0,			0,		 0,		0,		0}
}; //row,col. Elements go from 0 to led_num_cycles
int spi_index = 0;

void set_column(int c, int j);
void clear_all();

void main(void){

	WDTCTL = WDTPW + WDTHOLD;	// Stop WDT

	BCSCTL3 |= LFXT1S_2;					// Set clock source to VLO (only option-- don't have external oscillator)
	BCSCTL1 |= DIVA_0;						// ACLK is sourced on VLO by default. Run at 12KHz.
	BCSCTL2 |= SELM_3 + SELS + DIVM_0;   	// SMCLK  = MCLK = VLO = 12KHz

	P1OUT &= ~(BIT0 + BIT3 + BIT6 + BIT7);
	P1DIR |= (BIT0 + BIT3 + BIT6 + BIT7);

	P2OUT &= ~(BIT5 + BIT0 + BIT1 + BIT2 + BIT3 + BIT4);
	P2DIR |= (BIT5 + BIT0 + BIT1 + BIT2 + BIT3 + BIT4 + BIT7);

	while (!(P1IN & BIT4));                   // If clock sig from mstr stays low,
											// it is not yet in SPI mode

	P1SEL  =   BIT1 	|BIT2    |   BIT4	| BIT5;	//enable UCA0 transmit and clock
	P1SEL2 =   BIT1 	|	BIT2    |   BIT4	| BIT5;
	UCA0CTL1 = UCSWRST;                       // **Put state machine in reset**
	UCA0CTL0 |= UCCKPL + UCMSB + UCSYNC + UCMODE_1;      // 3-pin, 8-bit SPI master
	UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
	IE2 |= UCA0RXIE;                          // Enable USCI0 RX interrupt

	__bis_SR_register(GIE);       // Enter LPM4, enable interr

	while(1){



		int c; // column index
		int j; // pwm counter

		for (c=0; c<5; c++){

			for (j=0; j<led_num_cycles; j++){

				set_column(c,j); 	//PWM all lights

			}
			__delay_cycles(1000);
			clear_all();			//clear element (column goes low, desired rows go high)
			__delay_cycles(1000);
		}

		//TO REMOVE


	}

}

void set_column(int c, int j){
	// Must have 0 <= r,c < 5

	/////////
	//COLUMN
	//
	int col_mask = 0;


//	col_mask &= ~(BIT0 + BIT3 + BIT6 + BIT7);			//set all columns low

	//set appropriate column high
	if (col_array[c] == 0){
		P2OUT |= BIT5;
		P1OUT &= ~(BIT0 + BIT3 + BIT6 + BIT7);
	}
	else{
		col_mask |= col_array[c];
		P1OUT = col_mask;
		P2OUT &= ~BIT5;
	}
	///////
	//ROWS
	//

	int row_mask_on = (BIT0 + BIT1 + BIT2 + BIT3 + BIT4);

//	P2OUT |= (BIT0 + BIT1 + BIT2 + BIT3 + BIT4); // set all rows high.

	// set desired rows low. This is where the PWM happens.
	int r;
	for (r=0;r<5;r++){
			if (j < (int) pwm_array[r][c]){
				row_mask_on &= ~(row_array[r]);	//set row low
				P2OUT &= ~row_array[r];
			}
	}

	P2OUT |= row_mask_on;
}



void clear_all(){
	// Must have 0 <= r,c < 5


	P1OUT &= ~(BIT0 + BIT3 + BIT6 + BIT7);
	P2OUT &= ~BIT5;
	P2OUT |= (BIT0 + BIT1 + BIT2 + BIT3 + BIT4);


//	///////////////////
//	//FIRST SET ROWS!
//	//
//	P2OUT |= 0xFFEF; //don't set pin 2.5 (column 2)
//
//	//////////////////
//	//NOW CLEAR COLUMNS
//	//
//	P1OUT  &= ~(BIT0 + BIT3 + BIT6 + BIT7);
//	P2OUT &= ~BIT5; 	//cont.

}


#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR (void)
{
	uint8_t in = UCA0RXBUF;
	*((uint8_t*)pwm_array + spi_index) = in;	//read receive buffer

	spi_index++;

	if (spi_index > 24)
		spi_index = 0;
}






