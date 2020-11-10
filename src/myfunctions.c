#include "myfunctions.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "em_device.h"
#include "em_chip.h"
#include <em_gpio.h>
#include <em_cmu.h>
#include <em_usart.h>
#include "segmentlcd.h"
#include "segmentlcd_individual.h"

//a program által használ változók
volatile _Bool StartGame = false; //ezzel ellenõrizzük le, hogy elkezdõdött-e a játék
volatile uint8_t BananaCounter; //ez a banánokat számolja
const uint8_t MaxNumOfBanana = 25; //konstans beállítása a banánok számának korlátozására
volatile int FallingTime = 450; //ez lesz a banánok esésének az ideje, ez az alapértelmezett 450 ms-os beállítás
volatile uint8_t SpeedLvl = 1; //a nehézségi szint alapértelmezett 1-es (a legkönnyebb szint), ezt küldi vissza az UART a számítógépnek
volatile uint8_t BasketState; //az a változó tárolja a kosár állapotát
volatile uint8_t rx_data; //ide várjuk az UART-ból érkezõ adatokat
volatile _Bool rx_flag = false; //szükségünk van egy flagre amivel jelezni tudjuk azt, hogy az rx-en adat érkezett
volatile uint32_t msTicks; //ms számláló

//ezek a typedefek a tantárgy honlapján talált segmentlcd_individual projektbõl vannak, ezek szükségesek a játék mûködéséhez
SegmentLCD_UpperCharSegments_TypeDef upperCharSegments[SEGMENT_LCD_NUM_OF_UPPER_CHARS];
SegmentLCD_LowerCharSegments_TypeDef lowerCharSegments[SEGMENT_LCD_NUM_OF_LOWER_CHARS];

void SysTick_Handler(void)
{
  msTicks++;       //SysTick megszakítás kezelõ függvény, ms-onként növeli 1-el msTicks értékét
}

void DelayInGame(uint32_t dlyTicks) //a játék közben szükség van input kezelésre, ezért külön késleltetõ függvényt kellett ehhez létrehozni
{
  uint32_t curTicks;
  uint8_t i = 1;
  curTicks = msTicks; //az éppen adott idõpillantban elkapott msTicks értékét el kell menteni ahhoz, hogy idõt tudjunk számolni
  while ((msTicks - curTicks) < dlyTicks) { //dlyTicks értéknek megfelelõ ms idõt várunk (pl. dlyTicks = 500; akkor fél másodperc várakozás)
	  if((msTicks - curTicks) > 20*i){ //20 ms-onként lehallgatjuk az inputot
		  i++; UART0InGame();			//ez az érték megfelelõ, játék közben szinte észrevehetetlen a késletetés
	  }
  }
}

void Delay(uint32_t dlyTicks) //ez a Delay függvény ugyanúgy mûködik mint az játékközbeni késleltetõ függvény, de itt nem hallgatjuk le az inputot
{
  uint32_t curTicks;
  curTicks = msTicks;
  while ((msTicks - curTicks) < dlyTicks) { }
  }



void UART0_RX_IRQHandler(void){ //UART0 megszakításkezelõ függvény
	rx_flag = true;			//rx_flaget beállítjuk, hogy tudjunk arról, ha megszakítás történt, vagyis adat érkezett az UART0-ba
	rx_data = USART_Rx(UART0); //ezt az adatot kiolvassuk és elmentjuk rx_data-ba
	USART_IntClear(UART0, _USART_IF_MASK); //törlünk minden interrupt flaget
}

void SetSpeed(key k){ //nehézségi szintet állítja be, attól függõen, hogy + vagy - karakter érkezett
	if(k == PLUS){
		SpeedUp();
	}
	else if(k == MINUS){
		SlowDown();
	}
	else
		return;
}

void SpeedUp(){ //ezt hívja meg a SetSpeed függvény, ha + karakter érkezett, ilyenkor a nehézségi szintet emeljük
	if(SpeedLvl < 4) {
		SpeedLvl++; //néhézségi szint emelése egy fokozattal
		FallingTime -= 80; //kevesebb lesz a FallingTime, vagyis gyorsabban fog leesni a banán
		SegmentLCD_ARing(SpeedLvl, 1); //jelezzük a 8 szegmenses gyûrûn is, vagyis plusz egy szegmenst felkapcsolunk
		SegmentLCD_Write("LEVEL +"); // az alfanumerikus kijelzõn is kiírjuk
	}
}

void SlowDown(){ //ezt hívja meg a SetSpeed függvény, ha - karakter érkezett, ilyenkor a nehézségi szintet csökkentjük
	if(SpeedLvl > 1){
		SegmentLCD_ARing(SpeedLvl, 0); //az aktuális szintet jelzõ szegmenst kikapcsoljuk
		SpeedLvl--; //szintet csökkentünk
		FallingTime += 80; //80 ms-mal több lesz az esési idõ, vagyis lassabban fog esni a banán
		SegmentLCD_Write("LEVEL -"); //jelezzük az alfanumerikus kijelzõn is
	}
}

void GameStarted(){ //ez a függvény a játék kezdeti állapotát állítja be
	StartGame = true; //bebillenti a StartGame flaget
	BananaCounter = 0; //nullázza azt amit csak szükséges
	BasketState = 0;
	SegmentLCD_Number(0);
	SegmentLCD_Symbol(LCD_SYMBOL_COL10, 1);
	lowerCharSegments[BasketState].d = 1; //kosár alaphelyzetben bal szélen van
	SegmentLCD_LowerSegments(lowerCharSegments);
	SegmentLCD_Write("READY"); //felkészíti a játékost, hogy nemsokára indul a játék
	Delay(600);
	SegmentLCD_Write("START");
	Delay(400);
	StartGame=Gaming(); //amint meghívja Gaming függvényt, elkezdõdik a játék
}

void BasketMoveLeft(){ //a kosár balra mozgását valóítja meg
	uint8_t recent = BasketState; //a jelenlegi kosár állapotát elmentjük, hogy ki tudjuk kapcsolni a szegmenst
	if(BasketState == 0){ //ha már nem tudunk balra menni a kosárral, akkor jobb oldalról jöjjön vissza a kosár
			BasketState = 3;
		}
		else { //egyébként lépjen egyet balra a kosár
			BasketState -= 1;
		}
	lowerCharSegments[recent].d = 0; //kikapcsoluk az elõzõ állapotot
	SegmentLCD_LowerSegments(lowerCharSegments);
	lowerCharSegments[BasketState].d = 1; //és bekapcsoljuk a beállított állapotnak megfelelõ alsó szegmenst
	SegmentLCD_LowerSegments(lowerCharSegments);
}

void BasketMoveRight(){ //ez a függvény ugyanazo az elven mûködik, mint a BasketMoveLeft, csak ez jobbra viszi a kosarat
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

void BasketMove(key Move){ //ez a függvény kapja meg a felsorolás típusú változót, melynek megfelelõen meghívja a szükséges függvényt
if(Move == LEFT){
	BasketMoveLeft();
}
if(Move == RIGHT){
	BasketMoveRight();
}
}

_Bool Gaming(){ //ez a függvény valósítja meg magát a játékot
	int i = 0; //szükség van két segédváltozóra
	int j = 0;
	while(BananaCounter != MaxNumOfBanana) {
	uint8_t num = rand()%4; //random szám generálása és 4-gyel való modulálása, hogy 0,1,2 vagy 3 értéket kapjunk, hiszen csak 4 mezõn játszunk
    lowerCharSegments[num].a = 1; //a num értéknek megfelelõ mezõben felvillan a felsõ szegmens
    SegmentLCD_LowerSegments(lowerCharSegments); // a továbbiakban ebben a szegmensben FallingTime idõközönként fokozatosan leesik a banán
    UART0InGame(); //itt is lehallgatjuk az UART-ot ráadásként
    DelayInGame(FallingTime);
    UART0InGame();
    lowerCharSegments[num].a = 0;
    SegmentLCD_LowerSegments(lowerCharSegments);
    UART0InGame();
    lowerCharSegments[num].j = 1;
    SegmentLCD_LowerSegments(lowerCharSegments);
    UART0InGame();
    DelayInGame(FallingTime);
    UART0InGame();
    lowerCharSegments[num].j = 0;
    SegmentLCD_LowerSegments(lowerCharSegments);
    UART0InGame();
    lowerCharSegments[num].p = 1;
    SegmentLCD_LowerSegments(lowerCharSegments);
    UART0InGame();
    DelayInGame(FallingTime);

	if(BasketState==num){ //ha a kosár helyzete megegyezik a banán helyzetével, akkor elkaptuk a banánt
		i++;
		SegmentLCD_Number(((i*100)+j)); //az elkapott banánokat pedig a jobb felsõ sarokban számoljuk, a kettõspont bal oldalán
	}
	else {
		j++;
		SegmentLCD_Number(((i*100)+j)); // a leesett banánokat is számoljuk, a kettõsponttól jobbra
	}
    lowerCharSegments[num].p = 0;
    SegmentLCD_LowerSegments(lowerCharSegments);
    BananaCounter++;
	}
	lowerCharSegments[BasketState].d = 0; //kikapcsoljuk a kosarat, hogyha következõ játékot indítunk, akkor ne világítson feleslegesen plusz egy szegmens
	SegmentLCD_LowerSegments(lowerCharSegments);
	SegmentLCD_Write("ENDGAME");
    return false; //false értékkel térünk vissza, ez azért lesz fontos, mert a StartGame flaget ez a visszatérési érték fogja nullázni
}


key InputHandler(uint8_t c) { //input kezelõ függvény, a karaktereknek megfelelõen tér vissza
	switch(c){
	case 's': return START;
	case '+': return PLUS;
	case '-': return MINUS;
	case 'b': return LEFT;
	case 'j': return RIGHT;
	default:  return NOTHING;
	}
}


void ScrollText(char *scrolltext) //ez a függvény használaton kívül van!! talán nem is érdemes használni
{
  int  i, len;
  char buffer[8];

  buffer[7] = 0x00;
  len       = strlen(scrolltext);
  if (len < 7) {
    return;
  }
  for (i = 0; i < (len - 7); i++) {
    memcpy(buffer, scrolltext + i, 7);
    SegmentLCD_Write(buffer);
    Delay(400);
  }
}

void UART0Begin(){ //amikor még nem indult el a játék akkor, ez a függvény várja az UART0-tól érkezõ adatokat
	key b;
	  if(rx_flag){ //ha rx_flat==false, akkor ne csináljon semmit
		rx_flag = false; //egyébként kezeljük le az inputot
		b = InputHandler(rx_data);
		  if(b == START){ //kezdje el a játékot!
			  if(!StartGame){
			  GameStarted();
			  }
		  }
		  if((b == PLUS) || (b == MINUS)){ //állítsa be a sebességet!
			  SetSpeed(b);
			  USART_Tx(UART0, 0x30+SpeedLvl); //visszaküldi az nehézségi fokozat értékét (0x30 a 0 ASCII kódja, ez is egy megoldás)
		  }
}
}

void UART0InGame(){ //játék közben ez a függvény olvassa az inputot
	key b;
		  if(rx_flag){ //ha nem történt megszakítás akkor ne csináljon semmit
			  rx_flag = false; //egyébként pedig csak balra vagy jobbra mozoghat a kosár, ilyenkor nem lehet állítani a nehézégi szintet
			  b = InputHandler(rx_data);
			  BasketMove(b);
		  }
	  }

