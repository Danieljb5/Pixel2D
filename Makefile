# makefile - use to build projects
OBJECTS = client.o
SRC = src
INCLUDE = include
COPYFOLDERS = assets
BUILDDIR = build
BUILDFILE = main.x86_64
PARAMS = -I . -I asio-1.18.2/include -I $(SRC) -I $(INCLUDE) -I /usr/include/SFML -lsfml-graphics -lsfml-window -lsfml-system -ldl -lpthread -g -Wall -Wextra -Werror -pedantic -std=c++17 -Wno-error=overflow -Wno-overflow

run: $(BUILDDIR)/$(BUILDFILE)
	./$(BUILDDIR)/$(BUILDFILE)

$(BUILDDIR)/$(BUILDFILE): $(OBJECTS)
	mkdir -p $(BUILDDIR)
	g++ -O $(OBJECTS) -o $(BUILDDIR)/$(BUILDFILE) $(PARAMS)
	cp $(COPYFOLDERS) $(BUILDDIR) -r
	rm *.o
	
%.o: $(SRC)/%.cpp
	g++ $(PARAMS) -o $@ -c $<
	
.PHONY: clean
clean:
	rm build -r
