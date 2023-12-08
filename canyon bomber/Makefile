ALLEGRO_VERSION=5.0.10
MINGW_VERSION=4.7.0
FOLDER="C:\Users\Pichau\Downloads\canyon-bomber"

FOLDER_NAME=\allegro-$(ALLEGRO_VERSION)-mingw-$(MINGW_VERSION)
PATH_ALLEGRO=$(FOLDER)$(FOLDER_NAME)
LIB_ALLEGRO=\lib\liballegro-$(ALLEGRO_VERSION)-monolith-mt.a
INCLUDE_ALLEGRO=\include

all: canyonbomber.exe

#------------------------------------------------------

canyonbomber.exe: canyonbomber.o
	gcc -o canyonbomber.exe canyonbomber.o $(PATH_ALLEGRO)$(LIB_ALLEGRO)
canyonbomber.o: canyonbomber.c
	gcc -I $(PATH_ALLEGRO)$(INCLUDE_ALLEGRO) -c canyonbomber.c

#------------------------------------------------------

clean:
	del canyonbomber.o 
	del canyonbomber.exe