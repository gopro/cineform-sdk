wavelets : wavelets.o utils.o 
		gcc -o wavelets wavelets.o utils.o -lm

wavelets.o : wavelets.c 
		gcc -c wavelets.c
utils.o : utils.c
		gcc -c utils.c
clean :
		rm wavelets wavelets.o utils.o 
