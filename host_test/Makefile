CXX ?= g++
SRC = ../source/data_gen.cpp ../source/data_static.cpp ../source/i18n.cpp ../source/gfx.cpp ../source/sim.cpp ../source/sim_enemy.cpp \
      ../source/sim_draw.cpp ../source/audio.cpp ../source/save.cpp ../source/game.cpp platform_null.cpp test_main.cpp
FLAGS = -std=c++17 -O2 -Wall -Wextra -fno-exceptions -fno-rtti -I../source
run: soak
	./soak
soak: $(SRC) $(wildcard ../source/*.h)
	$(CXX) $(FLAGS) $(SRC) -o soak -lm
asan: $(SRC)
	$(CXX) $(FLAGS) -g -fsanitize=address,undefined $(SRC) -o soak_asan -lm && ./soak_asan
syntax3ds:
	$(CXX) -std=c++17 -fsyntax-only -Wall -Wextra -Istubs -I../source ../source/platform_3ds.cpp ../source/main.cpp
clean:
	rm -f soak soak_asan save.bin save.bin.tmp
