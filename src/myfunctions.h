/*
 * functions.h
 *
 *  Created on: 2020. nov. 9.
 *      Author: hdavi
 */

#ifndef SRC_MYFUNCTIONS_H_
#define SRC_MYFUNCTIONS_H_

#include <stdint.h>

//felsorolás típust hozunk létre az inputok kezeléséhez
enum key {PLUS, MINUS, START, LEFT, RIGHT, NOTHING};
typedef enum key key;

//függvények fejlécei
void SysTick_Handler(void);
void DelayInGame(uint32_t);
void Delay(uint32_t);
void SpeedUp(void);
void SlowDown(void);
void GameStarted();
void ScrollText(char*);
key InputHandler(uint8_t);
void UART0_RX_IRQHandler(void);
void UART0InGame();
void UART0Begin();
void SetSpeed(key);
void BasketMove(key);
void BasketMoveRight();
void BasketMoveLeft();
_Bool Gaming();


#endif /* SRC_MYFUNCTIONS_H_ */
