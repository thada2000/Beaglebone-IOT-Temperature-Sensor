all:
	gcc -lmraa -lcrypto -lssl -lm -o lab4c_tls lab4c_tls.c -g -Wall -Wextra
	gcc -lmraa -lm -o lab4c_tcp lab4c_tcp.c -g -Wall -Wextra

dist:
	tar -czvf lab4c-304925920.tar.gz lab4c_tls.c lab4c_tcp.c Makefile README

clean:
	rm -f lab4c-304925920.tar.gz lab4c_tcp lab4c_tls
	
