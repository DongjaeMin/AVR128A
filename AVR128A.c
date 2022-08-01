#define F_CPU 14745600L
#define  PORT_LED      PORTA
#define  PIN_SENSOR   PIND

#define PORT_DATA	PORTD
#define PORT_CONTROL PORTB
#define DDR_DATA	DDRD
#define DDR_CONTROL	DDRB

#define RS_PIN	0
#define RW_PIN	1
#define E_PIN	2

#define COMMAND_CLEAR_DISPLAY	0x01
#define COMMAND_8_BIT_MODE	0x38

#define COMMAND_DISPLAY_ON_OFF_BIT	2
#define COMMAND_CURSOR_ON_OFF_BIT	1
#define COMMAND_BLINK_ON_OFF_BIT	0

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

unsigned int adc_data= 0;
int sw_flag=0;
char rx_ch;
char str[50];
char rx_buf[50];
int rx_cnt=0;
int rx_flag = 0;

unsigned int read_adc(void)
{
	ADCSRA |= 0x40; // ADC 변환 시작
	while( 1 ) {
		if((ADCSRA & 0x10) != 0 ) break;
	}
	return ADCW ; //ADC 값 리턴
}

void LCD_pulse_enable(void)	//하강 에지에서 동작
{
	PORT_CONTROL |= (1<< E_PIN);
	_delay_ms(1);
	PORT_CONTROL &= ~(1<< E_PIN);
	_delay_ms(1);
}
void LCD_write_data(uint8_t data)
{
	PORT_CONTROL |= (1<<RS_PIN);	//문자 출력에서 RS는 1
	PORT_DATA = data;				//출력할 문자 데이터
	LCD_pulse_enable();
	_delay_ms(2);
}
void LCD_write_command(uint8_t command)
{
	PORT_CONTROL &= ~(1<<RS_PIN);
	PORT_DATA = command;
	LCD_pulse_enable();
	_delay_ms(2);
}
void LCD_clear(void)
{
	LCD_write_command(COMMAND_CLEAR_DISPLAY);
	_delay_ms(2);
}
void LCD_init(void)
{
	_delay_ms(50);
	
	DDR_DATA = 0xFF;
	PORT_DATA = 0x00;
	DDR_CONTROL |= (1<<RS_PIN) | (1<<RW_PIN) | (1<<E_PIN);
	
	PORT_CONTROL &= !(1<<RW_PIN);
	
	//LCD_write_command(COMMAND_8_BIT_MODE);
	
	uint8_t command = 0x08 | (1<<COMMAND_DISPLAY_ON_OFF_BIT);
	LCD_write_command(command);
	
	LCD_clear();
	LCD_write_command(0x06);
}
void LCD_write_string(char *string)
{
	uint8_t i;
	for(i=0;string[i];i++)
	LCD_write_data(string[i]);
}

void LCD_goto_XY(uint8_t row, uint8_t col)
{
	col %= 16;
	row %= 2;
	
	uint8_t address = (0x40 * row) + col;
	uint8_t command = 0x80 + address;
	
	LCD_write_command(command);
}

void UART0_transmit(char data)	//송 이전 전송 끝나기 기다림
{
	while(!(UCSR0A & (1<<UDRE0)));	//UDRE0 = 5;	비여있어야 돌아감
	//비어있는 상태 0 & 1 = 0 => 0(반전하면) 1임
	//if(data == )
	UDR0 = data;
}
uint8_t UART0_receive(void)	///수신 기다림
{
	while(!(UCSR0A & (1<<RXC1)));	//내용이 있어야 돌아감
	return UDR0;	//받은걸 꺼내와서 읽어와야 함
}

void UART0_print_string(char *str){
	for(int i=0; str[i];i++)
	UART0_transmit(str[i]);
}

ISR(INT4_vect)
{
	//sw_flag=1;
	PORTE=0x42;	//BUZZER ON
	_delay_ms(1000);
	PORTE=0x02;	//BUZZER OFF
	_delay_ms(10);
}

int main(void)
{
	char ch[100];
	char data;
	int temp;
	int humi;
	int j=0;
	
	char *result;
	char *parsing[3];
	
	char buffer[100]="";
	char ex_buffer[100]="";
	int index=0;
	int process_data=0;
	
	
	DDRE = 0x42; //0100 0010
	DDRA = 0xff;
	DDRF= 0x00; // PF0를 입력핀으로 설정
	ADMUX =0x00; // 채널 0 선택
	ADCSRA = 0x83;
	_delay_us(10);
	
	UCSR0B = (1<<RXEN0) | (1<<TXEN0);	//송수신 가능하게  |(1<<RXCIE0)
	UBRR0L = 95;	//보드레이트
	
	EIMSK |= (1<<INT4);
	EICRB |= (1<<ISC40) | (1<<ISC41);
	sei();
	LCD_init();
	while(1)
	{
		data = UART0_receive();
		
		if(data=='\n'){
			buffer[index]='\0';
			process_data=1;
		}
		else{
			buffer[index]=data;
			index++;
		}
		if(process_data==1)
		{
			result = strtok(buffer,":");
			while(result!=NULL)
			{
				parsing[j++]=result;
				result = strtok(NULL,":");
			}
			strcpy(ch,parsing[0]);
			temp = atoi(parsing[1]);
			humi = atoi(parsing[2]);
			sprintf(ex_buffer,"%s:%d:%d",ch,temp,humi);
			LCD_clear();
			LCD_write_string(ex_buffer);
			
			if(!(strcmp(ch,"N")))
			{
				LCD_clear();
				LCD_write_string(ex_buffer);
				while(1)
				{
					PORTA=0xFF;
					break;
				}
			}
			if(!(strcmp(ch,"S")))
			{
				LCD_clear();
				LCD_write_string(ex_buffer);
				while(1)
				{
					PORTA =0x00;
					_delay_ms(1);
					adc_data = read_adc();
					if(adc_data >=200)
					{
						PORTA =0xFF;	//LED ON
						_delay_ms(2000);
						PORTA =0x00;
						_delay_ms(100);
						PORTA =0x00;
						break;
					}
				}
			}
			if(!(strcmp(ch,"D")))
			{
				while(1)
				{
					LCD_clear();
					LCD_write_string(ex_buffer);
					PORTA =0x00;
					_delay_ms(1);
					adc_data = read_adc();
					if(adc_data >=200)
					{
						for(int i =0; i<5; i++)
						{
							PORTE|=0x42;	//BUZZER ON
							PORTA =0xa0;	//LED ON(1010 0000)
							_delay_ms(100);
							PORTE&=0x02;	//BUZZER OFF
							PORTA =0x0a;	//LED ON(0000 1010)
							_delay_ms(100);
						}
						PORTE&=0x02;	//BUZZER OFF
						PORTA =0x00;	//LED OFF
						break;
					}
				}
			}
			process_data=0;
			index=0;
		}
	}
}

