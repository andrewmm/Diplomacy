// diplomacy.cpp

// Andrew MacKie-Mason
// CMSC 162, University of Chicago

// Project site: http://brick.cs.uchicago.edu/Courses/CMSC-16200/2012/pmwiki/pmwiki.php/Student/Diplomacy

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "diplomacy.h"

void parse_order_file(DiplomacyGame *game, char *filename) {
	bison_box = game;
	FILE *filep = fopen(filename,"r");
	assert(filep != NULL);
	yyin = filep;
	yyparse();
	bison_box = NULL;
}

int main(int argc, char **argv) {
	assert(argc >= 2);
	DiplomacyGame *current = new DiplomacyGame((char *) argv[1]);
	DiplomacyGame *root = current;
	current->display();
	printf("PROCESSING ORDER FILES....\n");
	for (int i = 2; i < argc; ++i) {
		parse_order_file(current,argv[i]);
	}
	printf("PRELIMINARY RESOLUTION....\n");
	while (true) {
		current->resolve();
		DiplomacyGame *next = current->pass();
		if (next == NULL) {
			break;
		}
		printf("BRANCH DID NOT PASS. BACKTRACKING.\n\n");
		DiplomacyGame *temp = current;
		current = next;
	}
	printf("DONE WITH RESOLUTION.\n\n");
	current->display();
	return 0;
}