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
    player_list.resize(old.player_list.size(), NULL);
    for (int i = 0; i < player_list.size(); ++i) {
        player_list[i] = new DiplomacyPlayer(*(old.player_list[i]),this);
    }
    req_retreats.resize(old.req_retreats.size(), NULL);
    for (int i = 0; i < req_retreats.size(); ++i) {
        req_retreats[i] = get_player_by_num(get_player_num_by_name(old.req_retreats[i]->check_owner()->check_name()))->check_pieces()
                        [old.req_retreats[i]->check_self_num()];
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
    alternate = old.alternate;
    safe_supports.resize(old.safe_supports.size());
    for (int i = 0; i < safe_supports.size(); ++i) {
        safe_supports[i].attacker = get_player_by_num(get_player_num_by_name(old.safe_supports[i].attacker->check_owner()->
                check_name()))->check_pieces()[old.safe_supports[i].attacker->check_self_num()];
        safe_supports[i].safe_supporter = get_player_by_num(get_player_num_by_name(old.safe_supports[i].safe_supporter->
                check_owner()->check_name()))->check_pieces()[old.safe_supports[i].safe_supporter->check_self_num()];
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

//
// move handling functions
//

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
    // Eliminate obviously illegal moves and set non-moves to holds
    for (int i = 0; i < iter_regions.size(); ++i) {
        if (!iter_regions[i]->occupied())
            continue;
        std::vector<DiplomacyPiece *> all_occupiers = iter_regions[i]->check_all_occupiers();
        for (int j = 0; j < all_occupiers.size(); ++j) {
            // Things without an order hold
            if (all_occupiers[j]->check_move_type() == -1) {
                all_occupiers[j]->set_move_type(0);
                all_occupiers[j]->set_move_target(all_occupiers[j]->check_location());
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
                    fprintf(stderr,"Fleet or non-coastal army in %s tried to affect non-adjacent region %s.\n",
                            all_occupiers[j]->check_location()->check_names()[0],
                            all_occupiers[j]->check_move_target()->check_names()[0]);
                    all_occupiers[j]->set_move_type(0);
                    all_occupiers[j]->set_move_target(all_occupiers[j]->check_location());
                    all_occupiers[j]->check_location()->set_occupier_defending(true);
                }
            }

            // Armies cannot attack oceans, where oceans are things occupied by fleets that are not coastal
            if (all_occupiers[j]->check_type() == 0 && all_occupiers[j]->check_move_target()->check_occupier() != NULL) {
                if (all_occupiers[j]->check_move_target()->check_occupier()->check_type() == 1 &&
                        all_occupiers[j]->check_move_target()->check_coasts().size() == 0) {
                    fprintf(stderr,"Army in %s tried to attack fleet in aquatic region %s.\n",
                        all_occupiers[j]->check_location()->check_names()[0],
                        all_occupiers[j]->check_move_target()->check_names()[0]);
                    all_occupiers[j]->set_move_type(0);
                    all_occupiers[j]->set_move_target(all_occupiers[j]->check_location());
                    all_occupiers[j]->check_location()->set_occupier_defending(true);
                }
            }
        }

        // fleets cannot be convoyed, non-coastal regions cannot be convoyed out of or into
        iter_regions[i]->cull_convoys();

        // support cannot be given to non-adjacent regions
        iter_regions[i]->cull_support();
    }

    fprintf(stderr,"Done checking for illegal moves.\nCutting support...\n");
    // Cut support, branching as necessary
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
                    DiplomacyRegion *k_supported = to_cut[k]->check_move_target();
                    if (k_supported == iter_regions[i] || k_supported->check_parent() == iter_regions[i]) {
                        fprintf(stderr,"Piece in %s cannot cut support against itself from %s.\n",
                                all_occupiers[j]->check_location()->check_names()[0], to_cut[k]->check_location()->check_names()[0]);
                        // piece can't cut support that's against it
                        continue;
                    }
                    // if the supporter being attacked isn't adjacent, we need to deal with the convoy case
                    if (k_supported->occupied() && !check_if_adj(all_occupiers[j]->check_location(),all_occupiers[j]->check_move_target())) {
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
                                fprintf(stderr,"Piece in %s trying to cut support from %s against its convoy in %s. Branching.\n",
                                        all_occupiers[j]->check_location()->check_names()[0], to_cut[k]->check_location()->check_names()[0],
                                        k_convoyer->check_location()->check_names()[0]);
                                // branch off a copy
                                DiplomacyGame *branch = new DiplomacyGame(*this);
                                branch->alternate = alternate;
                                alternate = branch;

                                // primary: cut support, cut convoy, require convoy
                                to_cut[k]->check_move_target()->remove_support(to_cut[k]);
                                to_cut[k]->set_move_type(0);
                                to_cut[k]->set_move_target(to_cut[k]->check_location());
                                to_cut[k]->check_location()->set_occupier_defending(true);

                                k_convoyer->check_move_target()->remove_convoy(k_convoyer);
                                k_convoyer->set_move_type(0);
                                k_convoyer->set_move_target(k_convoyer->check_location());
                                k_convoyer->check_location()->set_occupier_defending(true);

                                add_convoy_condition(all_occupiers[j],all_occupiers[j]->check_location(),
                                                                    all_occupiers[j]->check_move_target());

                                // alternate: cut neither, declare this attack/support pair safe
                                DiplomacyPiece *alt_attacker = alternate->get_player_by_num(alternate->get_player_num_by_name(
                                        all_occupiers[j]->check_owner()->check_name()))->check_pieces()[all_occupiers[j]->check_self_num()];
                                DiplomacyPiece *alt_to_cut_k = alternate->get_player_by_num(alternate->get_player_num_by_name(
                                        to_cut[k]->check_owner()->check_name()))->check_pieces()[to_cut[k]->check_self_num()];
                                alternate->add_safe_support(alt_attacker, alt_to_cut_k);

                                continue;
                            }
                        }
                    }
                    fprintf(stderr,"Piece in %s cuts support coming from %s.\n", all_occupiers[j]->check_location()->check_names()[0],
                            to_cut[k]->check_location()->check_names()[0]);
                    to_cut[k]->check_move_target()->remove_support(to_cut[k]);
                    to_cut[k]->set_move_type(0);
                    to_cut[k]->set_move_target(to_cut[k]->check_location());
                    to_cut[k]->check_location()->set_occupier_defending(true);
                }
            }
        }
    }

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
                    fprintf(stderr,"Support found for %s's unit %d by %s's unit %d.\n", i_attackers[k]->check_owner()->check_name(),
                            i_attackers[k]->check_self_num(), i_att_support[j]->supporter->check_owner()->check_name(),
                            i_att_support[j]->supporter->check_self_num());
                    attack_power[k] += 1;
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
                    fprintf(stderr,"Support found for defender by %s's unit %d.\n", i_def_support[j]->supporter->check_owner()->check_name(),
                            i_def_support[j]->supporter->check_self_num());
                    defense_power += 1;
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

        // TODO what if defender = NULL?
        // if there is more than one winner or tie or defense wins, nothing happens
        if (winning_attackers.size() > 1 || (max_att_power <= defense_power && defense_move_type != 2)) {
            fprintf(stderr,"Standoff or defense win in %s.\n",iter_regions[i]->check_names()[0]);
            for (int j = 0; j < i_attackers.size(); ++j) {
                i_attackers[j]->set_move_type(0);
                i_attackers[j]->set_move_target(i_attackers[j]->check_location());
                i_attackers[j]->check_location()->set_occupier_defending(true);
            }
            for (int j = 0; j < i_att_support.size(); ++j) {
                i_att_support[j]->supporter->set_move_type(0);
                i_att_support[j]->supporter->set_move_target(i_att_support[j]->supporter->check_location());
                i_att_support[j]->supporter->check_location()->set_occupier_defending(true);
            }
            for (int j = 0; j < i_def_support.size(); ++j) {
                i_def_support[j]->supporter->set_move_type(0);
                i_def_support[j]->supporter->set_move_target(i_def_support[j]->supporter->check_location());
                i_def_support[j]->supporter->check_location()->set_occupier_defending(true);
            }
            iter_regions[i]->clear_move_records_no_convoy();
            continue;
        }

        // if attack is stronger than defense, attack wins
        else if (max_att_power > defense_power) {
            fprintf(stderr,"Attacker (%s's piece %d) wins in %s.\n",winning_attackers[0]->check_owner()->check_name(),
                    winning_attackers[0]->check_self_num(), iter_regions[i]->check_names()[0]);
            // no matter what: winner makes it in, other attackers and supports set to hold move records (except convoys!) are cleared
            winning_attackers[0]->check_move_target()->add_piece(winning_attackers[0]);
            winning_attackers[0]->check_location()->remove_piece(winning_attackers[0]);
            winning_attackers[0]->set_location(winning_attackers[0]->check_move_target());
            winning_attackers[0]->set_move_type(0);
            winning_attackers[0]->set_move_target(winning_attackers[0]->check_location());
            for (int j = 0; j < i_attackers.size(); ++j) {
                if (i_attackers[j] == winning_attackers[0]) {
                    continue;
                }
                i_attackers[j]->set_move_type(0);
                i_attackers[j]->set_move_target(i_attackers[j]->check_location());
                i_attackers[j]->check_location()->set_occupier_defending(true);
            }
            for (int j = 0; j < i_att_support.size(); ++j) {
                i_att_support[j]->supporter->set_move_type(0);
                i_att_support[j]->supporter->set_move_target(i_att_support[j]->supporter->check_location());
                i_att_support[j]->supporter->check_location()->set_occupier_defending(true);
            }
            for (int j = 0; j < i_def_support.size(); ++j) {
                i_def_support[j]->supporter->set_move_type(0);
                i_def_support[j]->supporter->set_move_target(i_def_support[j]->supporter->check_location());
                i_def_support[j]->supporter->check_location()->set_occupier_defending(true);
            }
            iter_regions[i]->clear_move_records_no_convoy();
            if (defender != NULL) {
                // if defender is attacking somewhere else, branch on whether they get out
                if (defender->check_move_type() == 2) {
                    fprintf(stderr,"Attacker is winning in %s, defender might make it out. Branching.\n",iter_regions[i]->check_names()[0]);
                    // branch off a copy
                    DiplomacyGame *branch = new DiplomacyGame(*this);
                    branch->alternate = alternate;
                    alternate = branch;

                    // primary: require that defender makes it out
                    add_moved_condition(defender,defender->check_location());

                    // secondary: winner makes it in, defender is required to retreat
                    DiplomacyPiece *alt_defender = alternate->get_player_by_num(alternate->get_player_num_by_name(
                                    defender->check_owner()->check_name()))->check_pieces()[defender->check_self_num()];
                    alternate->add_retreat(alt_defender);
                }

                // if defender is not attacking somewhere else, they have to dislodge
                else {
                    fprintf(stderr,"Attacker is winning in %s, defender forced to retreat.\n",iter_regions[i]->check_names()[0]);
                    // if defender is convoying, convoy is broken
                    if (defender->check_move_type() == 4) {
                        defender->check_move_target()->remove_convoy(defender);
                        defender->set_move_type(0);
                        defender->set_move_target(defender->check_location());
                    }
                    // TODO: problem here: dislodged supporter shouldn't affect region that dislodged him
                    // not sure how to deal with this yet

                    add_retreat(defender);
                }
            }
        }
        // if attack power weaker but defense trying to move out, branch on that condition
        else if (max_att_power <= defense_power && defense_move_type == 2) {
            fprintf(stderr,"Attacker (%s's piece %d) wins in %s if defender gets out. Branching.\n",
                    winning_attackers[0]->check_owner()->check_name(), winning_attackers[0]->check_self_num(),
                    iter_regions[i]->check_names()[0]);

            // all cases: clear other moves
            for (int j = 0; j < i_attackers.size(); ++j) {
                if (i_attackers[j] == winning_attackers[0]) {
                    continue;
                }
                i_attackers[j]->set_move_type(0);
                i_attackers[j]->set_move_target(i_attackers[j]->check_location());
                i_attackers[j]->check_location()->set_occupier_defending(true);
            }
            for (int j = 0; j < i_att_support.size(); ++j) {
                i_att_support[j]->supporter->set_move_type(0);
                i_att_support[j]->supporter->set_move_target(i_att_support[j]->supporter->check_location());
                i_att_support[j]->supporter->check_location()->set_occupier_defending(true);
            }
            for (int j = 0; j < i_def_support.size(); ++j) {
                i_def_support[j]->supporter->set_move_type(0);
                i_def_support[j]->supporter->set_move_target(i_def_support[j]->supporter->check_location());
                i_def_support[j]->supporter->check_location()->set_occupier_defending(true);
            }
            iter_regions[i]->clear_move_records_no_convoy();

            // branch off a copy
            DiplomacyGame *branch = new DiplomacyGame(*this);
            branch->alternate = alternate;
            alternate = branch;

            // primary: attacker moves in, require defender gets out
            winning_attackers[0]->check_move_target()->add_piece(winning_attackers[0]);
            winning_attackers[0]->check_location()->remove_piece(winning_attackers[0]);
            winning_attackers[0]->set_location(winning_attackers[0]->check_move_target());
            winning_attackers[0]->set_move_type(0);
            winning_attackers[0]->set_move_target(winning_attackers[0]->check_location());

            add_moved_condition(defender,defender->check_location());

            // alternate: attacker stays put:
            DiplomacyPiece *alt_win_att = alternate->get_player_by_num(alternate->get_player_num_by_name(winning_attackers[0]->
                    check_owner()->check_name()))->check_pieces()[winning_attackers[0]->check_self_num()];
            alt_win_att->set_move_type(0);
            alt_win_att->set_move_target(alt_win_att->check_location());
            alt_win_att->check_location()->set_occupier_defending(true);
        }
        else {
            fprintf(stderr,"ERROR: Unknown situation happening. HELP HELP.\n");
        }
    }
    // TODO: the rest of move resolution -- any more??
}

void DiplomacyGame::add_safe_support(DiplomacyPiece *att, DiplomacyPiece *safe_supp) {
    safe_support newsafesupp = {att, safe_supp};
    safe_supports.push_back(newsafesupp);
}

void DiplomacyGame::add_convoy_condition(DiplomacyPiece *piece, DiplomacyRegion *fromp, DiplomacyRegion *top) {
    conditions.terms.push_back(OrCondition(this));
    conditions.terms.back().factors.push_back(ConditionBox(this,convoy_condition));
    conditions.terms.back().factors.back().convoy_cond.push_back(ConvoyCondition(this, piece, fromp, top));
}

void DiplomacyGame::add_moved_condition(DiplomacyPiece *piece, DiplomacyRegion *from) {
    conditions.terms.push_back(OrCondition(this));
    conditions.terms.back().factors.push_back(ConditionBox(this,moved_condition));
    conditions.terms.back().factors.back().moved_cond.push_back(MovedCondition(this,piece,from));
}

bool DiplomacyGame::is_sea_path(DiplomacyRegion *start, DiplomacyRegion *end, const std::vector<DiplomacyRegion *>& steps) {
    if (start == end) {
        return true;
    }
    if (steps.empty()) {
        return false;
    }

    std::vector<DiplomacyRegion *> possibilities;
    if (start->check_coasts().empty()) { // we're out to sea
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

bool DiplomacyGame::pass() {
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