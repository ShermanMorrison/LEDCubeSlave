/*
 * PWM Control of an LED Matrix layer.
 */

#include <msp430g2553.h>		//must include so compiler knows what each variable means

const int led_num_cycles = 12;
const char col_array[5] = {BIT0, BIT3, 0x00, BIT6, BIT7};
const char row_array[5] = {BIT0, BIT1, BIT2, BIT3, BIT4};
int pwm_array[5][5] = {
		{0,			0,		0,		0,		0},
		{0,			0,		0,		0,		0},
		{0,			0,		 0, 	0,		0},
		{0,			0,		 0,		0,		0},
		{0,			0,		 0,		0,		0}
}; //row,col. Elements go from 0 to led_num_cycles
int spi_index = 0;

void set_column(int c, int j);
void clear_all();

void main(void){

	WDTCTL = WDTPW + WDTHOLD;	// Stop WDT
//	IE1 = WDTIE;
//	WDTCTL = WDT_ADLY_16;


	P1OUT &= (BIT0 + BIT3 + BIT6 + BIT7);
	P1DIR |= (BIT0 + BIT3 + BIT6 + BIT7);

	P2OUT &= (BIT5 + BIT0 + BIT1 + BIT2 + BIT3 + BIT4);
	P2DIR |= (BIT5 + BIT0 + BIT1 + BIT2 + BIT3 + BIT4);
	

	BCSCTL3 |= LFXT1S_2;					// Set clock source to VLO
//	_BIS_SR(LPM3_bits + GIE);				// Set clock mode to LPM3, and enable interrupts
//	__enable_interrupt();
	BCSCTL2 |= SELM_3 + SELS;          		// SMCLK  = MCLK = VLO = 12KHz

//	BCSCTL1 |= DIVA0;						// Divide ACLK (sourced by VLO) by 2 to get 6KHz clock ...
//	BCSCTL1 &= ~(DIVA0);					// cont.

//	IFG1 &= ~OFIFG;                     	// Clear OSCFault flag

	//__enable_interrupt();		//global interrupt enable


	while(1){

		int c; // global intensity setting, later will be an array with 25 (5x5) elements
		int j; // cycle index running from 0 to 15
		int k;

		// set the control array
		int row,col;
		int nextRow [5];
		for (col=0;col<5;col++)
			nextRow[col] = pwm_array[4][col];
		for (row=4;row>0;row--){
			for (col=0;col<5;col++)
				pwm_array[row][col] = pwm_array[row-1][col];
		}
		for (col=0;col<5;col++)
			pwm_array[0][col] = nextRow[col];

//		pwm_array[0][1] = 10 - pwm_array[0][1];
		// set the led matrix to a state for 1000 iterations

		while (spi_index < 25){
			while  (!(IFG2 &   UCA0RXIFG));    //  USCI_A0 RX  buffer  ready?
			pwm_array[spi_index / 5][spi_index % 5] = UCA0RXBUF;
			spi_index++;
		}

		int index;
		for (index=0; index<5;index++){
			for (k=0;k<1; k++){
				for (c=0; c<5; c++){
					for (j=0; j<led_num_cycles; j++){
						//PWM all lights
						set_column(c,j);

		//				_BIS_SR(LPM3_bits);	//enter low-power mode.
					}
					//clear element (column goes low, desired rows go high)
					clear_all();
				}
			}
		}
	}

}

void set_column(int c, int j){
	// Must have 0 <= r,c < 5

	/////////
	//COLUMN
	//

	P1OUT &= ~(BIT0 + BIT3 + BIT6 + BIT7);			//set all columns low
	P2OUT &= ~BIT5; 	//cont.

	//set appropriate column high
	if (col_array[c] == 0)
		P2OUT |= BIT5;
	else
		P1OUT |= col_array[c];

	///////
	//ROWS
	//

	P2OUT |= (BIT0 + BIT1 + BIT2 + BIT3 + BIT4); // set all rows high.

	// set desired rows low. This is where the PWM happens.
	int r;
	for (r=0;r<5;r++){
			if (j < pwm_array[r][c]){
				P2OUT &= ~(row_array[r]);	//set row low
			}
			else{
				P2OUT |= row_array[r];		//set row high
			}
	}
}



void clear_all(){
	// Must have 0 <= r,c < 5

	///////////////////
	//FIRST SET ROWS!
	//
	P2OUT |= 0xFFEF; //don't set pin 2.5 (column 2)

	//////////////////
	//NOW CLEAR COLUMNS
	//
	P1OUT  &= ~(BIT0 + BIT3 + BIT6 + BIT7);
	P2OUT &= ~BIT5; 	//cont.

}

#pragma vector = WDT_VECTOR   //says that the interrupt that follows will use the "TIMER0_A0_VECTOR" interrupt
__interrupt void WDT_Interrupt(void){     //can name the actual function anything

	_bic_SR_register_on_exit(LPM3_bits);
}






