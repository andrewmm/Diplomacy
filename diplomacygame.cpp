// diplomacygame.cpp
// Class methods for DiplomacyGame

// Andrew MacKie-Mason
// CMSC 162, University of Chicago

// Project site: http://brick.cs.uchicago.edu/Courses/CMSC-16200/2012/pmwiki/pmwiki.php/Student/Diplomacy

#include <cstring>

#include "diplomacy.h"
#include "diplomacygame_filein_bison.tab.h"

//
// class constructors/destructors/etc
//

DiplomacyGame::DiplomacyGame(char *filename) {
    clear_game();
    conditions.game = this;
    load_from_file(filename);
}

DiplomacyGame::DiplomacyGame() {
    clear_game();
    conditions.game = this;
}

DiplomacyGame::DiplomacyGame(const DiplomacyGame& old) {
    season = old.season;
    alternate = old.alternate;
    player_list.resize(old.player_list.size(), NULL);
    for (int i = 0; i < player_list.size(); ++i) {
        player_list[i] = new DiplomacyPlayer(*(old.player_list[i]),this);
    }
    req_retreats.resize(old.req_retreats.size(), NULL);
    for (int i = 0; i < req_retreats.size(); ++i) {
        req_retreats[i] = find_copied_piece(old.req_retreats[i]);
    }

    region_list.resize(old.region_list.size(), NULL);
    for (int i = 0; i < region_list.size(); ++i) {
        region_list[i] = new DiplomacyRegion(*(old.region_list[i]),this);
    }
    for (int i = 0; i < region_list.size(); ++i) {
        region_list[i]->copy_region_pointers(*(old.region_list[i]));
    }
    for (int i = 0; i < player_list.size(); ++i) {
        for (int j = 0; j < player_list[i]->check_pieces().size(); ++j) {
            player_list[i]->check_pieces()[j]->copy_region_pointers(*(old.player_list[i]->check_pieces()[j]));
        }
    }
    conditions = AndCondition(old.conditions, this);
    safe_supports.resize(old.safe_supports.size());
    for (int i = 0; i < safe_supports.size(); ++i) {
        safe_supports[i].attacker = find_copied_piece(old.safe_supports[i].attacker);
        safe_supports[i].safe_supporter = find_copied_piece(old.safe_supports[i].safe_supporter);
    }
    dislodgments.resize(old.dislodgments.size(),NULL);
    for (int i = 0; i < dislodgments.size(); ++i) {
        dislodgments[i] = new dislodgment;
        dislodgments[i]->dislodged = find_copied_piece(old.dislodgments[i]->dislodged);
        dislodgments[i]->by = get_region_by_name(old.dislodgments[i]->by->check_names()[0]);
    }
}

void DiplomacyGame::load_from_file(char *filename) {
    FILE *filep = fopen(filename, "r");
    if (filep == NULL) {
        fprintf(stderr, "Cannot open file: %s\n", filename);
        exit(-1);
    }
    bison_box = this;
    yyin = filep;
    yyparse();
    bison_box = NULL;
}

// call clear_game() to handle the work of destroying the pointed-to objects
DiplomacyGame::~DiplomacyGame() {
    clear_game();
}

// restore default season value and get rid of attached players and regions
void DiplomacyGame::clear_game() {
    season = 1;
    while (!player_list.empty()) {
        delete player_list.back();
        player_list.pop_back();
    }
    while (!region_list.empty()) {
        delete region_list.back();
        region_list.pop_back();
    }
    while (!dislodgments.empty()) {
        delete dislodgments.back();
        dislodgments.pop_back();
    }
}

//
// serialization/game setup functions
//

// Checks if a region by the given name already exists. If it doesn't,
// allocate it and add a pointer to the vector.
void DiplomacyGame::add_region(char *name) {
    DiplomacyRegion *newregion = get_region_by_name(name);
    if (newregion != NULL) 
        return;
    newregion = new DiplomacyRegion(name, this);
    region_list.push_back(newregion);
}

// Adds a region for name if it doesn't exist yet. Then gives region
// "name" the abbreviation.
int DiplomacyGame::add_abbrv(char *name, char *abbrv) {
    add_region(name);
    DiplomacyRegion *namereg = get_region_by_name(name);
    DiplomacyRegion *abbrvreg = get_region_by_name(abbrv);
    if (abbrvreg != NULL) {
        if (abbrvreg != namereg) {
            return -1;
        }
    }
    else {
        namereg->add_abbrv(abbrv);
    }
    return 0;
}

// Adds a region for both names if they don't exist yet. So long as the
// names don't point to the same region (return -1) it sets them equal
// to each other.
int DiplomacyGame::add_adj(char *newname, char *oldname) {
    add_region(newname);
    add_region(oldname);
    DiplomacyRegion *newreg = get_region_by_name(newname);
    DiplomacyRegion *oldreg = get_region_by_name(oldname);
    if (newreg == oldreg) {
        return -1;
    }
    newreg->add_adj(oldreg);
    oldreg->add_adj(newreg);
    return 0;
}

// Adds a region for both names if they don't exist yet. So long
// as the names don't point to the same region and the child doesn't 
// already have a parent(return -1) it sets the second as the parent
// of the first, and the first as a coast of the second.
int DiplomacyGame::add_par(char *child, char *parent) {
    add_region(child);
    add_region(parent);
    DiplomacyRegion *childreg = get_region_by_name(child);
    DiplomacyRegion *parentreg = get_region_by_name(parent);
    if (childreg == parentreg) {
        return -1;
    }
    int res = childreg->add_parent(parentreg);
    if (res == -1) {
        return -1;

    }
    parentreg->add_coast(childreg);
    return 0;
}

// Adds a player with the given name and email if the name and email
// are both free (if not, return -1).
int DiplomacyGame::add_player(char *name, char *email) {
    int namenum = get_player_num_by_name(name);
    int emailnum = get_player_num_by_email(email);
    if (namenum != -1 || emailnum != -1) { // at least one already used
        return -1;
    }
    DiplomacyPlayer *newplayer = new DiplomacyPlayer(name, email);
    player_list.push_back(newplayer);
    return 0;
}

// Adds a piece belonging to the given player at the given location.
// Returns -1 on bad player, piece type, or location.
int DiplomacyGame::add_piece(char *player, char piece, char *location) {
    DiplomacyPlayer *playerp = get_player_by_num(get_player_num_by_name(player));
    if (playerp == NULL) {
        fprintf(stderr, "player %s does not exist\n",player);
    }
    DiplomacyRegion *locationp = get_region_by_name(location);
    if (locationp == NULL) {
        fprintf(stderr, "location %s does not exist\n", location);
    }
    if (playerp == NULL || locationp == NULL) {
        return -1;
    }
    DiplomacyPiece *newpiece;
    if (piece == 'A' || piece == 'F') {
        int type = 0;
        if (piece == 'F')
            type = 1;
        newpiece = new DiplomacyPiece(playerp, type, locationp, this);
    }
    else {
        fprintf(stderr,"invalid piece type\n");
        return -1;
    }
    locationp->add_piece(newpiece);
    playerp->add_piece(newpiece);
    return 0;
}

// TODO: prototype
void DiplomacyGame::save_game(char *filename) {
    return;
}

//
// access functions
//

int DiplomacyGame::check_season() {
    return season;
}

DiplomacyPlayer *DiplomacyGame::get_player_by_num(int player) {
    if (player < player_list.size() && player >= 0) {
        return player_list[player];
    }
    else {
        return NULL; // bad access num
    }
}

//
// search functions
//

// returns number of first player in the list with the given name (avoid duplicate names!)
int DiplomacyGame::get_player_num_by_name(char *name) {
    for (int i = 0; i < player_list.size(); ++i) {
        if (strcmp(name, player_list[i]->check_name()) == 0) {
            return i;
        }
    }
    return -1; // no match found
}

// returns number of first player in the list with the given email address (avoid duplicate emails!)
int DiplomacyGame::get_player_num_by_email(char *email) {
    for (int i = 0; i < player_list.size(); ++i) {
        if (strcmp(email, player_list[i]->check_email()) == 0) {
            return i;
        }
    }
    return -1; // no match found
}

// returns pointer to region with given name
DiplomacyRegion *DiplomacyGame::get_region_by_name(char *name) {
    for (int i = 0; i < region_list.size(); ++i) {
        std::vector<char *> names = region_list[i]->check_names();
        for (int j = 0; j < names.size(); ++j) {
            if (strcmp(name, names[j]) == 0) {
                return region_list[i];
            }
        }
    }
    return NULL; // no match found
}

// returns true if the target is adjacent to the source
// (of course, adjacency is a symmetric relationship)
bool DiplomacyGame::check_if_adj(DiplomacyRegion *source, DiplomacyRegion *target) {
    std::vector<DiplomacyRegion *> adjs = source->check_adj_regions();
    for (int i = 0; i < adjs.size(); ++i) {
        if (target == adjs[i]) {
            return true;
        }
    }
    return false;
}

// returns true if reg1 and reg2 are part of the same region (i.e. region or any of its coasts)
bool DiplomacyGame::same_region(DiplomacyRegion *reg1, DiplomacyRegion *reg2) {
    DiplomacyRegion *par1 = reg1->check_parent();
    if (par1 == NULL) {
        par1 = reg1;
    }
    DiplomacyRegion *par2 = reg2->check_parent();
    if (par2 == NULL) {
        par2 = reg2;
    }
    return par1 == par2;
}

// returns true if the target or one of its coasts is adjcaent to the source
bool DiplomacyGame::check_if_supp_adj(DiplomacyRegion *source, DiplomacyRegion *target) {
    bool acc = check_if_adj(source,target);
    std::vector<DiplomacyRegion *> target_coasts = target->check_coasts();
    for (int i = 0; i < target_coasts.size(); ++i) {
        acc = acc || check_if_adj(source,target_coasts[i]);
    }
    return acc;
}

DiplomacyGame *DiplomacyGame::check_alternate() {
    return alternate;
}

DiplomacyPiece *DiplomacyGame::find_copied_piece(DiplomacyPiece *oldpiece) {
    return get_player_by_num(get_player_num_by_name(oldpiece->check_owner()->check_name()))->check_pieces()[oldpiece->check_self_num()];
}

std::vector<dislodgment *> DiplomacyGame::check_dislodgments() {
    return dislodgments;
}

//
// move handling functions
//

// branch off a copy
void DiplomacyGame::branch() {
    DiplomacyGame *branch = new DiplomacyGame(*this);
    alternate = branch;
}

// Eliminate obviously illegal moves and set non-moves to holds
void DiplomacyGame::remove_illegal_moves(const std::vector<DiplomacyRegion *>& iter_regions) {
    for (int i = 0; i < iter_regions.size(); ++i) {
        if (!iter_regions[i]->occupied())
            continue;
        std::vector<DiplomacyPiece *> all_occupiers = iter_regions[i]->check_all_occupiers();
        for (int j = 0; j < all_occupiers.size(); ++j) {
            // Things without an order hold
            if (all_occupiers[j]->check_move_type() == -1) {
                all_occupiers[j]->change_to_hold();
            }

            // Things which are holding must do it to their own location
            // This error shouldn't really arise...
            if (all_occupiers[j]->check_move_type() == 0 &&
                    all_occupiers[j]->check_move_target() !=
                        all_occupiers[j]->check_location()) {
                fprintf(stderr,"ERROR: unit in %s trying to 'hold' in %s.\n",
                    all_occupiers[j]->check_location()->check_names()[0],
                    all_occupiers[j]->check_move_target()->check_names()[0]);
                all_occupiers[j]->set_move_target(all_occupiers[j]->check_location());
                all_occupiers[j]->check_location()->set_occupier_defending(true);
            }

            // Fleets (and non-coastal armies) can't move to non-adjacent regions
            if ((all_occupiers[j]->check_type() == 1 ||
                        (all_occupiers[j]->check_type() == 0 && all_occupiers[j]->check_location()->check_coasts().size() == 0))
                    && all_occupiers[j]->check_move_type() == 2) {
                if (!check_if_adj(all_occupiers[j]->check_location(), all_occupiers[j]->check_move_target())) {
                    fprintf(stderr,"Fleet or non-coastal army in %s tried to attack non-adjacent region %s.\n",
                            all_occupiers[j]->check_location()->check_names()[0],
                            all_occupiers[j]->check_move_target()->check_names()[0]);
                    all_occupiers[j]->check_move_target()->remove_attacker(all_occupiers[j]);
                    all_occupiers[j]->change_to_hold();
                }
            }

            // Armies cannot attack oceans, where oceans are things occupied by fleets that are not coastal
            if (all_occupiers[j]->check_type() == 0 && all_occupiers[j]->check_move_target()->check_occupier() != NULL) {
                if (all_occupiers[j]->check_move_target()->check_occupier()->check_type() == 1 &&
                        all_occupiers[j]->check_move_target()->check_coasts().size() == 0) {
                    fprintf(stderr,"Army in %s tried to attack fleet in aquatic region %s.\n",
                        all_occupiers[j]->check_location()->check_names()[0],
                        all_occupiers[j]->check_move_target()->check_names()[0]);
                    all_occupiers[j]->check_move_target()->remove_attacker(all_occupiers[j]);
                    all_occupiers[j]->change_to_hold();
                }
            }

            // trading places requires a convoy
            if (all_occupiers[j]->check_move_type() != 0 && all_occupiers[j]->check_move_target()->occupied()) {
                DiplomacyPiece *j_potential_trader = all_occupiers[j]->check_move_target()->check_occupier();
                if (j_potential_trader->check_move_type() == 2 && j_potential_trader->check_move_target()->check_occupier() == 
                        all_occupiers[j]) {

                    if (all_occupiers[j]->check_move_type() == 2) {
                        // potential for actual place trade
                        bool j_occ_coastal = all_occupiers[j]->check_location()->check_coasts().size() > 0;
                        bool j_pot_coastal = j_potential_trader->check_location()->check_coasts().size() > 0;

                        // if both aren't coastal, they can't change places
                        if (!(j_occ_coastal && j_pot_coastal)) {
                            fprintf(stderr,"Non-coastal units in %s and %s trying to trade places. BRANCHING.\n",
                                all_occupiers[j]->check_location()->check_names()[0],j_potential_trader->check_location()->check_names()[0]);
                            // branch off a copy
                            branch();

                            // primary: require that they don't succeed in trading places
                            add_no_trade_condition(all_occupiers[j],j_potential_trader);

                            // secondary: both attacks canceled
                            DiplomacyPiece *alt_j_occ_2 = alternate->find_copied_piece(all_occupiers[j]);
                            alt_j_occ_2->check_move_target()->remove_attacker(alt_j_occ_2);
                            alt_j_occ_2->change_to_hold();

                            DiplomacyPiece *alt_j_pot = alternate->find_copied_piece(j_potential_trader);
                            alt_j_pot->check_move_target()->remove_attacker(alt_j_pot);
                            alt_j_pot->change_to_hold();
                        }

                        // otherwise, at least one of them needs a convoy
                        else {
                            fprintf(stderr,"Coastal units in %s and %s trying to trade places. BRANCHING.\n",
                                all_occupiers[j]->check_location()->check_names()[0],j_potential_trader->check_location()->check_names()[0]);
                            
                            branch();

                            // primary: options: no trade, or at least one has a convoy
                            add_no_trade_or_convoy_condition(all_occupiers[j],j_potential_trader);

                            // alternate: both attacks canceled
                            DiplomacyPiece *alt_j_occ = alternate->find_copied_piece(all_occupiers[j]);
                            alt_j_occ->check_move_target()->remove_attacker(alt_j_occ);
                            alt_j_occ->change_to_hold();

                            DiplomacyPiece *alt_j_pot = alternate->find_copied_piece(j_potential_trader);
                            alt_j_pot->check_move_target()->remove_attacker(alt_j_pot);
                            alt_j_pot->change_to_hold();
                        }
                    }
                }
            }
            // Coastal armies moving to non-adjacent regions must be convoyed
            // Branch on existence of a convoy
            if (all_occupiers[j]->check_type() == 0 &&
                    !check_if_adj(all_occupiers[j]->check_move_target(),all_occupiers[j]->check_location()) &&
                    all_occupiers[j]->check_move_type() == 2) {
                // TODO: if both aren't coastal just fail it
                
                branch();

                // primary: require convoy
                add_convoy_condition(all_occupiers[j],all_occupiers[j]->check_location(),all_occupiers[j]->check_move_target());

                // alternate: piece cannot attack
                DiplomacyPiece *alt_piece = alternate->find_copied_piece(all_occupiers[j]);
                alt_piece->check_move_target()->remove_attacker(alt_piece);
                alt_piece->change_to_hold();
            }            
        }

        // fleets cannot be convoyed, non-coastal regions cannot be convoyed out of or into
        iter_regions[i]->cull_convoys();

        // support cannot be given to non-adjacent regions
        iter_regions[i]->cull_support();
    }
}

// TODO: add comment
void DiplomacyGame::guarantee_non_dislodgments(const std::vector<DiplomacyRegion *>& iter_regions) {
    for (int i = 0; i < iter_regions.size(); ++i) {
        if (!iter_regions[i]->occupied()) {
            continue;
        }
        std::vector<DiplomacyPiece *> all_occupiers = iter_regions[i]->check_all_occupiers();
        for (int j = 0; j < all_occupiers.size(); ++j) {
            if (all_occupiers[j]->check_move_type() != 0 && all_occupiers[j]->check_move_target()->occupied()) {
                DiplomacyPiece *j_potential_dislodger = all_occupiers[j]->check_move_target()->check_occupier();
                if (j_potential_dislodger->check_move_type() == 2 && j_potential_dislodger->check_move_target()->check_occupier() == 
                        all_occupiers[j]) {
                    // Potential for j_potential_dislodger to dislodge all_occupiers[j]. Branch on that
                    fprintf(stderr,"Unit in %s could be dislodged by unit in %s. BRANCHING.\n",
                            all_occupiers[j]->check_location()->check_names()[0],j_potential_dislodger->check_location()->check_names()[0]);

                    branch();

                    // primary: require that all_occupiers[j] not dislodged by j_potential_dislodger
                    add_not_dislodged_by_condition(all_occupiers[j],all_occupiers[j]->check_move_target());

                    // alternate: all_occupiers[j] does nothing
                    DiplomacyPiece *alt_j_occ = alternate->find_copied_piece(all_occupiers[j]);
                    alt_j_occ->check_move_target()->remove_attacker(alt_j_occ);
                    alt_j_occ->change_to_hold();
                }
            }
        }
    }
}

// Cut support, branching as necessary
void DiplomacyGame::cut_support(const std::vector<DiplomacyRegion *> iter_regions) {
    for (int i = 0; i < iter_regions.size(); ++i) {
        if (!iter_regions[i]->occupied())
            continue;
        std::vector<DiplomacyPiece *> all_occupiers = iter_regions[i]->check_all_occupiers();
        for (int j = 0; j < all_occupiers.size(); ++j) {
            if (all_occupiers[j]->check_move_type() == 2) {
                DiplomacyRegion *target = all_occupiers[j]->check_move_target();
                std::vector<DiplomacyPiece *> to_cut = target->check_all_occupiers();
                for (int k = 0; k < to_cut.size(); ++k) {
                    // First check if this relationship has been deemed safe in the iterative structure of resolution
                    bool safe = false;
                    for (int l = 0; l < safe_supports.size(); ++l) {
                        safe = safe || (safe_supports[l].attacker == all_occupiers[j] && safe_supports[l].safe_supporter == to_cut[k]);
                    }
                    if (safe) {
                        fprintf(stderr,"Piece in %s cannot cut support from %s: deemed safe.\n",
                                all_occupiers[j]->check_location()->check_names()[0], to_cut[k]->check_location()->check_names()[0]);
                        continue;
                    }

                    if (to_cut[k]->check_move_type() != 1 && to_cut[k]->check_move_type() != 3) {
                        continue;
                    }

                    if (to_cut[k]->check_owner() == all_occupiers[j]->check_owner()) {
                        fprintf(stderr,"%s's piece in %s cannot cut support given by its own piece in %s.\n",all_occupiers[j]->
                            check_owner()->check_name(),all_occupiers[j]->check_location()->check_names()[0],
                            to_cut[k]->check_location()->check_names()[0]);
                        continue;
                    }
                    
                    DiplomacyRegion *k_supported = to_cut[k]->check_move_target();
                    if (k_supported == iter_regions[i] || k_supported->check_parent() == iter_regions[i]) {
                        fprintf(stderr,"Piece in %s cannot cut support against itself from %s.\n",
                                all_occupiers[j]->check_location()->check_names()[0], to_cut[k]->check_location()->check_names()[0]);
                        // piece can't cut support that's against it
                        continue;
                    }
                    // if the supporter being attacked isn't adjacent, we need to deal with the convoy case
                    if (k_supported->occupied() && !check_if_adj(all_occupiers[j]->check_location(),all_occupiers[j]->check_move_target())){
                        if (k_supported->check_occupier()->check_move_type() == 4) {
                            DiplomacyPiece *k_convoyer = k_supported->check_occupier();
                            bool convoyingfrom = false;
                            for (int l = 0; l < k_convoyer->check_move_target()->check_convoys().size(); ++l) {
                                if (k_convoyer->check_move_target()->check_convoys()[l]->convoyer == k_convoyer) {
                                    convoyingfrom = (k_convoyer->check_move_target()->check_convoys()[l]->convoyed->check_location()
                                                                                                                    == iter_regions[i]);
                                    break;
                                }
                            }
                            if (convoyingfrom) {
                                fprintf(stderr,"Piece in %s trying to cut support from %s against its convoy in %s. BRANCHING.\n",
                                        all_occupiers[j]->check_location()->check_names()[0], to_cut[k]->check_location()->check_names()[0],
                                        k_convoyer->check_location()->check_names()[0]);
                                branch();

                                // primary: cut support, cut convoy, require convoy
                                to_cut[k]->check_move_target()->remove_support(to_cut[k]);
                                to_cut[k]->change_to_hold();
                                k_convoyer->check_move_target()->remove_convoy(k_convoyer);
                                k_convoyer->change_to_hold();
                                add_convoy_condition(all_occupiers[j],all_occupiers[j]->check_location(),
                                                                    all_occupiers[j]->check_move_target());

                                // alternate: cut neither, declare this attack/support pair safe
                                DiplomacyPiece *alt_attacker = alternate->find_copied_piece(all_occupiers[j]);
                                DiplomacyPiece *alt_to_cut_k = alternate->find_copied_piece(to_cut[k]);
                                alternate->add_safe_support(alt_attacker, alt_to_cut_k);

                                continue;
                            }
                        }
                    }
                    fprintf(stderr,"Piece in %s cuts support coming from %s.\n", all_occupiers[j]->check_location()->check_names()[0],
                            to_cut[k]->check_location()->check_names()[0]);
                    to_cut[k]->check_move_target()->remove_support(to_cut[k]);
                    to_cut[k]->change_to_hold();
                }
            }
        }
    }
}

// TODO: in progress
void DiplomacyGame::resolve() {
    // TODO: Implement 'no trading places without a convoy' rule
    // TODO: implement 'non-adjacent attacks require convoys' rule
    // both ^ should be in illegal move section

    // We only want to iterate over non-coasts
    std::vector<DiplomacyRegion *> iter_regions = region_list;
    for (int i = 0; i < iter_regions.size(); ++i) {
        if (iter_regions[i]->check_parent() != NULL) {
            iter_regions.erase(iter_regions.begin()+i);
            --i;
        }
    }

    fprintf(stderr, "Checking for illegal moves...\n");
    remove_illegal_moves(iter_regions);
    guarantee_non_dislodgments(iter_regions);
    fprintf(stderr,"Done checking for illegal moves.\nCutting support...\n");
    cut_support(iter_regions);
    fprintf(stderr,"Done cutting support.\nConsidering attacks...\n");

    // Now we go one-by-one through each region
    for (int i = 0; i < iter_regions.size(); ++i) {
        // pull data
        std::vector<DiplomacyPiece *> i_attackers = iter_regions[i]->check_attackers();
        std::vector<support *> i_att_support = iter_regions[i]->check_att_support();
        std::vector<support *> i_def_support = iter_regions[i]->check_def_support();

        // if no one's attacking, do nothing
        if (i_attackers.size() == 0) {
            continue;
        }

        fprintf(stderr,"Counting up support in %s...\n",iter_regions[i]->check_names()[0]);

        // count up support
        std::vector<int> attack_power;
        attack_power.resize(i_attackers.size(),1);
        for (int j = 0; j < i_att_support.size(); ++j) {
            for (int k = 0; k < i_attackers.size(); ++k) {
                if (i_attackers[k] == i_att_support[j]->supported) {
                    // branch on whether the support still exists
                    fprintf(stderr,"Support found for %s's unit %d by %s's unit %d. BRANCHING.\n",
                        i_attackers[k]->check_owner()->check_name(),
                            i_attackers[k]->check_self_num(), i_att_support[j]->supporter->check_owner()->check_name(),
                            i_att_support[j]->supporter->check_self_num());
                    attack_power[k] += 1;

                    branch();
                    // primary: require that support is still there
                    add_support_condition(iter_regions[i],i_att_support[j]);
                    // if attacker/defender have the same owner, also require non-dislodgment
                    if (iter_regions[i]->occupied()) {
                        if (iter_regions[i]->check_occupier()->check_owner() == i_att_support[j]->supporter->check_owner()) {
                            add_not_dislodged_by_condition(iter_regions[i]->check_occupier(),i_att_support[j]->supported->check_location());
                        }
                    }

                    // secondary: cut the support
                    DiplomacyPiece *alt_i_att_supp_j = alternate->find_copied_piece(i_att_support[j]->supporter);
                    alt_i_att_supp_j->check_move_target()->remove_support(alt_i_att_supp_j);
                    alt_i_att_supp_j->change_to_hold();
                }
            }
        }

        DiplomacyPiece *defender = NULL;
        int defense_power = 0;
        int defense_move_type = -2;
        if (iter_regions[i]->occupied()) {
            defender = iter_regions[i]->check_occupier();
            defense_move_type = defender->check_move_type();
            defense_power = 1;
            fprintf(stderr,"Counting up defensive support for %s's unit %d in %s...\n", defender->check_owner()->check_name(),
                        defender->check_self_num(), iter_regions[i]->check_names()[0]);
            for (int j = 0; j < i_def_support.size(); ++j) {
                if (i_def_support[j]->supported == defender) {
                    // branch on whether the support still exists
                    fprintf(stderr,"Support found for defender by %s's unit %d. BRANCHING.\n",
                            i_def_support[j]->supporter->check_owner()->check_name(), i_def_support[j]->supporter->check_self_num());
                    defense_power += 1;

                    branch();

                    // primary: require that support is still there
                    add_support_condition(iter_regions[i],i_def_support[j]);

                    // secondary: cut the support
                    DiplomacyPiece *alt_i_def_supp_j = alternate->find_copied_piece(i_def_support[j]->supporter);
                    alt_i_def_supp_j->check_move_target()->remove_support(alt_i_def_supp_j);
                    alt_i_def_supp_j->change_to_hold();
                }
                else {
                    fprintf(stderr,"%s's unit %d trying to support non-defending %s's unit %d.\n",
                            i_def_support[j]->supporter->check_owner()->check_name(), i_def_support[j]->supporter->check_self_num(),
                            i_def_support[j]->supported->check_owner()->check_name(), i_def_support[j]->supported->check_self_num());
                }
            }
        }
        else {
            fprintf(stderr,"Region %s unoccupied, no defenders.\n", iter_regions[i]->check_names()[0]);
        }

        // find the collection of most powerful attackers
        int max_att_power = 0;
        std::vector<DiplomacyPiece *> winning_attackers;
        for (int j = 0; j < attack_power.size(); ++j) {
            if (attack_power[j] == max_att_power) {
                winning_attackers.push_back(i_attackers[j]);
            }
            else if (attack_power[j] > max_att_power) {
                max_att_power = attack_power[j];
                winning_attackers.clear();
                winning_attackers.push_back(i_attackers[j]);
            }
        }

        // if there is more than one winner or tie or defense wins, nothing happens
        if (winning_attackers.size() > 1 || (max_att_power <= defense_power && defense_move_type != 2)) {
            fprintf(stderr,"Standoff or defense win in %s.\n",iter_regions[i]->check_names()[0]);
            for (int j = 0; j < i_attackers.size(); ++j) {
                i_attackers[j]->change_to_hold();
            }
            iter_regions[i]->clear_attacker_records();
            continue;
        }

        bool att_def_same_owner = defender != NULL;
        if (att_def_same_owner) {
            att_def_same_owner = defender->check_owner() == winning_attackers[0]->check_owner();
            if (att_def_same_owner) {
                fprintf(stderr,"Attacker and defender have the same owner.\n");
            }
        }

        // if attack is stronger than defense (and from a different player), attack wins
        if (max_att_power > defense_power && !att_def_same_owner) {
            fprintf(stderr,"Attacker (%s's piece %d) wins in %s.\n",winning_attackers[0]->check_owner()->check_name(),
                    winning_attackers[0]->check_self_num(), iter_regions[i]->check_names()[0]);
            // no matter what: winner makes it in, other attackers and supports set to hold move records (except convoys!) are cleared
            DiplomacyRegion *attack_source = winning_attackers[0]->check_location();
            attack_source->remove_piece(winning_attackers[0]);
            winning_attackers[0]->check_move_target()->add_piece(winning_attackers[0]);
            winning_attackers[0]->set_location(winning_attackers[0]->check_move_target());
            winning_attackers[0]->change_to_hold();
            for (int j = 0; j < i_attackers.size(); ++j) {
                if (i_attackers[j] == winning_attackers[0]) {
                    continue;
                }
                i_attackers[j]->change_to_hold();
            }
            iter_regions[i]->clear_attacker_records();
            if (defender != NULL) {
                // if defender is attacking somewhere else, branch on whether they get out
                if (defender->check_move_type() == 2) {
                    fprintf(stderr,"Attacker is winning in %s, defender might make it out. BRANCHING.\n",iter_regions[i]->check_names()[0]);                    
                    branch();

                    // primary: require that defender makes it out.
                    // if defender attacking attacker's region, also add a convoy requirement
                    add_moved_condition(defender,defender->check_location());
                    if (same_region(defender->check_move_target(),attack_source)) {
                        add_convoy_condition(defender,defender->check_location(),defender->check_move_target());
                    }

                    // secondary: winner makes it in, defender is required to retreat
                    DiplomacyPiece *alt_defender = alternate->find_copied_piece(defender);
                    DiplomacyRegion *alt_att_source = alternate->get_region_by_name(attack_source->check_names()[0]);
                    alternate->dislodge_unit(alt_defender,alt_att_source);
                }

                // if defender is not attacking somewhere else, they have to dislodge
                else {
                    fprintf(stderr,"Attacker is winning in %s, defender forced to retreat.\n",iter_regions[i]->check_names()[0]);
                    // if defender is convoying or support, convoy or support is broken
                    dislodge_unit(defender,attack_source);
                }
            }
        }
        // if attack power weaker (or from same player as defense) but defense trying to move out, branch on that condition
        else if ((max_att_power <= defense_power || (att_def_same_owner && max_att_power > defense_power))
                    && defense_move_type == 2) {
            fprintf(stderr,"Attacker (%s's piece %d) wins in %s if defender gets out. BRANCHING.\n",
                    winning_attackers[0]->check_owner()->check_name(), winning_attackers[0]->check_self_num(),
                    iter_regions[i]->check_names()[0]);

            // all cases: clear other moves
            for (int j = 0; j < i_attackers.size(); ++j) {
                if (i_attackers[j] == winning_attackers[0]) {
                    continue;
                }
                i_attackers[j]->change_to_hold();
            }
            iter_regions[i]->clear_attacker_records();

            branch();

            // primary: attacker moves in, require defender gets out
            winning_attackers[0]->check_move_target()->add_piece(winning_attackers[0]);
            winning_attackers[0]->check_location()->remove_piece(winning_attackers[0]);
            winning_attackers[0]->set_location(winning_attackers[0]->check_move_target());
            winning_attackers[0]->change_to_hold();

            add_moved_condition(defender,defender->check_location());

            // alternate: attacker stays put:
            DiplomacyPiece *alt_win_att = alternate->find_copied_piece(winning_attackers[0]);
            alt_win_att->check_move_target()->remove_attacker(alt_win_att);
            alt_win_att->change_to_hold();
        }
        // If winning attacker would dislodge same player's unit, nothing happens
        else if (att_def_same_owner && max_att_power > defense_power) {
            fprintf(stderr,"Winning attacker would have to dislodge defender owned by same player.\n");
            for (int j = 0; j < i_attackers.size(); ++j) {
                i_attackers[j]->change_to_hold();
            }
            iter_regions[i]->clear_attacker_records();
            continue;
        }
        else {
            fprintf(stderr,"ERROR: Unknown situation happening. HELP HELP.\n");
        }
    }
}

void DiplomacyGame::add_condition(conditiontype ctype) {
    conditions.terms.push_back(OrCondition(this));
    conditions.terms.back().factors.push_back(ConditionBox(this,ctype));
}

void DiplomacyGame::add_safe_support(DiplomacyPiece *att, DiplomacyPiece *safe_supp) {
    safe_support newsafesupp = {att, safe_supp};
    safe_supports.push_back(newsafesupp);
}

void DiplomacyGame::dislodge_unit(DiplomacyPiece *piece, DiplomacyRegion *fromp) {
    if (piece->check_move_type() == 4) {
        piece->check_move_target()->remove_convoy(piece);
        piece->change_to_hold();
    }
    else if (piece->check_move_type() == 1 || piece->check_move_type() == 3) {
        piece->check_move_target()->remove_support(piece);
        piece->change_to_hold();
    }
    add_retreat(piece);
    add_dislodgment(piece,fromp);
}

void DiplomacyGame::add_dislodgment(DiplomacyPiece *disld, DiplomacyRegion *byp) {
    dislodgment *new_disl = new dislodgment;
    new_disl->dislodged = disld;
    new_disl->by = byp;
    dislodgments.push_back(new_disl);
}

void DiplomacyGame::add_convoy_condition(DiplomacyPiece *piece, DiplomacyRegion *fromp, DiplomacyRegion *top) {
    add_condition(convoy_condition);
    conditions.terms.back().factors.back().convoy_cond.push_back(ConvoyCondition(this, piece, fromp, top));
}

void DiplomacyGame::add_support_condition(DiplomacyRegion *region, support *req_support) {
    add_condition(support_condition);
    conditions.terms.back().factors.back().supp_cond.push_back(SupportCondition(this,region,req_support->supported,req_support->supporter));
}

void DiplomacyGame::add_moved_condition(DiplomacyPiece *piece, DiplomacyRegion *from) {
    add_condition(moved_condition);
    conditions.terms.back().factors.back().moved_cond.push_back(MovedCondition(this,piece,from));
}

void DiplomacyGame::add_no_trade_condition(DiplomacyPiece *piece1p, DiplomacyPiece *piece2p) {
    add_condition(no_trade_condition);
    conditions.terms.back().factors.back().no_trade_cond.push_back(NoTradeCondition(this,piece1p,piece2p));
}

void DiplomacyGame::add_no_trade_or_convoy_condition(DiplomacyPiece *piece1, DiplomacyPiece *piece2) {
    add_condition(no_trade_condition);
    conditions.terms.back().factors.back().no_trade_cond.push_back(NoTradeCondition(this,piece1,piece2));
    conditions.terms.back().factors.push_back(ConditionBox(this,convoy_condition));
    conditions.terms.back().factors.back().convoy_cond.push_back(ConvoyCondition(this,piece1,piece1->check_location(),
            piece1->check_move_target()));
    conditions.terms.back().factors.push_back(ConditionBox(this,convoy_condition));
    conditions.terms.back().factors.back().convoy_cond.push_back(ConvoyCondition(this,piece2,piece2->check_location(),
            piece2->check_move_target()));
}

void DiplomacyGame::add_not_dislodged_by_condition(DiplomacyPiece *piecep, DiplomacyRegion *byp) {
    add_condition(not_dislodged_condition);
    conditions.terms.back().factors.back().not_disl_cond.push_back(NotDislodgedByCondition(this, piecep, byp));
}

bool DiplomacyGame::is_sea_path(DiplomacyRegion *start, DiplomacyRegion *end, const std::vector<DiplomacyRegion *>& steps) {
    if (start == end) {
        return true;
    }

    std::vector<DiplomacyRegion *> possibilities;
    if (start->check_coasts().empty()) { // we're out to sea - could be at the end
        std::vector<DiplomacyRegion *> adjacencies = start->check_adj_regions();
        for (int i = 0; i < adjacencies.size(); ++i) {
            if (!adjacencies[i]->check_coast()) {
                possibilities.push_back(adjacencies[i]);
            }
            else if (adjacencies[i]->check_parent() == end) {
                return true;
            }
        }
    }

    if (steps.empty()) {
        return false;
    }

    for (int i = 0; i < start->check_coasts().size(); ++i) { // should only be non-empty first time
        std::vector<DiplomacyRegion *> seas = start->check_coasts()[i]->check_adj_regions();
        for (int j = 0; j < seas.size(); ++j) {
            if (seas[j]->check_coast() == false) {
                possibilities.push_back(seas[j]);
            }
        }
    }

    bool acc = false;
    for (int i = 0; i < steps.size(); ++i) {
        for (int j = 0; j < possibilities.size(); ++j) {
            if (end == possibilities[j]) {
                return false; // in this case end must be a coast or sea, which is illegal
            }
            if (steps[i] == possibilities[j]) {
                std::vector<DiplomacyRegion *> newsteps = steps;
                newsteps.erase(newsteps.begin()+i);
                acc = acc || is_sea_path(steps[i],end,newsteps);
            }
        }
    }
    return acc;
}

void DiplomacyGame::add_retreat(DiplomacyPiece *retreater) {
    req_retreats.push_back(retreater);
}

DiplomacyGame *DiplomacyGame::pass() {
    return conditions.pass();
}

//
// Command line display functions
//

void DiplomacyGame::display() {
    printf("Game info:\nThe season is: %d.\n\nPieces:\n-------\n",season);
    for (int i = 0; i < player_list.size(); ++i) {
        //printf("Player %d\n",i+1);
        player_list[i]->display();
    }
    /*for (int i = 0; i < region_list.size(); ++i) {
        region_list[i]->display();
    }*/
    printf("\nThe following pieces need to retreat:\n");
    for (int i = 0; i < req_retreats.size(); ++i) {
        req_retreats[i]->display();
    }
    printf("\n");
}