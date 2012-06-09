ll:legacy

legacy: DebugOutput.o VideoDriver.o VideoDriverPalette.o VideoDriverPalette1h.o VideoDriverPalette3h.o VideoDriverPaletteUndocumented.o VideoDriverPalette4h.o VideoDriverPalette5h.o VideoDriverPalette6h.o VideoDriverPaletteComposite.o VideoDriverPalette13h.o configuration.o interrupt.o cpu86.o DiskImage.o utilities.o port.o main.o pic.o pit.o disasm.o
	g++ -Wall -g  DebugOutput.o main.o VideoDriver.o VideoDriverPalette.o VideoDriverPalette1h.o VideoDriverPalette3h.o VideoDriverPaletteUndocumented.o VideoDriverPalette4h.o VideoDriverPalette5h.o VideoDriverPalette6h.o VideoDriverPaletteComposite.o VideoDriverPalette13h.o configuration.o interrupt.o cpu86.o DiskImage.o utilities.o port.o pic.o pit.o disasm.o -o bin/legacy `sdl-config --libs` `pkg-config --cflags sdl` `pkg-config sdl --libs`

pic.o: pic.cpp
	g++ -Wall -g -c pic.cpp

pit.o: pit.cpp
	g++ -Wall -g -c pit.cpp

port.o: port.cpp
	g++ -Wall -g -c port.cpp

DebugOutput.o: DebugOutput.cpp DebugOutput.h
	g++ -Wall -g -c DebugOutput.cpp

VideoDriver.o: VideoDriver.cpp VideoDriver.h
	g++ -Wall   -g -c VideoDriver.cpp

VideoDriverPalette.o: VideoDriverPalette.cpp VideoDriverPalette.h VideoDriver.cpp VideoDriver.h
	g++ -Wall   -g `sdl-config --libs` `pkg-config --cflags sdl` `pkg-config sdl --libs` `sdl-config --cflags` -c -g VideoDriverPalette.cpp

VideoDriverPalette1h.o: VideoDriverPalette1h.cpp VideoDriverPalette1h.h VideoDriverPalette.cpp VideoDriverPalette.h
	g++ -Wall   -g -c VideoDriverPalette1h.cpp

VideoDriverPalette3h.o: VideoDriverPalette3h.cpp VideoDriverPalette3h.h VideoDriverPalette.cpp VideoDriverPalette.h
	g++ -Wall   -g -c VideoDriverPalette3h.cpp

VideoDriverPalette4h.o: VideoDriverPalette4h.cpp VideoDriverPalette4h.h VideoDriverPalette.cpp VideoDriverPalette.h
	g++ -Wall   -g  -c VideoDriverPalette4h.cpp

VideoDriverPalette5h.o: VideoDriverPalette5h.cpp VideoDriverPalette5h.h VideoDriverPalette.cpp VideoDriverPalette.h
	g++ -Wall   -g -c VideoDriverPalette5h.cpp

VideoDriverPalette6h.o: VideoDriverPalette6h.cpp VideoDriverPalette6h.h VideoDriverPalette.cpp VideoDriverPalette.h
	g++ -Wall   -g -c VideoDriverPalette6h.cpp

VideoDriverPaletteComposite.o: VideoDriverPaletteComposite.cpp VideoDriverPaletteComposite.h VideoDriverPalette.cpp VideoDriverPalette.h
	g++ -Wall   -g -c VideoDriverPaletteComposite.cpp

VideoDriverPalette13h.o: VideoDriverPalette13h.cpp VideoDriverPalette13h.h VideoDriverPalette.cpp VideoDriverPalette.h
	g++ -Wall   -g -c VideoDriverPalette13h.cpp

VideoDriverPaletteUndocumented.o: VideoDriverPaletteUndocumented.cpp VideoDriverPaletteUndocumented.h VideoDriverPalette.cpp VideoDriverPalette.h
	g++ -Wall   -g -c VideoDriverPaletteUndocumented.cpp

configuration.o: configuration.cpp configuration.h
	g++ -Wall   -g -c configuration.cpp

interrupt.o: interrupt.cpp interrupt.h
	g++ -Wall   -g -c interrupt.cpp 

cpu86.o: cpu86.cpp cpu86.h
	g++ -Wall   -g -c -fpermissive cpu86.cpp

DiskImage.o: DiskImage.cpp DiskImage.h
	g++ -Wall   -g -c DiskImage.cpp

utilities.o: utilities.cpp utilities.h
	g++ -Wall   -g -c utilities.cpp

disasm.o: disasm.cpp disasm.h cpu86.cpp cpu86.h
	g++ -Wall   -g -c disasm.cpp

main.o: main.cpp main.h
	g++  -Wall `sdl-config --libs` `pkg-config --cflags sdl` `pkg-config sdl --libs`  -g -c `sdl-config --cflags` main.cpp

