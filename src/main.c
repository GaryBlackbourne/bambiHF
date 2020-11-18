#include <stdint.h>
#include <string.h>
#include "em_device.h"
#include "em_chip.h"
#include <em_gpio.h>
#include <em_cmu.h>
#include <em_usart.h>
#include "segmentlcd.h"
#include "segmentlcd_individual.h"
#include "myfunctions.h"


int main(void)
{
  /* Chip errata */
  CHIP_Init();

  //systick konfigurálása
  if (SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000)) { //ms egységben szeretnénk számolni az időt
	  while (1) ;						//hibakezelés: hiba esetén a SysTick_Config visszatérési értéke 1 lesz és ilyenkor blokkoljuk a folytatást
   }

  //INICIALIZÁLÁS

  // engedélyezzük a perifériának az órajelet
  CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_GPIO;
  //  CMU_ClockEnable(cmuClock_HFPER, true);

  //LCD inicializálás (default)
  SegmentLCD_Init(false);

  //órajelet engedélyezünk az UART0-nak
  CMU_ClockEnable(cmuClock_UART0, true);


  //default értéket választjuk az UART inicializáláshoz
  USART_InitAsync_TypeDef UART0_init = USART_INITASYNC_DEFAULT;

  //UART0 inicializálása
  USART_InitAsync(UART0, &UART0_init);

  //beállítjuk az UART0 lábait; ezek az adatok benne vannak a datasheetben is de a gyakorlaton hasonlóan oldottuk meg
  GPIO_PinModeSet(gpioPortF, 7, gpioModePushPull, 1); //a debugger felé (ki)
  GPIO_PinModeSet(gpioPortE, 0, gpioModePushPull, 1); //ez a tx láb (ki)
  GPIO_PinModeSet(gpioPortE, 1, gpioModeInput, 0); //ez az rx láb (be)

  // "Location 1" kiválasztása
  UART0->ROUTE |= (USART_ROUTE_LOCATION_LOC1 | USART_ROUTE_RXPEN | USART_ROUTE_TXPEN);

  //void USART_IntClear(USART typedef *usart, uint32_t flag)
  //törlünk minden interrupt flaget
  USART_IntClear(UART0, _USART_IF_MASK);

  //void USART_IntEnable(USART typdef *usart, uint32_t flag)
  //beállítjuk az rxdatav interrupt flaget
  USART_IntEnable(UART0, USART_IEN_RXDATAV);

  //töröljük a foglaltság jelző bitet
  NVIC_ClearPendingIRQ(UART0_RX_IRQn);

  //kezdeti beállítások
  //a képernyőn megjelenítjük az alapértelmezett nezéhségi szintet
  SegmentLCD_ARing(1, 1);	  //a kijelző 8 szegmenses gyűrűjén az 1-es szegmenst bakapcsoljuk, ez az alapértelmezett 1-es nehézségi szintet jelenti
  SegmentLCD_Write("BANANA"); //a játék neve kikerül a kijelzőre
  Delay(800);
  SegmentLCD_Write(" GAME ");

  //és engedélyezzük az UART0 megszakítást, mostantól az rx lábon érkező adatokra megszakítást fog generálni
  NVIC_EnableIRQ(UART0_RX_IRQn);

  while (1) {				  //innentől várjuk a megszakításokat

	  UART0Begin();			  //listen típusú függvény, ha a megszakításkezelő függvény az rx_flaget beállítja akkor ez a függvény végrehajtódik
  	  	  	  	  	  	  	  //egyébként pedig nem fog csinálni semmit, és a while ciklusban fogunk várakozni, amíg rx_flag hamis
  }
}
