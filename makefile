myhttpd:main.o utils.o sendutils.o getline.o epollfunc.o
	gcc -o myhttpd main.o utils.o sendutils.o getline.o epollfunc.o
main.o:main.c
	gcc -c main.c
utils.o:utils.c
	gcc -c utils.c
sendutils.o:sendutils.c
	gcc -c sendutils.c
getline.o:getline.c
	gcc -c getline.c
epollfunc.o:epollfunc.c
	gcc -c epollfunc.c