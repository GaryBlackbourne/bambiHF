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

//a program �ltal haszn�l v�ltoz�k
volatile _Bool StartGame = false; //ezzel ellen�rizz�k le, hogy elkezd�d�tt-e a j�t�k
volatile uint8_t BananaCounter; //ez a ban�nokat sz�molja
const uint8_t MaxNumOfBanana = 25; //konstans be�ll�t�sa a ban�nok sz�m�nak korl�toz�s�ra
volatile int FallingTime = 450; //ez lesz a ban�nok es�s�nek az ideje, ez az alap�rtelmezett 450 ms-os be�ll�t�s
volatile uint8_t SpeedLvl = 1; //a neh�zs�gi szint alap�rtelmezett 1-es (a legk�nnyebb szint), ezt k�ldi vissza az UART a sz�m�t�g�pnek
volatile uint8_t BasketState; //az a v�ltoz� t�rolja a kos�r �llapot�t
volatile uint8_t rx_data; //ide v�rjuk az UART-b�l �rkez� adatokat
volatile _Bool rx_flag = false; //sz�ks�g�nk van egy flagre amivel jelezni tudjuk azt, hogy az rx-en adat �rkezett
volatile uint32_t msTicks; //ms sz�ml�l�

//ezek a typedefek a tant�rgy honlapj�n tal�lt segmentlcd_individual projektb�l vannak, ezek sz�ks�gesek a j�t�k m�k�d�s�hez
SegmentLCD_UpperCharSegments_TypeDef upperCharSegments[SEGMENT_LCD_NUM_OF_UPPER_CHARS];
SegmentLCD_LowerCharSegments_TypeDef lowerCharSegments[SEGMENT_LCD_NUM_OF_LOWER_CHARS];

void SysTick_Handler(void)
{
  msTicks++;       //SysTick megszak�t�s kezel� f�ggv�ny, ms-onk�nt n�veli 1-el msTicks �rt�k�t
}

void DelayInGame(uint32_t dlyTicks) //a j�t�k k�zben sz�ks�g van input kezel�sre, ez�rt k�l�n k�sleltet� f�ggv�nyt kellett ehhez l�trehozni
{
  uint32_t curTicks;
  uint8_t i = 1;
  curTicks = msTicks; //az �ppen adott id�pillantban elkapott msTicks �rt�k�t el kell menteni ahhoz, hogy id�t tudjunk sz�molni
  while ((msTicks - curTicks) < dlyTicks) { //dlyTicks �rt�knek megfelel� ms id�t v�runk (pl. dlyTicks = 500; akkor f�l m�sodperc v�rakoz�s)
	  if((msTicks - curTicks) > 20*i){ //20 ms-onk�nt lehallgatjuk az inputot
		  i++; UART0InGame();			//ez az �rt�k megfelel�, j�t�k k�zben szinte �szrevehetetlen a k�sletet�s
	  }
  }
}

void Delay(uint32_t dlyTicks) //ez a Delay f�ggv�ny ugyan�gy m�k�dik mint az j�t�kk�zbeni k�sleltet� f�ggv�ny, de itt nem hallgatjuk le az inputot
{
  uint32_t curTicks;
  curTicks = msTicks;
  while ((msTicks - curTicks) < dlyTicks) { }
  }



void UART0_RX_IRQHandler(void){ //UART0 megszak�t�skezel� f�ggv�ny
	rx_flag = true;			//rx_flaget be�ll�tjuk, hogy tudjunk arr�l, ha megszak�t�s t�rt�nt, vagyis adat �rkezett az UART0-ba
	rx_data = USART_Rx(UART0); //ezt az adatot kiolvassuk �s elmentjuk rx_data-ba
	USART_IntClear(UART0, _USART_IF_MASK); //t�rl�nk minden interrupt flaget
}

void SetSpeed(key k){ //neh�zs�gi szintet �ll�tja be, att�l f�gg�en, hogy + vagy - karakter �rkezett
	if(k == PLUS){
		SpeedUp();
	}
	else if(k == MINUS){
		SlowDown();
	}
	else
		return;
}

void SpeedUp(){ //ezt h�vja meg a SetSpeed f�ggv�ny, ha + karakter �rkezett, ilyenkor a neh�zs�gi szintet emelj�k
	if(SpeedLvl < 4) {
		SpeedLvl++; //n�h�zs�gi szint emel�se egy fokozattal
		FallingTime -= 80; //kevesebb lesz a FallingTime, vagyis gyorsabban fog leesni a ban�n
		SegmentLCD_ARing(SpeedLvl, 1); //jelezz�k a 8 szegmenses gy�r�n is, vagyis plusz egy szegmenst felkapcsolunk
		SegmentLCD_Write("LEVEL +"); // az alfanumerikus kijelz�n is ki�rjuk
	}
}

void SlowDown(){ //ezt h�vja meg a SetSpeed f�ggv�ny, ha - karakter �rkezett, ilyenkor a neh�zs�gi szintet cs�kkentj�k
	if(SpeedLvl > 1){
		SegmentLCD_ARing(SpeedLvl, 0); //az aktu�lis szintet jelz� szegmenst kikapcsoljuk
		SpeedLvl--; //szintet cs�kkent�nk
		FallingTime += 80; //80 ms-mal t�bb lesz az es�si id�, vagyis lassabban fog esni a ban�n
		SegmentLCD_Write("LEVEL -"); //jelezz�k az alfanumerikus kijelz�n is
	}
}

void GameStarted(){ //ez a f�ggv�ny a j�t�k kezdeti �llapot�t �ll�tja be
	StartGame = true; //bebillenti a StartGame flaget
	BananaCounter = 0; //null�zza azt amit csak sz�ks�ges
	BasketState = 0;
	SegmentLCD_Number(0);
	SegmentLCD_Symbol(LCD_SYMBOL_COL10, 1);
	lowerCharSegments[BasketState].d = 1; //kos�r alaphelyzetben bal sz�len van
	SegmentLCD_LowerSegments(lowerCharSegments);
	SegmentLCD_Write("READY"); //felk�sz�ti a j�t�kost, hogy nemsok�ra indul a j�t�k
	Delay(600);
	SegmentLCD_Write("START");
	Delay(400);
	StartGame=Gaming(); //amint megh�vja Gaming f�ggv�nyt, elkezd�dik a j�t�k
}

void BasketMoveLeft(){ //a kos�r balra mozg�s�t val��tja meg
	uint8_t recent = BasketState; //a jelenlegi kos�r �llapot�t elmentj�k, hogy ki tudjuk kapcsolni a szegmenst
	if(BasketState == 0){ //ha m�r nem tudunk balra menni a kos�rral, akkor jobb oldalr�l j�jj�n vissza a kos�r
			BasketState = 3;
		}
		else { //egy�bk�nt l�pjen egyet balra a kos�r
			BasketState -= 1;
		}
	lowerCharSegments[recent].d = 0; //kikapcsoluk az el�z� �llapotot
	SegmentLCD_LowerSegments(lowerCharSegments);
	lowerCharSegments[BasketState].d = 1; //�s bekapcsoljuk a be�ll�tott �llapotnak megfelel� als� szegmenst
	SegmentLCD_LowerSegments(lowerCharSegments);
}

void BasketMoveRight(){ //ez a f�ggv�ny ugyanazo az elven m�k�dik, mint a BasketMoveLeft, csak ez jobbra viszi a kosarat
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

void BasketMove(key Move){ //ez a f�ggv�ny kapja meg a felsorol�s t�pus� v�ltoz�t, melynek megfelel�en megh�vja a sz�ks�ges f�ggv�nyt
if(Move == LEFT){
	BasketMoveLeft();
}
if(Move == RIGHT){
	BasketMoveRight();
}
}

_Bool Gaming(){ //ez a f�ggv�ny val�s�tja meg mag�t a j�t�kot
	int i = 0; //sz�ks�g van k�t seg�dv�ltoz�ra
	int j = 0;
	while(BananaCounter != MaxNumOfBanana) {
	uint8_t num = rand()%4; //random sz�m gener�l�sa �s 4-gyel val� modul�l�sa, hogy 0,1,2 vagy 3 �rt�ket kapjunk, hiszen csak 4 mez�n j�tszunk
    lowerCharSegments[num].a = 1; //a num �rt�knek megfelel� mez�ben felvillan a fels� szegmens
    SegmentLCD_LowerSegments(lowerCharSegments); // a tov�bbiakban ebben a szegmensben FallingTime id�k�z�nk�nt fokozatosan leesik a ban�n
    UART0InGame(); //itt is lehallgatjuk az UART-ot r�ad�sk�nt
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

	if(BasketState==num){ //ha a kos�r helyzete megegyezik a ban�n helyzet�vel, akkor elkaptuk a ban�nt
		i++;
		SegmentLCD_Number(((i*100)+j)); //az elkapott ban�nokat pedig a jobb fels� sarokban sz�moljuk, a kett�spont bal oldal�n
	}
	else {
		j++;
		SegmentLCD_Number(((i*100)+j)); // a leesett ban�nokat is sz�moljuk, a kett�spontt�l jobbra
	}
    lowerCharSegments[num].p = 0;
    SegmentLCD_LowerSegments(lowerCharSegments);
    BananaCounter++;
	}
	lowerCharSegments[BasketState].d = 0; //kikapcsoljuk a kosarat, hogyha k�vetkez� j�t�kot ind�tunk, akkor ne vil�g�tson feleslegesen plusz egy szegmens
	SegmentLCD_LowerSegments(lowerCharSegments);
	SegmentLCD_Write("ENDGAME");
    return false; //false �rt�kkel t�r�nk vissza, ez az�rt lesz fontos, mert a StartGame flaget ez a visszat�r�si �rt�k fogja null�zni
}


key InputHandler(uint8_t c) { //input kezel� f�ggv�ny, a karaktereknek megfelel�en t�r vissza
	switch(c){
	case 's': return START;
	case '+': return PLUS;
	case '-': return MINUS;
	case 'b': return LEFT;
	case 'j': return RIGHT;
	default:  return NOTHING;
	}
}


void ScrollText(char *scrolltext) //ez a f�ggv�ny haszn�laton k�v�l van!! tal�n nem is �rdemes haszn�lni
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

void UART0Begin(){ //amikor m�g nem indult el a j�t�k akkor, ez a f�ggv�ny v�rja az UART0-t�l �rkez� adatokat
	key b;
	  if(rx_flag){ //ha rx_flat==false, akkor ne csin�ljon semmit
		rx_flag = false; //egy�bk�nt kezelj�k le az inputot
		b = InputHandler(rx_data);
		  if(b == START){ //kezdje el a j�t�kot!
			  if(!StartGame){
			  GameStarted();
			  }
		  }
		  if((b == PLUS) || (b == MINUS)){ //�ll�tsa be a sebess�get!
			  SetSpeed(b);
			  USART_Tx(UART0, 0x30+SpeedLvl); //visszak�ldi az neh�zs�gi fokozat �rt�k�t (0x30 a 0 ASCII k�dja, ez is egy megold�s)
		  }
}
}

void UART0InGame(){ //j�t�k k�zben ez a f�ggv�ny olvassa az inputot
	key b;
		  if(rx_flag){ //ha nem t�rt�nt megszak�t�s akkor ne csin�ljon semmit
			  rx_flag = false; //egy�bk�nt pedig csak balra vagy jobbra mozoghat a kos�r, ilyenkor nem lehet �ll�tani a neh�z�gi szintet
			  b = InputHandler(rx_data);
			  BasketMove(b);
		  }
	  }

