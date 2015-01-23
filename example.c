/**
 * \file
 *
 * \brief SD/MMC card example with FatFs
 *
 * Copyright (c) 2012 - 2013 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

/**
 * \mainpage SD/MMC/SDIO Card with FatFs Example
 *
 * \section Purpose
 *
 * This example shows how to implement the SD/MMC stack with the FatFS.
 * It will mount the file system and write a file in the card.
 *
 * The example outputs the information through the standard output (stdio).
 * To know the output used on the board, look in the conf_example.h file
 * and connect a terminal to the correct port.
 *
 * While using Xplained Pro evaluation kits, please attach I/O1 Xplained Pro
 * extension board to EXT1.
 *
 * \section Usage
 *
 * -# Build the program and download it into the board.
 * -# On the computer, open and configure a terminal application.
 * Refert to conf_example.h file.
 * -# Start the application.
 * -# In the terminal window, the following text should appear:
 *    \code
 *     -- SD/MMC/SDIO Card Example on FatFs --
 *     -- Compiled: xxx xx xxxx xx:xx:xx --
 *     Please plug an SD, MMC or SDIO card in slot.
 *    \endcode
 */

#include <asf.h>
#include "conf_example.h"
#include <string.h>

#include "conf_board.h"
#include "conf_clock.h"
#include "smc.h"

#define ILI93XX_LCD_CS      1

struct ili93xx_opt_t g_ili93xx_display_opt;

/**
 * \brief Application entry point.
 *
 * \return Unused (ANSI-C compatibility).
 */
int main(void)
{
	char test_file_name[] = "0:sd_mmc_test.txt";
	Ctrl_status status;
	FRESULT res;
	FATFS fs;
	FIL file_object;
/**
	const usart_serial_options_t usart_serial_options = {
		.baudrate   = CONF_TEST_BAUDRATE,
		.charlength = CONF_TEST_CHARLENGTH,
		.paritytype = CONF_TEST_PARITY,
		.stopbits   = CONF_TEST_STOPBITS,
	};
**/
	irq_initialize_vectors();
	cpu_irq_enable();

	sysclk_init();
	board_init();
//	stdio_serial_init(CONF_TEST_USART, &usart_serial_options);

	/* Initialize SD MMC stack */
	sd_mmc_init();
	
	pmc_enable_periph_clk(ID_SMC);

	/** Configure SMC interface for Lcd */
	smc_set_setup_timing(SMC, ILI93XX_LCD_CS, SMC_SETUP_NWE_SETUP(2)
	| SMC_SETUP_NCS_WR_SETUP(2)
	| SMC_SETUP_NRD_SETUP(2)
	| SMC_SETUP_NCS_RD_SETUP(2));
	smc_set_pulse_timing(SMC, ILI93XX_LCD_CS, SMC_PULSE_NWE_PULSE(4)
	| SMC_PULSE_NCS_WR_PULSE(4)
	| SMC_PULSE_NRD_PULSE(10)
	| SMC_PULSE_NCS_RD_PULSE(10));
	smc_set_cycle_timing(SMC, ILI93XX_LCD_CS, SMC_CYCLE_NWE_CYCLE(10)
	| SMC_CYCLE_NRD_CYCLE(22));
	#if ((!defined(SAM4S)) && (!defined(SAM4E)))
	smc_set_mode(SMC, ILI93XX_LCD_CS, SMC_MODE_READ_MODE
	| SMC_MODE_WRITE_MODE
	| SMC_MODE_DBW_8_BIT);
	#else
	smc_set_mode(SMC, ILI93XX_LCD_CS, SMC_MODE_READ_MODE
	| SMC_MODE_WRITE_MODE);
	#endif
	/** Initialize display parameter */
	g_ili93xx_display_opt.ul_width = ILI93XX_LCD_WIDTH;
	g_ili93xx_display_opt.ul_height = ILI93XX_LCD_HEIGHT;
	g_ili93xx_display_opt.foreground_color = COLOR_BLACK;
	g_ili93xx_display_opt.background_color = COLOR_WHITE;

	/** Switch off backlight */
	aat31xx_disable_backlight();

	/** Initialize LCD */
	ili93xx_init(&g_ili93xx_display_opt);

	/** Set backlight level */
	aat31xx_set_backlight(AAT31XX_AVG_BACKLIGHT_LEVEL);
	
	ili93xx_set_foreground_color(COLOR_BLACK);
	ili93xx_draw_filled_rectangle(0, 0, ILI93XX_LCD_WIDTH,
	ILI93XX_LCD_HEIGHT);
	/** Turn on LCD */
	ili93xx_display_on();
	
//	printf("\x0C\n\r-- SD/MMC/SDIO Card Example on FatFs --\n\r");
//	printf("-- Compiled: %s %s --\n\r", __DATE__, __TIME__);
	while (1) {
		ili93xx_set_foreground_color(COLOR_WHITE);
//		printf("Please plug an SD, MMC or SDIO card in slot.\n\r");
		ili93xx_draw_string(5, 5, (const uint8_t *)"CHECKING SD CARD");
		delay_ms(800);
		
		status = sd_mmc_test_unit_ready(0);
		if (CTRL_FAIL == status) {
//			printf("Card install FAIL\n\r");
//			printf("Please unplug and re-plug the card.\n\r");
			ili93xx_draw_string(5, 12, (const uint8_t *) "PLEASE INSERT SD CARD");
			goto main_end_of_test;
		}
		

//		printf("Mount disk (f_mount)...\r\n");
		memset(&fs, 0, sizeof(FATFS));
		res = f_mount(LUN_ID_SD_MMC_0_MEM, &fs);
		if (FR_INVALID_DRIVE == res) {
//			printf("[FAIL] res %d\r\n", res);
			ili93xx_draw_string(5, 12, (const uint8_t *) "SD CARD CORRUPTED"); 
			goto main_end_of_test;
		}
//		printf("[OK]\r\n");

/**		printf("Create a file (f_open)...\r\n");
		test_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
		res = f_open(&file_object,
				(char const *)test_file_name, FA_READ);
//				FA_CREATE_ALWAYS | FA_WRITE);
		if (res != FR_OK) {
//			printf("[FAIL] res %d\r\n", res);
			goto main_end_of_test;
		}
//		printf("[OK]\r\n");
		
		char pixel[9], *ptr;
		uint32_t hex;
		
		ili93xx_display_off();
		
		gpio_set_pin_low(LED0_GPIO);
		for(int y=2; y<=320+1; ++y) {
			for(int x=2; x<=240+1; ++x) {
				if(0 == f_gets(pixel, 9, &file_object))
					break;
				hex = strtoul(pixel, &ptr, 16);
				ili93xx_set_foreground_color(hex);
				ili93xx_draw_pixel(x, y);				
			}
		}
		gpio_set_pin_high(LED0_GPIO);
		
		ili93xx_display_on();
/**
		printf("Write to test file (f_puts)...\r\n");
		if (0 == f_puts("Test SD/MMC stack\n", &file_object)) {
			f_close(&file_object);
			printf("[FAIL]\r\n");
			goto main_end_of_test;
		}
		printf("[OK]\r\n");
**/		
		f_mkdir((const TCHAR *)"New");
		f_mkdir((const TCHAR *)"New/new");
		
		TCHAR path[100];
		memset(path,0,100);
//		f_unlink((const TCHAR *)"New/new");
		
		res = f_getcwd(path, 100);
//		ili93xx_draw_string(5, 20, (const uint8_t *)"First");
		ili93xx_draw_string(5, 30, (const uint8_t *)path);
		
		DIR dj;
		memset(&dj,0,sizeof(dj));
		const TCHAR npath[100] = "0:/New";
		dj.fs = &fs;
		res = f_opendir(&dj, npath);
		if(res == FR_OK)
			ili93xx_draw_string(5, 57, (const uint8_t *)"opendir");
		
		FILINFO f;
		memset(&f,0,sizeof(f));
		if((res = f_readdir(&dj, &f)) != FR_OK);
		ili93xx_draw_string(5, 80, (const uint8_t *)"Second");	
		if(res == FR_OK)	
			ili93xx_draw_string(5,117, (const uint8_t *)f.fname);
		memset(&f,0,sizeof(f));
		if((res = f_readdir(&dj, &f)) != FR_OK);
		if(res == FR_OK)
		ili93xx_draw_string(5,147, (const uint8_t *)f.fname);
		memset(&f,0,sizeof(f));
		if((res = f_readdir(&dj, &f)) != FR_OK);
		if(res == FR_OK)
		ili93xx_draw_string(5,177, (const uint8_t *)f.fname);
//		f_chdir((const TCHAR *)"NEW");
//		f_getcwd(path, 100);
//		f_mkdir((const TCHAR *)"Hello");
//		ili93xx_draw_string(5, 57, (const uint8_t *)path);
//		ili93xx_draw_string(5, 147, (const uint8_t *)"HELLO");
		
//		f_close(&file_object);
//		printf("Test is successful.\n\r");

main_end_of_test:
//		printf("Please unplug the card.\n\r");
		while (CTRL_NO_PRESENT != sd_mmc_check(0)) {
		}
	}
}
