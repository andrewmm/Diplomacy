// diplomacyplayer.cpp
// Class methods for DiplomacyPlayer

// Andrew MacKie-Mason
// CMSC 162, University of Chicago

// Project site: http://brick.cs.uchicago.edu/Courses/CMSC-16200/2012/pmwiki/pmwiki.php/Student/Diplomacy

#include <stdlib.h>
#include <cstring>

#include "diplomacy.h"

//
// class constructors
//

DiplomacyPlayer::DiplomacyPlayer(char *newname, char *email) {
	name=strdup(newname);
	email_address=strdup(email);
}

DiplomacyPlayer::DiplomacyPlayer() {
	name = NULL;
	email_address = NULL;
	pieces.clear();
}

DiplomacyPlayer::DiplomacyPlayer(const DiplomacyPlayer& old, DiplomacyGame *newgame) {
	name = strdup(old.name);
	email_address = strdup(old.email_address);
	pieces.resize(old.pieces.size(), NULL);
	for (int i = 0; i < pieces.size(); ++i) {
		DiplomacyPiece *piece = new DiplomacyPiece(*(old.pieces[i]),this,newgame);
		pieces[i] = piece;
	}
	for (int i = 0; i < pieces.size(); ++i) {
		pieces[i]->copy_region_pointers(*(old.pieces[i]));
	}
}

DiplomacyPlayer::~DiplomacyPlayer() {
	while(!pieces.empty()) {
		delete pieces.back();
		pieces.pop_back();
	}
}

//
// game setup
//

void DiplomacyPlayer::add_piece(DiplomacyPiece *piece) {
	for (int i = 0; i < pieces.size(); ++i) {
		if (piece == pieces[i]) {
			return;
		}
	}
	pieces.push_back(piece);
}

//
// access functions
//

char *DiplomacyPlayer::check_name() {
	return name;
}
char *DiplomacyPlayer::check_email() {
	return email_address;
}

std::vector<DiplomacyPiece *> DiplomacyPlayer::check_pieces() {
	return pieces;
}

//
// Command line display functions
//

void DiplomacyPlayer::display() {
	//printf("Name: %s\n",name,email_address);
	for (int i = 0; i < pieces.size(); ++i) {
		pieces[i]->display();
	}
}