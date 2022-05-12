
//telemetre à ultrason

#include "LPC17xx.h"
#include "mbed.h"
#include "string.h"
#include "math.h"
#define FPCLK 24e6

// audible homme entre 20hz et 20khz
// la vitesse du  son dans l'air est de 340 m/s
// le signal est emis a 0.7ms
// si le signal met 3ms a etre recu par le recepteur a ultrason
// vitesse  = distance * temps
// pour 3.8ms l'onde a fais un aller retour
// distance (aller retour) = vitesse / temps = 340 / 3*10^-3 = 89e3 distance = distance(aller-retour)/2 = 56.7e3 metres
// teta = k * d dans notre cas teta = 3.8 s et la distance est de 0.3 m k = 3.8/0.3 = 12.6


unsigned int debut;
float periode;
int flag =0;
unsigned int it=0;
AnalogIn ain(A0);
//const int B = 4275;
const int addr = 0x7c;
I2C i2c(D14, D15);
char cmd[2];
char color[2];
char buffer[20];

void init_T0(){
	 LPC_TIM0 -> MR0 = FPCLK * 10e-6 ;// interruption generer toute les 10us
	 LPC_TIM0 -> MR1 = FPCLK * 500e-3 ;//interruptoion generer toute les 500ms
	LPC_TIM0 -> MCR = 0x19 ; // Mr0 et Mr1 sont activer quand les valeur de tc sont atteinte
	NVIC_EnableIRQ(TIME0_IRQn);//
	LPC_TIME ->TCR = 0x01 ; // active le timer 0;

}


extern "C" void TIMER0_URQHandler(void){
	if (LPC_TIME0->IR == 0x01 ){
		LPC_TIME0->IR == 0x01;// aqquitement de mr0
		LPC_GPIO->FIOCLR0 = (1<<4); // P0.4 niv bas
		LPC_PINCON->PINSEL0 |= (3<<8) ;// P0.4 RD2
		LPC_TIM2->CCR =0x05;// captures du Timer 2 sur les pins P0.4 (CAP2.0) et P0.5 (CAP2.1) coresond a D2 D3
	}

	if(LPC_TIME0->IR == 0x02){
		LPC_TIME0->IR == 0x02;//aquittement de mr1
		LPC_GPIO0->FIOSET0 = (1<<4);//P0.4 niveau haut
	}
}


void init_color(void)
    {


        color[0] = 0x00;
        color[1] = 0x00;
        i2c.write(0xC4, color, 2);


        color[0] = 0x01;
        color[1] = 0x00;
        i2c.write(0xC4, color, 2);


        color[0] = 0x08;
        color[1] = 0xAA;
        i2c.write(0xC4, color, 2);

      }










void init_LCD(void)
    {
            wait_ms(30);
            cmd[0] = 0x0;
            cmd[1] = 0x3c;
            i2c.write(addr, cmd, 2);
            wait_us(40);
            cmd[1] = 0x0C;
            i2c.write(addr, cmd, 2);
            wait_us(45);
            cmd[1] = 0x01;
            i2c.write(addr, cmd, 2);
            wait_ms(2);
            cmd[1] = 0x06;
            i2c.write(addr, cmd, 2);

    }




void init_T2(void)
{
	LPC_PINCON->PINSEL0 |= 3<<8;
	LPC_SC->PCONP |= (1<<22);
	LPC_TIM2->CCR = 0x5; //Capture sur front descendant et interruption
	NVIC_EnableIRQ(TIMER2_IRQn);
	LPC_TIM2->TCR = 0x01;
}



extern "C" void TIMER2_IRQHandler(void)
{
		if (flag == 0)
		{

			debut = LPC_TIM2->CR0;
			LPC_TIM2->IR = 0x10;
			flag =1;
			LPC_TIM2->CCR = 0x6;
	}
	else if (flag == 1){
		periode = LPC_TIM2->CR0 - debut;
		it = 1;
		LPC_TIM2->IR = 0x10;
		flag = 0;
		LPC_PINCON->PINSEL0 &= ~(3<<8);
	}
}




void afficher_char(char c)
    {
        cmd[0] = 0x40;
        cmd[1] = c;
        i2c.write(addr, cmd, 2);
    }

 void afficher_phrase(char *s)
    {
        int i;
        for (i=0;i<strlen(s);i++)
            display_char(s[i]);
    }





void init_GPIO(void)
{
	LPC_GPIO0->FIODIR0 |= (1<<4); // P0.4 activé en sortie
	LPC_GPIO0->FIOSET0 = (1<<4); // P0.4 mis a 1
	LPC_GPIO0->FIODIR0 |= 0x01; // P0.0 en sortie
}

int main(void){

	float distance =0;
	init_GPIO();
	init_color();
	init_LCD();
	init_T0();
	//init_T2();

	while(1){

	float R = 5/(3.3*ain)-1.0;

	R = 100000.0*R;

	//float temperature=1.0/(log(R/100000.0)/B+1/298.15);
	//float vraivitesse = 20*sqrt(temperature);

	if (it==1){
		it = 0;
		periode = periode *(40e-9);
		distance = 100.0* R * periode/2.0;
		init_LCD();
		sprintf(buffer, "dist : %.2f cm",distance);

		afficher_phrase(buffer);

		if (distance < 5){
			LPC_GPIO0->FIOSET0 = 0x01;

				color[0] = 0x02; //bleu
			color[1] = 0x00;
			i2c.write(0xC4, color, 2);

			color[0] = 0x03; //vert
			color[1] = 0x00;
			i2c.write(0xC4, color, 2);

			color[0] = 0x04; //rouge
			color[1] = 0xFF;
			i2c.write(0xC4, color, 2);
		}
		else if (distance > 5.0 && distance < 10.0){

			LPC_GPIO0->FIOPIN0 ^= 0x01;
			color[0] = 0x02; //bleu
			color[1] = 0xFF;
			i2c.write(0xC4, color, 2);

			color[0] = 0x03; //vert
			color[1] = 0x00;
			i2c.write(0xC4, color, 2);

			color[0] = 0x04; //rouge
			color[1] = 0x00;
			i2c.write(0xC4, color, 2);
	}

		else {

		LPC_GPIO0->FIOCLR0 = 0x01;
    color[0] = 0x02; //bleu
    color[1] = 0x00;
    i2c.write(0xC4, color, 2);

    color[0] = 0x03; //vert
    color[1] = 0xFF;
    i2c.write(0xC4, color, 2);

    color[0] = 0x04; //rouge
    color[1] = 0x00;
    i2c.write(0xC4, color, 2);

		}


	}

}

}





