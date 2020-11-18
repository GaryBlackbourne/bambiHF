#include "myfunctions.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "em_device.h"
#include "em_chip.h"
#include <em_gpio.h>
#include <em_cmu.h>
#include <em_usart.h>
#include "segmentlcd.h"
#include "segmentlcd_individual.h"


//a program által használ változók
_Bool StartGame = false; 			//ezzel a flaggel ellenőrizzük le, hogy elkezdődött-e már a játék
uint8_t BananaCounter; 				//ez a banánokat számolja
const uint8_t MaxNumOfBanana = 25;  //konstans beállítása a banánok számának korlátozására
int FallingTime = 450;			    //ez lesz a banánok esésének az ideje milliszekudomban, ez az alapértelmezett 450 ms-os beállítás
uint8_t SpeedLvl = 1;			    //a nehézségi szint alapértelmezetten 1-es (a legkönnyebb szint), ezt küldi vissza az UART a számítógépnek
uint8_t BasketState;				//ez a változó tárolja a kosár állapotát
uint8_t rx_data;					//ide várjuk az UART-ból érkező adatokat
_Bool rx_flag = false;				//szükségünk van egy flagre amivel jelezni szeretnénk azt, hogy az rx-en adat érkezett
uint32_t msTicks;					//ms számláló




//ezek a typedefek a tantárgy honlapján talált segmentlcd_individual projektből vannak
//a játék működöséhez a lowerCharSegments szükséges
SegmentLCD_UpperCharSegments_TypeDef upperCharSegments[SEGMENT_LCD_NUM_OF_UPPER_CHARS];
SegmentLCD_LowerCharSegments_TypeDef lowerCharSegments[SEGMENT_LCD_NUM_OF_LOWER_CHARS];






void SysTick_Handler(void)
{
  msTicks++;       //SysTick megszakítás kezelő függvény, ms-onként növeli 1-el msTicks értékét
}


void UART0_RX_IRQHandler(void){ 			//UART0 megszakításkezelő függvény
	rx_flag = true;							//rx_flaget beállítjuk, hogy tudjunk arról, ha megszakítás történt, vagyis adat érkezett az UART0-ba
	rx_data = USART_Rx(UART0); 				//ezt az adatot kiolvassuk és elmentjuk rx_data-ba
	USART_IntClear(UART0, _USART_IF_MASK);  //törlünk minden interrupt flaget
}


void UART0Begin(){ 							//amikor még nem indult el a játék akkor, ez a függvény várja az UART0-tól érkező adatokat
	key b;
	  if(rx_flag){							//ha rx_flat==false, akkor ne csináljon semmit
		rx_flag = false;					//egyébként kezeljük le az inputot
		b = InputHandler(rx_data);
		  if(b == START){					//kezdje el a játékot ha a felhasználó 's' karaktert küldött
			  if(!StartGame){
			  GameStarted();
			  }
		  }
		  if((b == PLUS) || (b == MINUS)){  //állítsa be a sebességet, ha '+' vagy '-' karakter érkezett
			  SetSpeed(b);
			  USART_Tx(UART0, 0x30+SpeedLvl); //visszaküldi az nehézségi fokozat értékét (0x30 a 0 ASCII kódja)
		  }
	  }
}


void UART0InGame(){ 						//játék közben ez a függvény olvassa az inputot
	key b;
	if(rx_flag){ 						//ha nem történt megszakítás akkor ne csináljon semmit
	  rx_flag = false; //egyébként pedig csak balra vagy jobbra mozoghat a kosár, ilyenkor nem lehet állítani a nehézségi szintet
	  b = InputHandler(rx_data);
	  BasketMove(b);
	 }
}


void DelayInGame(uint32_t dlyTicks) //a játék közben szükség van input kezelésre, ezért külön késleltető függvényt kellett ehhez létrehozni
{
  uint32_t curTicks;
  uint8_t i = 1;
  curTicks = msTicks;				//az éppen adott időpillantban elkapott msTicks értékét el kell menteni ahhoz, hogy időt tudjunk számolni
  while ((msTicks - curTicks) < dlyTicks) { 		//dlyTicks értéknek megfelelő ms időt várunk (pl. dlyTicks = 500; akkor fél másodperc várakozás)
	  if((msTicks - curTicks) > 20*i){				//20 ms-onként lehallgatjuk az inputot
		  i++; UART0InGame();						//ez az érték megfelelő, játék közben szinte észrevehetetlen a késletetés
	  }
  }
}


void Delay(uint32_t dlyTicks) //ez a Delay függvény ugyanúgy működik mint az játékközbeni késleltető függvény, de itt nem hallgatjuk le az inputot
{
  uint32_t curTicks;
  curTicks = msTicks;
  while ((msTicks - curTicks) < dlyTicks) { }
}


key InputHandler(uint8_t c) { //input kezelő függvény, a karaktereknek megfelelően tér vissza
	switch(c){
	case 's': return START;
	case '+': return PLUS;
	case '-': return MINUS;
	case 'b': return LEFT;
	case 'j': return RIGHT;
	default:  return NOTHING;
	}
}


void SetSpeed(key k){						//nehézségi szintet állítja be, attól függően, hogy + vagy - karakter érkezett
	if(k == PLUS){
		SpeedUp();
	}
	else if(k == MINUS){
		SlowDown();
	}
	else
		return;
}


void SpeedUp(){								//ezt hívja meg a SetSpeed függvény, ha '+' karakter érkezett, ilyenkor a nehézségi szintet emeljük
	if(SpeedLvl < 4) {
		SpeedLvl++;							//néhézségi szint emelése egy fokozattal
		FallingTime -= 80;					//kevesebb lesz a FallingTime, vagyis gyorsabban fog leesni a banán
		SegmentLCD_ARing(SpeedLvl, 1);		//jelezzük a 8 szegmenses gyűrűn is, vagyis plusz egy szegmenst felkapcsolunk
		SegmentLCD_Write("LEVEL +");		//az alfanumerikus kijelzőn is kiírjuk
	}
}


void SlowDown(){					    //ezt hívja meg a SetSpeed függvény, ha '-' karakter érkezett, ilyenkor a nehézségi szintet csökkentjük
	if(SpeedLvl > 1){
		SegmentLCD_ARing(SpeedLvl, 0);  //az aktuális szintet jelző szegmenst kikapcsoljuk
		SpeedLvl--;					    //szintet csökkentünk
		FallingTime += 80; 				//80 ms-mal több lesz az esési idő, vagyis lassabban fog esni a banán
		SegmentLCD_Write("LEVEL -"); 	//jelezzük az alfanumerikus kijelzőn is
	}
}


void BasketMoveLeft(){								 //a kosár balra mozgását valóítja meg
	uint8_t recent = BasketState; 					 //a jelenlegi kosár állapotát elmentjük, hogy ki tudjuk kapcsolni a szegmenst
	if(BasketState == 0){ 						 	 //ha már nem tudunk balra menni a kosárral, akkor jobb oldalról jöjjön vissza a kosár
			BasketState = 3;
		}
		else {										 //egyébként lépjen egyet balra a kosár
			BasketState -= 1;
		}
	lowerCharSegments[recent].d = 0;				 //kikapcsoluk az előző állapotot
	SegmentLCD_LowerSegments(lowerCharSegments);
	lowerCharSegments[BasketState].d = 1;			 //és bekapcsoljuk a beállított állapotnak megfelelő alsó szegmenst
	SegmentLCD_LowerSegments(lowerCharSegments);
}


void BasketMoveRight(){ //ez a függvény ugyanazon az elven működik, mint a BasketMoveLeft(), csak ez jobbra viszi a kosarat
	uint8_t recent = BasketState;
	if(BasketState == 3){
		BasketState = 0;
	}
	else {
		BasketState += 1;
	}
	lowerCharSegments[recent].d = 0;
	SegmentLCD_LowerSegments(lowerCharSegments);
	lowerCharSegments[BasketState].d = 1;
	SegmentLCD_LowerSegments(lowerCharSegments);
}


void BasketMove(key Move){ //ez a függvény kapja meg a felsorolás típusú változót, melynek megfelelően meghívja a szükséges függvényt
if(Move == LEFT){
	BasketMoveLeft();
	}
if(Move == RIGHT){
	BasketMoveRight();
	}
}


void GameStarted(){ 								//ez a függvény a játék kezdeti állapotát állítja be
	StartGame = true; 								//bebillenti a StartGame flaget, hogy tudjunk arról, hogy a játék elkezdődött
	BananaCounter = 0; 								//minden értéket resetelünk
	BasketState = 0;
	SegmentLCD_Number(0);
	SegmentLCD_Symbol(LCD_SYMBOL_COL10, 1);
	lowerCharSegments[BasketState].d = 1; 			//kosár alaphelyzetben bal szélen van
	SegmentLCD_LowerSegments(lowerCharSegments);
	SegmentLCD_Write("READY"); 						//felkészíti a játékost, hogy nemsokára indul a játék
	Delay(600);
	SegmentLCD_Write("START");
	Delay(400);
	StartGame=Gaming(); //amint meghívja Gaming függvényt, elkezdődik a játék, a visszatérési érték pedig false értékre állítja a StartGame flaget
}


_Bool Gaming(){									 //ez a függvény valósítja meg magát a játékot
	int i = 0;									 //szükség van két segédváltozóra
	int j = 0;
	srand((unsigned)time(NULL));				 //randomszám generálásához

	while(BananaCounter != MaxNumOfBanana) {
	uint8_t num = rand()%4; 	  //random szám generálása és 4-gyel való modulálása, hogy 0,1,2 vagy 3 értéket kapjunk, hiszen csak 4 mezőn játszunk
    lowerCharSegments[num].a = 1; //a num értéknek megfelelő mezőben felvillan a felső szegmens
    SegmentLCD_LowerSegments(lowerCharSegments); 		// a továbbiakban ebben a szegmensben FallingTime időközönként fokozatosan leesik a banán
    DelayInGame(FallingTime); 	  //itt ráadásként lehallgatjuk az UART-ot, attól függetlenül hogy DelayInGame() is megteszi ezt időközönként
    UART0InGame();
    lowerCharSegments[num].a = 0;
    SegmentLCD_LowerSegments(lowerCharSegments);
    UART0InGame();
    lowerCharSegments[num].j = 1;
    SegmentLCD_LowerSegments(lowerCharSegments);
    DelayInGame(FallingTime);
    UART0InGame();
    lowerCharSegments[num].j = 0;
    SegmentLCD_LowerSegments(lowerCharSegments);
    UART0InGame();
    lowerCharSegments[num].p = 1;
    SegmentLCD_LowerSegments(lowerCharSegments);
    UART0InGame();
    DelayInGame(FallingTime);

	if(BasketState==num){						 //ha a kosár helyzete megegyezik a banán helyzetével, akkor elkaptuk a banánt
		i++;
		SegmentLCD_Number(((i*100)+j));			 //az elkapott banánokat pedig a jobb felső sarokban számoljuk, a kettőspont bal oldalán
		}

	else {
		j++;
		SegmentLCD_Number(((i*100)+j));			 // a leesett banánokat is számoljuk, a kettősponttól jobbra
		}

    lowerCharSegments[num].p = 0;
    SegmentLCD_LowerSegments(lowerCharSegments);
    BananaCounter++;
	}

	//kikapcsoljuk az aktuális kosár szegmenst, hogyha következő játékot indítunk, akkor ne világítson feleslegesen plusz egy szegmens
	lowerCharSegments[BasketState].d = 0;
	SegmentLCD_LowerSegments(lowerCharSegments);
	SegmentLCD_Write("ENDGAME");
   return false; //false értékkel térünk vissza, ez azért lesz fontos, mert a StartGame flaget ez a visszatérési érték fogja nullázni
}


