// diplomacypiece.cpp
// Class methods for DiplomacyPiece

// Andrew MacKie-Mason
// CMSC 162, University of Chicago

// Project site: http://brick.cs.uchicago.edu/Courses/CMSC-16200/2012/pmwiki/pmwiki.php/Student/Diplomacy

#include "diplomacy.h"

//
// class constructors
//

DiplomacyPiece::DiplomacyPiece(DiplomacyPlayer *playerp, int piece, DiplomacyRegion *locationp, DiplomacyGame *gamep) {
    game = gamep;
    owner = playerp;
    piece_type = piece;
    location = locationp;
    move_type = -1;
    move_target = NULL;
}

DiplomacyPiece::DiplomacyPiece() {
    owner = NULL;
    piece_type = -1;
    location = NULL;
    move_type = -1;
    move_target = NULL;
}

DiplomacyPiece::DiplomacyPiece(const DiplomacyPiece& old, DiplomacyPlayer *own, DiplomacyGame *newgame) {
    game = newgame;
    owner = own;
    location = NULL;
    piece_type = old.piece_type;
    move_type = old.move_type;
    move_target = NULL;
}

void DiplomacyPiece::copy_region_pointers(const DiplomacyPiece& old) {
    if (old.location != NULL) {
        char *locname = old.location->check_names()[0];
        DiplomacyRegion *loc = game->get_region_by_name(locname);
        location = loc;
    }

    if (old.move_target != NULL) {
        char *tarname = old.move_target->check_names()[0];
        DiplomacyRegion *tar = game->get_region_by_name(tarname);
        move_target = tar;
    }
}

DiplomacyPiece::~DiplomacyPiece() {
    clear_move();
}

void DiplomacyPiece::clear_move() {
    move_type = -1;
    move_target = NULL;
}

//
// move recording functions
//

void DiplomacyPiece::set_move_type(int type) {
    move_type = type;
}

void DiplomacyPiece::set_move_target(DiplomacyRegion *target) {
    move_target = target;
}

//
// move resolution functions
//

void DiplomacyPiece::set_location(DiplomacyRegion *loc) {
    location = loc;
}

//
// access functions
//

DiplomacyRegion *DiplomacyPiece::check_location() {
    return location;
}

DiplomacyPlayer *DiplomacyPiece::check_owner() {
    return owner;
}

int DiplomacyPiece::check_self_num() {
    std::vector<DiplomacyPiece *> piece_vec = owner->check_pieces();
    for (int i = 0; i < piece_vec.size(); ++i) {
        if (this == piece_vec[i]) {
            return i;
        }
    }
    return -1;
}

int DiplomacyPiece::check_type() {
    return piece_type;
}

int DiplomacyPiece::check_move_type() {
    return move_type;
}

DiplomacyRegion *DiplomacyPiece::check_move_target() {
    return move_target;
}

//
// Command line display functions
//

void DiplomacyPiece::display() {
    //printf("\t\tPiece number %d of type %d owned by %s.\n",check_self_num()+1,piece_type,owner->check_name());
    printf("%s ",owner->check_name());
    if (piece_type == 0) {
        printf("A ");
    }
    else {
        printf("F ");
    }
    printf("%s\n",location->check_names()[0]);
}