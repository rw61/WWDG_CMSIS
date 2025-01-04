#include <stm32f4xx.h>
#include <stm32f411xe.h>

 
 int RCC_Init(void);
 void GPIO_Init(void);
 void TIM3_Init(void);
 void TIM2_Init(void);
 void delay_Us(int uS);
 void delay_Ms(int mS);
 void TIM2_IRQHandler(void);
 void TIM3_IRQHandler(void);
 void WWDG_Init(void);


static __IO int StartUpCounter;
static int myTicks = 0;





void TIM3_IRQHandler(void)
   {
		 
		 if(READ_BIT(TIM3->SR, TIM_SR_UIF) )    //проверяем флаг прерывания в Status Reg
		 {
             myTicks++;
		    
				CLEAR_BIT(TIM3->SR, TIM_SR_UIF);				//очистка бита uif статусного регистра для выхода из прерывания после обработки
			} }
		

 int main(void){
	 
	 RCC_Init();
	 GPIO_Init();
	 TIM3_Init();
	 WWDG_Init();
	 __enable_irq();
	 

	 while(1){
		 for(uint16_t n = 0; n < 4; n++){    //цикл на 150 мс
	         GPIOA->ODR ^= GPIO_ODR_ODR_5;
	         delay_Ms(50);
	     }
	         WWDG->CR = 127;               //обновление происходит после 200 мс, тоесть в доспустимом интервале 108-262 мс
	     for(uint16_t n = 0; n < 16; n++){
	         GPIOA->ODR ^= GPIO_ODR_ODR_5;
	         delay_Ms(200);
		 WWDG->CR = 127;               //обновление каждые 200 мс, тоесть в допустимом интервале 108-262 мс 
	     }
	    
	 }
 

 }

int RCC_Init(void){
		
	   RCC->CR |= RCC_CR_HSION;
	   while(READ_BIT(RCC->CR, RCC_CR_HSIRDY) == 0){}
		 
	   CLEAR_BIT(RCC->CR, RCC_CR_HSEBYP);
           SET_BIT(RCC->CR, RCC_CR_HSEON);
//ожидание флага готовности НЕ по-простому, на случай выхода из строя КВАРЦА HSE
//Ждем успешного запуска или окончания тайм-аута
	for(StartUpCounter=0; ; StartUpCounter++)
{
//Если успешно запустилось, то выходим из цикла
if(RCC->CR & (1<<RCC_CR_HSERDY_Pos))
break;
//Если не запустилось, то отключаем все, что включили и возвращаем ошибку
if(StartUpCounter > 0x1000)
{
RCC->CR &= ~RCC_CR_HSEON; //Останавливаем HSE
return 1;
}
}
				 
//ожидание флага готовности по-простому
//while(READ_BIT(RCC->CR, RCC_CR_HSERDY) == 0){}
			 
SET_BIT(RCC->CR, RCC_CR_CSSON); //Включим CSS 
MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_HSE);
MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE, RCC_CFGR_HPRE_DIV1); //AHB Prescaler /1/ НА AHB ТАКАЯ ЖЕ ЧАСТОТА КАК НА ЯДРЕ = 8 MHz
MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, RCC_CFGR_PPRE1_DIV2); //APB1 Prescaler /2  = 4 MHz
MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2, RCC_CFGR_PPRE2_DIV1); //APB2 Prescaler /1 
MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, FLASH_ACR_LATENCY_2WS);   //ВАЖНО! These bits represent the ratio of the CPU clock period to the Flash memory access time. 2wS = 2 wait states
	
RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN | RCC_APB1ENR_WWDGEN;
RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
						
return 0;
}
	
void GPIO_Init(void){
GPIOA->MODER |= GPIO_MODER_MODE5_0;                 //LED
GPIOA->MODER &= ~GPIO_MODER_MODE5_1;
}
	
		
	void TIM3_Init(void)
 {
	
	 WRITE_REG(TIM3->PSC, 79);				//800-ПРИ HSE = 8mHZ; устанавливаем делитель на 1599, фоормула расчета частоты таймера х = F(частота проца, в нашем случае это 16Мгц)/значение . записанное в регистр предделителя TIMx_PSC(в нашем случае это 1599)+1 = 16000000/1600 = 10000 тактов в секунду
	 WRITE_REG(TIM3->ARR, 99);       //чтобы получить время счета таймера в 1 секунду записываем в регистр AUTO RELOAD REG число 10000. 1 секунда получается так: частота счета таймера равна 10000 тактов в секунду
	 TIM3->DIER |= TIM_DIER_UIE;
	 TIM3->CR1 |= TIM_CR1_URS;         //Only counter overflow/underflow generates an update interrupt or DMA request if enabled.
	 TIM3->EGR |= TIM_EGR_UG;          //ВАЖНО! РЕИНИЦИАЛИЗАЦИЯ ТАЙМЕРА И ОБНОВЛЕНИЕ РЕГИСТРОВ. ПОСЛЕ АКТИВАЦИИ ТАМЕРА СЧЁТЧИК НАЧИНАЕТСЧИТАТЬ С НУЛЯ! С ЭТИМ БИТОМ ВСЁ РАБОТАЕТ КОРРЕКТНО ТОЛЬКО В ПАРЕ С БИТОМ CR1_UR
	 NVIC_EnableIRQ (TIM3_IRQn);        //разрешение прерывания для таймера 3 , IRQ - Interrupt request
	 
 }
	
void delay_Ms(int mS)  //милисек
 {
	 TIM3->CR1 |= TIM_CR1_CEN;
	 myTicks = 0;
	 while(myTicks < mS);
	 TIM3->CR1 &= ~TIM_CR1_CEN;
 }
 
  void WWDG_Init(void)
  {
    WWDG->CFR |= WWDG_CFR_WDGTB0 | WWDG_CFR_WDGTB1 | 100;    //делитель частоты = 8, WGTB = 11 , Итого: 8 MHz/4096/8 = 244Hz
	  						     //верхняя граница окна , то значение, достигнув которого , можно обновлять сторожевой таймер, чтобы он начинал                                                     //считать сначала сверху вниз. так, как один тик таймера будет равен 1/(8МГц/4096/8)=0,00409 c = 4 мс.
                                                            //то время с начала таймера до возможности начать обновление = (127-100)*4мс = 108мс
    WWDG->CR |= WWDG_CR_WDGA | 127;                         //нижняя граница ,до которой возможно обновление - это 63,по времени от старта -это (127-63)*0,004096 = 262 мс
    
  }
