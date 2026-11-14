// Line Following Robot
// Clock: 7372800Hz

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "lcd.c"

unsigned char ADC_Conversion(unsigned char);
unsigned char ADC_Value;
unsigned char l = 0;
unsigned char c = 0;
unsigned char r = 0;
unsigned char PortBRestore = 0;

//Variable definitions
float error = 0;
float prev_error = 0;

float Kp = 1;  
float Kd = 0.1;   

#define threshold 20
#define maxspeed 250
#define normalspeed 200
#define minspeed 80

#define minpwm 50
//Variable difintion end

void motion_pin_config (void)
{
	DDRB = DDRB | 0x0F;   //set direction of the PORTB3 to PORTB0 pins as output
	PORTB = PORTB & 0xF0; // set initial value of the PORTB3 to PORTB0 pins to logic 0
	DDRD = DDRD | 0x30;   //Setting PD4 and PD5 pins as output for PWM generation
	PORTD = PORTD | 0x30; //PD4 and PD5 pins are for velocity control using PWM
}

void motion_set (unsigned char Direction)
{
	unsigned char PortBRestore = 0;
	Direction &= 0x0F;
	PortBRestore = PORTB;
	PortBRestore &= 0xF0;
	PortBRestore |= Direction;
	PORTB = PortBRestore;
}

void forward (void) { motion_set(0x06); }
void back (void)    { motion_set(0x09); }
void left (void)    { motion_set(0x05); }
void right (void)   { motion_set(0x0A); }
void soft_left (void)   { motion_set(0x04); }
void soft_right (void)  { motion_set(0x02); }
void soft_left_2 (void) { motion_set(0x01); }
void soft_right_2 (void){ motion_set(0x08); }
void hard_stop (void)   { motion_set(0x00); }
void soft_stop (void)   { motion_set(0x0F); }


void adc_init()
{
	ADCSRA = 0x00;
	ADMUX = 0x20;
	ACSR = 0x80;
	ADCSRA = 0x86;
}

void lcd_port_config (void)
{
	DDRC = DDRC | 0xF7;
	PORTC = PORTC & 0x80;
}

void adc_pin_config (void)
{
	DDRA = 0x00;
	PORTA = 0x00;
}

void port_init()
{
	lcd_port_config();
	adc_pin_config();
	motion_pin_config();
}

void timer1_init(void)
{
	TCCR1B = 0x00;
	TCNT1H = 0xFF;
	TCNT1L = 0x01;
	OCR1AH = 0x00;
	OCR1AL = 0xFF;
	OCR1BH = 0x00;
	OCR1BL = 0xFF;
	ICR1H  = 0x00;
	ICR1L  = 0xFF;
	TCCR1A = 0xA1;
	TCCR1B = 0x0D;
}
//function to give PWM values
void velocity (unsigned char left_motor, unsigned char right_motor)
{
	OCR1AL = left_motor;
	OCR1BL = right_motor;
}

void init_devices (void)
{
	cli();
	port_init();
	adc_init();
	timer1_init();
	sei();
}

unsigned char ADC_Conversion(unsigned char Ch)
{
	unsigned char a;
	Ch = Ch & 0x07;
	ADMUX= 0x20| Ch;
	ADCSRA = ADCSRA | 0x40;
	while((ADCSRA&0x10)==0);
	a=ADCH;
	ADCSRA = ADCSRA|0x10;
	return a;
}
//limiting PWM to 8 bits
int pwm_limiter(int value){
	if(value>255)
		return 255;
	if(value<0)
		return 0;
	return value;
}

int main(void)
{
	//local variables
	float correction;
	int left_speed;
	int right_speed;
	int normal_speed;
	
	init_devices();
	lcd_set_4bit();
	lcd_init();

	while(1)
	{
		// Sensor readings
		l = ADC_Conversion(3);
		c = ADC_Conversion(4);
		r = ADC_Conversion(5);

		lcd_print(1, 1, l, 3);
		lcd_print(1, 5, c, 3);
		lcd_print(1, 9, r, 3);

		
		//all sensors read black
		if((l>threshold) && (c>threshold) && (r>threshold)){
			forward();
			velocity(normalspeed, normalspeed);
			prev_error =0;
			_delay_ms(2);
			continue;
		}
		
		//all sensors read white
		if((l<threshold) && (c<threshold) && (r<threshold)){
			if(prev_error>0){
				left();
				velocity(100,160);
			}
			else{
				right();
				velocity(160,100);
			}
			_delay_ms(2);
			continue;
		}
		
		// error = l - r;
		error = (float)l - (float) r;
		correction = (Kp*error) + (Kd*(error - prev_error)); 
		
		//speed calculations
		normal_speed = normalspeed - (int)(0.5* ((error>=0)?error: -error));
			
			//speed limiters
		if(normal_speed > maxspeed)
			normal_speed = maxspeed;
			
		if(normal_speed < minspeed)
			normal_speed = minspeed;
			//for turning variable speed
		left_speed = normal_speed - (int)correction;
		right_speed = normal_speed + (int)correction;
		
		left_speed = pwm_limiter(left_speed);
		right_speed = pwm_limiter(right_speed);
		
		if((left_speed > 0) && (left_speed < minpwm)){
			left_speed = minpwm;
		}
		
		if(right_speed > 0 && (right_speed < minpwm)){
			right_speed = minpwm;
		}
		//movement commands
		forward();
		velocity((unsigned char) left_speed, (unsigned char) right_speed);
		
		prev_error = error;
		_delay_ms(2);
		
		
		
	}
}