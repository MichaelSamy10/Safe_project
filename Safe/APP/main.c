/*
 * Safe.c
 *
 * Created: 06-Jan-24 2:54:32 PM
 * Author : MICHAEL
 */ 
#include "../LIB/STD_TYPES.h"
#include "../LIB/BIT_MATH.h"
#include <util/delay.h>

#include "../MCAL/DIO/DIO_interface.h"
#include "../MCAL/TWI/TWI_interface.h"
#include "../HAL/CLCD/CLCD_interface.h"
#include "../HAL/EEPROM/EEPROM_interface.h"
#include "../HAL/KPD/KPD_interface.h"

#define LCD_CLR_DELAY						2000
#define USER_BLOCK_DELAY					20000
#define PASSWORD_HIDE_DELAY					300

#define PASSWORD_DIGITS						4
#define PASSWORD_TRIES						3

#define FIRST_LOGIN_ADDRESS					100
#define PASS_SET							1

#define RIGHT_PASSWORD						4
#define WRONG_PASSWORD						5

u8 keyPressed;

void passwordCheck(u8 passArr[PASSWORD_DIGITS],u8 Copy_u8CopmareAddress);
void firstLoginPage(void);

int main(void)
{
	DIO_voidInit();
	KPD_voidInit();
	CLCD_voidInit();
	TWI_voidMasterInit(NO_ADDRESS);
	
	u8 PassArr[PASSWORD_DIGITS];

    while (1) 
    {
		firstLoginPage();
		passwordCheck(PassArr,0);
		keyPressed = NOT_PRESSED;
		while(keyPressed == NOT_PRESSED)	//repeat till the user press any key
		{
			keyPressed = KPD_u8GetPressedKey(); //get the user pressed button in keypad and save the value in keyPressed
		}
		if (keyPressed == 'C')
		{
			EEPROM_voidWriteByte(0xFF,FIRST_LOGIN_ADDRESS);
			DIO_u8SetPinValue(DIO_u8PORTC,DIO_u8PIN6,DIO_u8HIGH);	//TurnOFF Green Led
		}
    }
}


void firstLoginPage(void)
{
	/* FIRST TIME LOGIN PAGE */
	/* 
	   firstLoginStatus -> status of login first time or not (0xFF,1) 
	   addresses that password will be saved (0,1,2,3)
	*/	
	u8 firstLoginStatus , address=0 ,column, passCounter=0;
	EEPROM_voidReadByte(&firstLoginStatus,FIRST_LOGIN_ADDRESS);
	/* Check if content of address 100 in EEPROM = 0xFF then this is first login time and then set password for the first time */
	if(firstLoginStatus != PASS_SET)
	{
		CLCD_voidClearDisplay();
		CLCD_voidSendString("Set Pass For");
		CLCD_voidSendStringPosition("First Time",1,0);
		_delay_ms(LCD_CLR_DELAY);
		CLCD_voidClearDisplay();
		column=7;
		CLCD_voidSendString("Set Password");
		CLCD_voidMoveCursor(1,column);
		/* This loop will exit after entering 4 numbers as Password is 4 digits only */
		while(passCounter<PASSWORD_DIGITS){
			keyPressed = NOT_PRESSED;
			while(keyPressed == NOT_PRESSED)	//repeat till the user press any key
			{
				keyPressed = KPD_u8GetPressedKey(); //get the user pressed button in keypad and save the value in keyPressed
			}
			/* Display keyPressed on LCD and after PASSWORD_HIDE_DELAY display '*' */
			CLCD_voidSendCharPosition(keyPressed,1,column);
			_delay_ms(PASSWORD_HIDE_DELAY);
			CLCD_voidSendCharPosition('*',1,column);
			column++;
	
			/* save Admin password in addresses from 0 -> 3 in EEPROM */
			EEPROM_voidWriteByte(keyPressed,address);
			address++;
			passCounter++;
		}
		
		passCounter=0;	address=4;	column=12;
		_delay_ms(LCD_CLR_DELAY);
		CLCD_voidClearDisplay();
		CLCD_voidSendString("Password Saved");
		_delay_ms(LCD_CLR_DELAY);
		CLCD_voidClearDisplay();
		
		
		/* Write PASS_SET(1) in address 100 in EEPROM to indicate status of password as it is set, then this page is not appeared again*/
		EEPROM_voidWriteByte(PASS_SET,FIRST_LOGIN_ADDRESS);

	}
	
}



/* Function to Check password */
void passwordCheck(u8 passArr[PASSWORD_DIGITS],u8 Copy_u8CopmareAddress)
{
	u8 loginFlag=WRONG_PASSWORD ,Local_u8CompareAddress;
	u8 passCounter=0, column;
	while(loginFlag == WRONG_PASSWORD)
	{
			CLCD_voidClearDisplay();
			CLCD_voidSendString("Enter Password");
			DIO_u8SetPinValue(DIO_u8PORTC,DIO_u8PIN5,DIO_u8LOW);
			DIO_u8SetPinValue(DIO_u8PORTC,DIO_u8PIN6,DIO_u8LOW);
			column=5;	//password position
			/* Enter password to login */
			CLCD_voidMoveCursor(1,column);
			while(passCounter<PASSWORD_DIGITS){
				keyPressed = NOT_PRESSED;
				while (keyPressed == NOT_PRESSED)	//repeat till the user press any key
				{
					keyPressed = KPD_u8GetPressedKey();		//if the user pressed any button in keypad save the value in keyPressed
				}
				passArr[passCounter]=keyPressed;//add the entered character to the pass array
				CLCD_voidSendCharPosition(keyPressed,1,column);
				_delay_ms(PASSWORD_HIDE_DELAY);
				CLCD_voidSendCharPosition('*',1,column);
				column++;
				passCounter++;
			}
			passCounter = 0;
			/*
				compareVal-> value in EEPROM to be compared 
				compareArrIndx -> index of array that holds tha password
				compareAddress -> address to be compared with array elements
			*/
			u8 compareVal,compareArrIndx=0;
			/* compare with password saved in EEPROM */
			Local_u8CompareAddress = Copy_u8CopmareAddress;
			while(passCounter<PASSWORD_DIGITS){
 				EEPROM_voidReadByte(&compareVal,Local_u8CompareAddress);
				if(compareVal == passArr[compareArrIndx])
					passCounter++;
				else{
					passCounter = WRONG_PASSWORD;
				}
				compareArrIndx++; Local_u8CompareAddress++;
			}
			/* check if the loop exit for entering right password (passCounter = RIGHT_PASSWORD) or wrong password (passCounter = WRONG_PASSWORD) */
			if(passCounter == RIGHT_PASSWORD){
				_delay_ms(LCD_CLR_DELAY);
				CLCD_voidClearDisplay();
				CLCD_voidSendString("Safe opened");
				DIO_u8SetPinValue(DIO_u8PORTC,DIO_u8PIN6,DIO_u8HIGH);	//TurnOn Green Led
				/* Change password flag to exit from login page and go to Rooms page*/
				loginFlag = RIGHT_PASSWORD;
			}
			else if(passCounter == WRONG_PASSWORD)
			{
				static s8 passTries = PASSWORD_TRIES; // Number of tries is 3
				_delay_ms(LCD_CLR_DELAY);
				CLCD_voidClearDisplay();
				CLCD_voidSendString("Wrong Password");
				passTries--;
				CLCD_voidMoveCursor(1,0);
				CLCD_voidSendString("Tries Left: ");	// display Number of Tries left for the user to enter password
				CLCD_voidSendNum(passTries);	
				_delay_ms(LCD_CLR_DELAY);
				DIO_u8SetPinValue(DIO_u8PORTC,DIO_u8PIN7,DIO_u8HIGH);	// TurnOn Buzzer
				_delay_ms(500);
				DIO_u8SetPinValue(DIO_u8PORTC,DIO_u8PIN7,DIO_u8LOW);	// TurnOff Buzzer
				loginFlag = WRONG_PASSWORD;	passCounter = 0;
				/* No tries left then the user will be blocked for 20 Sec */
				if(passTries == 0){
					DIO_u8SetPinValue(DIO_u8PORTC,DIO_u8PIN5,DIO_u8HIGH);	// TurnOn Red led
					CLCD_voidClearDisplay();
					CLCD_voidSendString("Safe Closed");
					CLCD_voidSendStringPosition("Block For 20 Second",1,0);
					/* delay for 20 sec to Block the user */
					_delay_ms(USER_BLOCK_DELAY);
					//TIMER0_voidOVSetIntervalSynchronousMS(5000);
					CLCD_voidClearDisplay();
					/* Then return to enter password another time */
					loginFlag = WRONG_PASSWORD;	passCounter = 0; passTries=PASSWORD_TRIES;
				}
			}			
		}	// End LOGIN PAGE		

}