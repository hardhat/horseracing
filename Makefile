ZOS_PATH ?= ../Zeal-8-bit-OS
ZVB_SDK_PATH ?= ../Zeal-VideoBoard-SDK
BIN=bin/horser.bin
OBJ=obj/main.rel obj/game.rel obj/menu.rel obj/dat.rel obj/controller.rel obj/text.rel obj/textaccel.rel obj/race_sim.rel obj/wager_math.rel obj/session_logic.rel
IMG=img/title.zts img/map.zts img/hud.zts img/sprites.zts img/theme.zts img/accent.zts
DAT=bin/horser.dat
CC=sdcc
CFLAGS=-mz80 --std-c2x -c -I $(ZOS_PATH)/kernel_headers/sdcc/include/ -I $(ZVB_SDK_PATH)/include --codeseg TEXT --debug
AS=sdasz80 -o -l -s
OBJCOPY=sdobjcopy
LD=sdldz80
LDFLAGS=-n -y -mjwx -i -b _HEADER=0x4000 -k $(ZOS_PATH)/kernel_headers/sdcc/lib -l z80 $(ZOS_LDFLAGS)
ZOS_LIBS=-k $(ZVB_SDK_PATH)/lib -l zvb_sound -l zvb_gfx
all: init $(BIN) $(DAT)

PHONY: init clean reallyclean test-linux

init:
	@mkdir -p obj
	@mkdir -p bin

$(BIN): $(OBJ)
	$(LD) $(LDFLAGS) -o $(BIN:.bin=.ihx) \
	../Zeal-8-bit-OS/kernel_headers/sdcc/bin/zos_crt0.rel $(OBJ) $(ZOS_LIBS)
	$(OBJCOPY) --input-target=ihex --output-target binary $(BIN:.bin=.ihx) $(BIN)
	cp $(BIN) s

$(DAT): tools/dat_gen.py $(IMG) $(IMG:.zts=.ztp)
	python3 tools/dat_gen.py

obj/%.rel: src/%.c
	$(CC) $(CFLAGS) -o $@ $<

obj/%.rel: src/%.asm
	$(AS) $@ $<

img/%.zts: img/%.gif
	$(ZVB_SDK_PATH)/tools/zeal2gif/gif2zeal.py -l -i $<

img/accent.gif: tools/make_accent_gif.py img/hud.gif
	python3 tools/make_accent_gif.py

src/level1.h: level/level1.tmx level/map.tsx level/title.tsx level/hud.tsx
	python3 tools/tmx_to_h.py level/level1.tmx src/level1.h

test-linux:
	$(MAKE) -C tests/linux ROOT=../..

obj/game.rel: src/game.c src/game.h src/main.h src/race_sim.h
obj/menu.rel: src/menu.c src/menu.h src/game.h src/text.h
obj/text.rel: src/text.c src/text.h src/main.h
obj/main.rel: src/main.c src/game.h src/menu.h src/dat.h
obj/dat.rel: src/dat.c src/dat.h
obj/controller.rel: src/controller.c src/controller.h
obj/race_sim.rel: src/race_sim.c src/race_sim.h
obj/wager_math.rel: src/wager_math.c src/wager_math.h src/game.h
obj/session_logic.rel: src/session_logic.c src/session_logic.h src/game.h src/wager_math.h

clean:
	-rm $(OBJ)
	-rm $(BIN)
	-rm obj/*.lst obj/*.sym obj/*.adb obj/*.asm
	-rm bin/*.ihx bin/*.noi bin/*.map bin/*.cdb
	-$(MAKE) -C tests/linux clean ROOT=../..

reallyclean: clean
	-rm img/*.zts img/*.ztp src/level1.h
