all: ringmaster player

ringmaster: ringmaster.cpp role.cpp role.h potato.h
	g++ -g -o ringmaster ringmaster.cpp role.cpp 
player: player.cpp role.cpp role.h potato.h
	g++ -g -o player player.cpp role.cpp 

.PHONY:
	clean
clean:
	rm -rf *.o ringmaster player
