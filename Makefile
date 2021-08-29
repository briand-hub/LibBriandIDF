# Variables to control Makefile operation

SRCPATH = src/
INCLUDEPATH = include/
OUTNAME = main_linux_exe

CC = g++
CFLAGS = -g -pthread -lmbedtls -lmbedcrypto -lmbedx509 -lsodium -std=gnu++17


main:
	$(CC) -o $(OUTNAME) $(SRCPATH)*.cpp $(CFLAGS) -I$(INCLUDEPATH)
