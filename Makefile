NAME=moto

proc=main
elib=lib

CC=${CROSS_COMPILE}gcc
STRIP=${CROSS_COMPILE}strip
RM=rm

CFLAG =-std=gnu99 -O0 -Wall -g -D_GNU_SOURCE -D_REENTERANT

INC=${SYSROOT}/usr/include
LIB_DIR=${SYSROOT}/usr/lib

$(NAME): $(elib).o $(proc).o
	$(CC) -o $(NAME) $(elib).o $(proc).o -L$(LIB_DIR) -lpthread -ldl
	$(STRIP) $(NAME)
$(elib).o: $(elib).c
	$(CC) -c $(elib).c $(CFLAG) -I$(INC)
$(proc).o: $(proc).c
	$(CC) -c $(proc).c $(CFLAG) -I$(INC)
clean:
	$(RM) -f *.o $(NAME)

