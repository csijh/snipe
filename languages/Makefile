scan: scan.c
	gcc -std=c11 -Wall -g -fsanitize=undefined -fsanitize=address scan.c style.c -o scan

w: scan.c
	gcc -std=c11 -Wall -g scan.c style.c -o scan
