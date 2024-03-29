CC=g++
CFLAGS=
LEX=flex

GHEAD=diplomacy.h

BISONSOURCE=diplomacygame_filein_bison.y
BISONHEADS=$(GHEAD)
BISONCSOURCE=$(BISONSOURCE:.y=.tab.c)
BISONPRODHEAD=$(BISONCSOURCE:.c=.h)
BISONOBJ=$(BISONCSOURCE:.c=.o)

FLEXSOURCE=diplomacygame_filein_lex.l
FLEXCSOURCE=$(FLEXSOURCE:.l=.c)
FLEXHEADS=$(GHEAD) $(BISONPRODHEAD)
FLEXOBJ=$(FLEXSOURCE:.l=.o)

MAINSOURCE=diplomacy.cpp
MAINHEADS=$(GHEAD)
MAINOBJ=$(MAINSOURCE:.cpp=.o)

SOURCE1=diplomacyplayer.cpp
HEADS1=$(GHEAD)
OBJ1=$(SOURCE1:.cpp=.o)

SOURCE2=diplomacygame.cpp
HEADS2=$(GHEAD) $(BISONPRODHEAD)
OBJ2=$(SOURCE2:.cpp=.o)

SOURCE3=diplomacyregion.cpp
HEADS3=$(GHEAD)
OBJ3=$(SOURCE3:.cpp=.o)

SOURCE4=diplomacypiece.cpp
HEADS4=$(GHEAD)
OBJ4=$(SOURCE4:.cpp=.o)

SOURCE5=diplomacyconditions.cpp
HEADS5=$(GHEAD)
OBJ5=$(SOURCE5:.cpp=.o)

OBJECTS=$(MAINOBJ) $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) $(OBJ5) $(FLEXOBJ) $(BISONOBJ)
EXEC=diplomacy

all: $(EXEC)

$(EXEC): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXEC)

$(MAINOBJ): $(MAINSOURCE) $(MAINHEADS)
	$(CC) $(MAINSOURCE) -c -o $(MAINOBJ)
$(OBJ1): $(SOURCE1) $(HEADS1)
	$(CC) $(SOURCE1) -c -o $(OBJ1)
$(OBJ2): $(SOURCE2) $(HEADS2)
	$(CC) $(SOURCE2) -c -o $(OBJ2)
$(OBJ3): $(SOURCE3) $(HEADS3)
	$(CC) $(SOURCE3) -c -o $(OBJ3)
$(OBJ4): $(SOURCE4) $(HEADS4)
	$(CC) $(SOURCE4) -c -o $(OBJ4)
$(OBJ5): $(SOURCE5) $(HEADS5)
	$(CC) $(SOURCE5) -c -o $(OBJ5)

$(FLEXCSOURCE): $(FLEXSOURCE)
$(FLEXOBJ): $(FLEXCSOURCE) $(FLEXHEADS)

$(BISONCSOURCE) $(BISONPRODHEAD): $(BISONSOURCE) $(BISONHEADS)
	bison -d $(BISONSOURCE)
$(BISONOBJ): $(BISONCSOURCE) $(BISONPRODHEAD)

clean:
	rm -rf $(OBJECTS) $(EXEC) $(BISONPRODHEAD) $(BISONCSOURCE) $(FLEXCSOURCE)
