includeDirectory 	= include/
MCU 				= at90usb1286
outputObj 			= obj/mainObjFile.o
main				= src/*.c
outputHex			= runme.hex
PRINTF_LIB_FLOAT = -Wl,-u,vfprintf -lprintf_flt -lm

make : 	
		@avr-gcc -mmcu=$(MCU) -Os -DF_CPU=16000000UL -std=c99 -I $(includeDirectory) $(main) -o $(outputObj) $(PRINTF_LIB_FLOAT)
		@avr-objcopy -O ihex $(outputObj) $(outputHex)
		@echo '\n\tCompilation successful. \n\tPrepare to upload code and acheive all of your hopes and dreams.'
		
warning:
		@avr-gcc -mmcu=$(MCU) -O3 -DF_CPU=16000000UL -std=c99 -I $(includeDirectory) $(main) -o $(outputObj) $(PRINTF_LIB_FLOAT) -Wall
		@avr-objcopy -O ihex $(outputObj) $(outputHex)
		@echo '\n\tCompilation successful. \n\tPrepare to upload code and acheive all of your hopes and dreams.'
		
clean: 
		@rm -f $(outputHex)
		@rm -f $(outputObj)
		@echo '\n\tYou don''t need those hexes anyway.'