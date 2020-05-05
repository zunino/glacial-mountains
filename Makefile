glacial: glacial.o
	g++ -o $@ $^ -L/usr/lib/x86_64-linux-gnu $$(pkg-config --libs sdl2 SDL2_image)

glacial.o: glacial.cpp
	g++ --std=c++17 -c -o $@ $< $$(pkg-config --cflags sdl2)

clean:
	rm -f *.o
	rm -f glacial
