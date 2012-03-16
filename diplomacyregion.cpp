// diplomacyregion.cpp
// Class methods for DiplomacyGame

// Andrew MacKie-Mason
// CMSC 162, University of Chicago

// Project site: http://brick.cs.uchicago.edu/Courses/CMSC-16200/2012/pmwiki/pmwiki.php/Student/Diplomacy

#include <cstring>
#include <string>

#include "diplomacy.h"

//
// class constructors and setup
//

DiplomacyRegion::DiplomacyRegion(char *name, DiplomacyGame *gamep) {
    clear_region();
    names.push_back(strdup(name));
    game = gamep;
}
DiplomacyRegion::DiplomacyRegion() {
    clear_region();
}

DiplomacyRegion::DiplomacyRegion(const DiplomacyRegion& old, DiplomacyGame *newgame) {
    game = newgame;
    names.resize(old.names.size(), NULL);
    for (int i = 0; i < names.size(); ++i) {
        names[i] = strdup(old.names[i]);
    }

    occupiers.resize(old.occupiers.size(), NULL);
    for (int i = 0; i < occupiers.size(); ++i) {
        occupiers[i] = game->find_copied_piece(old.occupiers[i]);
    }

    coast = old.coast;
    // Need a later function to fill in values for these:
    coasts.resize(old.coasts.size(), NULL);
    parent = NULL;
    adj_regions.resize(old.adj_regions.size(), NULL);

    occupier_defending = old.occupier_defending;
    defending_support.resize(old.defending_support.size(), NULL);
    for (int i = 0; i < defending_support.size(); ++i) {
        support *cpy_supp = new support;
        cpy_supp->supporter = game->find_copied_piece(old.defending_support[i]->supporter);
        cpy_supp->supported = game->find_copied_piece(old.defending_support[i]->supported);
        defending_support[i] = cpy_supp;
    }

    attacking_pieces.resize(old.attacking_pieces.size(), NULL);
    for (int i = 0; i < attacking_pieces.size(); ++i) {
        attacking_pieces[i] = game->find_copied_piece(old.attacking_pieces[i]);
    }

    attacking_support.resize(old.attacking_support.size(), NULL);
    for (int i = 0; i < attacking_support.size(); ++i) {
        support *cpy_supp = new support;
        cpy_supp->supporter = game->find_copied_piece(old.attacking_support[i]->supporter);
        cpy_supp->supported = game->find_copied_piece(old.attacking_support[i]->supported);
        attacking_support[i] = cpy_supp;
    }

    attacking_convoys.resize(old.attacking_convoys.size(), NULL);
    for (int i = 0; i < attacking_convoys.size(); ++i) {
        convoy *cpy_conv = new convoy;
        cpy_conv->convoyer = game->find_copied_piece(old.attacking_convoys[i]->convoyer);
        cpy_conv->convoyed = game->find_copied_piece(old.attacking_convoys[i]->convoyed);
        attacking_convoys[i] = cpy_conv;
    }
}

void DiplomacyRegion::copy_region_pointers(const DiplomacyRegion& old) {
    for (int i = 0; i < coasts.size(); ++i) {
        char *regname = old.coasts[i]->check_names()[0];
        DiplomacyRegion *region = game->get_region_by_name(regname);
        coasts[i] = region;
    }
    if (old.parent != NULL) {
        char *parname = old.parent->check_names()[0];
        DiplomacyRegion *par = game->get_region_by_name(parname);
        parent = par;
    }
    for (int i = 0; i < adj_regions.size(); ++i) {
        char *regname = old.adj_regions[i]->check_names()[0];
        DiplomacyRegion *region = game->get_region_by_name(regname);
        adj_regions[i] = region;
    }
}

DiplomacyRegion::~DiplomacyRegion() {
    clear_region();
}

// sets everything back to defaults
void DiplomacyRegion::clear_region() {
    while (!names.empty()) {
        free((void *) names.back());
        names.pop_back();
    }
    occupiers.clear();
    coast = false;
    coasts.clear();
    parent = NULL;
    adj_regions.clear();

    clear_move_records();
}

void DiplomacyRegion::clear_move_records() {
    clear_attacker_records();
    while (!attacking_convoys.empty()) {
        delete attacking_convoys.back();
        attacking_convoys.pop_back();
    }
    while (!attacking_support.empty()) {
        delete attacking_support.back();
        attacking_support.pop_back();
    }
    while (!defending_support.empty()) {
        delete defending_support.back();
        defending_support.pop_back();
    }
}

void DiplomacyRegion::clear_attacker_records() {
    attacking_pieces.clear();
}

//
// game setup functions
//

// if the given abbrv isn't already a name for the region, it adds it
// to the vector of names.
void DiplomacyRegion::add_abbrv(char *abbrv) {
    for (int i = 0; i < names.size(); ++i) {
        if (strcmp(abbrv,names[i]) == 0) {
            return;
        }
    }
    names.push_back(strdup(abbrv));
}

// if the given region isn't already adjacent to the current one, adds
// an adjacency
void DiplomacyRegion::add_adj(DiplomacyRegion *adj) {
    for (int i = 0; i < adj_regions.size(); ++i) {
        if (adj_regions[i] == adj) {
            return;
        }
    }
    adj_regions.push_back(adj);
}

// If the region doesn't already have a parent, adds the indicated one.
// Otherwise returns -1.
int DiplomacyRegion::add_parent(DiplomacyRegion *parentp) {
    if (parent != NULL) {
        return -1;
    }
    parent = parentp;
    coast = true;
    return 0;
}

// If the given region isn't already a cost of the current one, adds it
void DiplomacyRegion::add_coast(DiplomacyRegion *child) {
    for (int i = 0; i < coasts.size(); ++i) {
        if (coasts[i] == child) {
            return;
        }
    }
    coasts.push_back(child);
}

// Adds the piece as an occupier
void DiplomacyRegion::add_piece(DiplomacyPiece *newpiece) {
    for (int i = 0; i < occupiers.size(); ++i) {
        if (occupiers[i] == newpiece)
            return;
    }
    occupiers.push_back(newpiece);
}

//
// move recording functions
//

// all assume that supporter hasn't been issued another move yet

void DiplomacyRegion::set_occupier_defending(bool status) {
    occupier_defending = status;
    if (parent != NULL)
        parent->set_occupier_defending(status);
    for (int i = 0; i < coasts.size(); ++i) {
        coasts[i]->occupier_defending = status;
    }
}

void DiplomacyRegion::set_hold() {
    set_occupier_defending(true);
    occupiers[0]->set_move_type(0);
    occupiers[0]->set_move_target(this);
}

void DiplomacyRegion::add_def_supp(DiplomacyPiece *supporter, DiplomacyPiece *supported) {
    DiplomacyRegion *target = this;
    if (parent != NULL) {
        target = parent;
    }
    support *supp_struct = new support;
    supp_struct->supporter = supporter;
    supp_struct->supported = supported;
    target->defending_support.push_back(supp_struct);
    supporter->set_move_type(1);
    supporter->set_move_target(this);
}

void DiplomacyRegion::add_attacker(DiplomacyPiece *piece) {
    DiplomacyRegion *target = this;
    if (parent != NULL) {
        target = parent;
    }
    target->attacking_pieces.push_back(piece);
    piece->set_move_type(2);
    piece->set_move_target(this);
}


void DiplomacyRegion::add_att_supp(DiplomacyPiece *supporter, DiplomacyPiece *supported) {
    DiplomacyRegion *target = this;
    if (parent != NULL) {
        target = parent;
    }
    support *supp_struct = new support;
    supp_struct->supporter = supporter;
    supp_struct->supported = supported;
    target->attacking_support.push_back(supp_struct);
    supporter->set_move_type(3);
    supporter->set_move_target(this);
}

void DiplomacyRegion::add_att_conv(DiplomacyPiece *convoyer, DiplomacyPiece *convoyed) {
    DiplomacyRegion *target = this;
    if (parent != NULL) {
        target = parent;
    }
    convoy *conv_struct = new convoy;
    conv_struct->convoyer = convoyer;
    conv_struct->convoyed = convoyed;
    target->attacking_convoys.push_back(conv_struct);
    convoyer->set_move_type(4);
    convoyer->set_move_target(this);
}

//
// move handling functions
//

void DiplomacyRegion::clear_convoys() {
    while(!attacking_convoys.empty()) {
        delete attacking_convoys.back();
        attacking_convoys.pop_back();
    }
    // This will probably just be used for non-coastal regions, but just in case:
    for (int i = 0; i < coasts.size(); ++i) {
        coasts[i]->clear_convoys();
    }
}

// eliminates all convoys in the region if it's not coastal
// eliminates all attempts to convoy a fleet
// eliminates all convoys whose source isn't coastal
void DiplomacyRegion::cull_convoys() {
    if (coasts.size() == 0) {
        if (attacking_convoys.size() > 0) {
            fprintf(stderr,"%d convoys into non-coastal region %s.\n",attacking_convoys.size(),
                names[0]);
            clear_convoys();
        }
        return;
    }
    for (int i = 0; i < attacking_convoys.size(); ++i) {
        if (attacking_convoys[i]->convoyed->check_type() == 1) {
            fprintf(stderr,"Attempt to convoy fleet in %s into %s.\n",
                attacking_convoys[i]->convoyed->check_location()->check_names()[0],
                names[0]);
            attacking_convoys[i]->convoyer->set_move_type(0);
            attacking_convoys[i]->convoyer->set_move_target(attacking_convoys[i]->convoyer->check_location());
            attacking_convoys[i]->convoyer->check_location()->set_occupier_defending(true);
            delete attacking_convoys[i];
            attacking_convoys.erase(attacking_convoys.begin()+i);
            --i;
        }
    }
    for (int i = 0; i < attacking_convoys.size(); ++i) {
        if (attacking_convoys[i]->convoyed->check_location()->check_coasts().size() == 0) {
            fprintf(stderr,"Attempt to convoy army out of non-coastal region %s into %s.\n",
                attacking_convoys[i]->convoyed->check_location()->check_names()[0],
                names[0]);
            attacking_convoys[i]->convoyer->set_move_type(0);
            attacking_convoys[i]->convoyer->set_move_target(attacking_convoys[i]->convoyer->check_location());
            attacking_convoys[i]->convoyer->check_location()->set_occupier_defending(true);
            delete attacking_convoys[i];
            attacking_convoys.erase(attacking_convoys.begin()+i);
            --i;
        }
    }
}

// eliminates all support comming from a non-adjacent region
// TODO need to check if it's coming from region adjacent to a coast
void DiplomacyRegion::cull_support() {
    for (int i = 0; i < attacking_support.size(); ++i) {
        if (!game->check_if_supp_adj(attacking_support[i]->supporter->check_location(),this)) {
            fprintf(stderr,"Attempt by unit in %s to support attack on non-adjacent region %s.\n",
                attacking_support[i]->supporter->check_location()->check_names()[0],
                names[0]);
            attacking_support[i]->supporter->set_move_type(0);
            attacking_support[i]->supporter->set_move_target(attacking_support[i]->supporter->check_location());
            attacking_support[i]->supporter->check_location()->set_occupier_defending(true);
            delete attacking_support[i];
            attacking_support.erase(attacking_support.begin()+i);
            --i;
        }
    }
    for (int i = 0; i < defending_support.size(); ++i) {
        if (!game->check_if_supp_adj(defending_support[i]->supporter->check_location(),this)) {
            fprintf(stderr,"Attempt by unit in %s to support defense in non-adjacent region %s.\n",
                defending_support[i]->supporter->check_location()->check_names()[0],
                names[0]);
            defending_support[i]->supporter->set_move_type(0);
            defending_support[i]->supporter->set_move_target(defending_support[i]->supporter->check_location());
            defending_support[i]->supporter->check_location()->set_occupier_defending(true);
            delete defending_support[i];
            defending_support.erase(defending_support.begin()+i);
            --i;
        }
    }
}

// eliminates support structs relying on the passed in piece
void DiplomacyRegion::remove_support(DiplomacyPiece *supporter) {
    if (parent != NULL) {
        parent->remove_support(supporter);
        return;
    }
    for (int i = 0; i < attacking_support.size(); ++i) {
        if (attacking_support[i]->supporter == supporter) {
            delete attacking_support[i];
            attacking_support.erase(attacking_support.begin()+i);
            --i;
        }
    }
    for (int i = 0; i < defending_support.size(); ++i) {
        if (defending_support[i]->supporter == supporter) {
            delete defending_support[i];
            defending_support.erase(defending_support.begin()+i);
            --i;
        }
    }
}

// eliminates convoy structs relying on the passed in piece
void DiplomacyRegion::remove_convoy(DiplomacyPiece *convoyer) {
    if (parent != NULL) {
        parent->remove_convoy(convoyer);
        return;
    }
    for (int i = 0; i < attacking_convoys.size(); ++i) {
        if (attacking_convoys[i]->convoyer == convoyer) {
            delete attacking_convoys[i];
            attacking_convoys.erase(attacking_convoys.begin()+i);
            --i;
        }
    }
}

// removes an attacking piece
void DiplomacyRegion::remove_attacker(DiplomacyPiece *attacker) {
    if (parent != NULL) {
        parent->remove_attacker(attacker);
        return;
    }
    for (int i = 0; i < attacking_pieces.size(); ++i) {
        if (attacking_pieces[i] == attacker) {
            attacking_pieces.erase(attacking_pieces.begin()+i);
            --i;
        }
    }
}

// removes an occupying piece
void DiplomacyRegion::remove_piece(DiplomacyPiece *piecep) {
    for (int i = 0; i < occupiers.size(); ++i) {
        if (occupiers[i] == piecep) {
            occupiers.erase(occupiers.begin()+i);
            return;
        }
    }
    fprintf(stderr,"ERROR: Could not find piece to remove in %s.\n",names[0]);
}

//
// access functions
//

// checks if there's a piece in the region
bool DiplomacyRegion::occupied() {
    bool acc = !occupiers.empty();
    for (int i = 0; i < coasts.size(); ++i) {
        acc = acc || coasts[i]->occupied();
    }
    return acc;
}

DiplomacyRegion *DiplomacyRegion::check_parent() {
    return parent;
}

bool DiplomacyRegion::check_coast() {
    return coast;
}

std::vector<DiplomacyRegion *> DiplomacyRegion::check_adj_regions() {
    return adj_regions;
}

std::vector<DiplomacyRegion *> DiplomacyRegion::check_coasts() {
    return coasts;
}

std::vector<char *> DiplomacyRegion::check_names() {
    return names;
}

std::vector<DiplomacyPiece *> DiplomacyRegion::check_occupiers() {
    return occupiers;
}

// gets occupiers of coasts, parent, and current region
std::vector<DiplomacyPiece *> DiplomacyRegion::check_all_occupiers() {
    std::vector<DiplomacyPiece *> all_occupiers = occupiers;
    std::vector<DiplomacyPiece *> par_occupiers;
    if (parent != NULL) {
        par_occupiers = parent->check_occupiers();
        all_occupiers.insert(all_occupiers.end(),par_occupiers.begin(),par_occupiers.end());
    }
    for (int i = 0; i < coasts.size(); ++i) {
        std::vector<DiplomacyPiece *> coastal_occupiers = coasts[i]->check_occupiers();
        all_occupiers.insert(all_occupiers.end(),coastal_occupiers.begin(),coastal_occupiers.end());
    }
    return all_occupiers;
}

DiplomacyPiece *DiplomacyRegion::check_occupier() {
    std::vector<DiplomacyPiece *> all_occupiers = check_all_occupiers();
    if (all_occupiers.size() > 0) {
        return all_occupiers[0];
    }
    else {
        return NULL;
    }
}

std::vector<DiplomacyPiece *> DiplomacyRegion::check_attackers() {
    return attacking_pieces;
}

std::vector<support *> DiplomacyRegion::check_att_support() {
    return attacking_support;
}

std::vector<convoy *> DiplomacyRegion::check_convoys() {
    return attacking_convoys;
}

std::vector<support *> DiplomacyRegion::check_def_support() {
    return defending_support;
}

//
// Command line display functions
//

void DiplomacyRegion::display() {
    if (!occupied() && attacking_pieces.empty() && attacking_support.empty() && attacking_convoys.empty()
                    && defending_support.empty()) {
        return;
    }
    printf("Region: %s (%s)\n",names[0],names[1]);
    if (occupied()) {
        printf("\tThe region itself has %d occupiers.\n",occupiers.size());
        for (int i = 0; i < occupiers.size(); ++i) {
            occupiers[i]->display();
        }
        for (int i = 0; i < coasts.size(); ++i) {
            printf("------<coast>------\n");
            coasts[i]->display();
            printf("------</coast>-----\n");
        }
    }
    if (!attacking_pieces.empty()) {
        printf("\tThe region is under attack by %d pieces.\n",attacking_pieces.size());
        for (int i = 0; i < attacking_pieces.size(); ++i) {
            attacking_pieces[i]->display();
        }
    }
    if (!attacking_support.empty()) {
        printf("\tAttackers are being supported by %d pieces.\n",attacking_support.size());
        for (int i = 0; i < attacking_support.size(); ++i) {
            printf("\tThe piece: ");
            attacking_support[i]->supporter->display();
            printf("\tIs supporting: ");
            attacking_support[i]->supported->display();
        }
    }
}