// diplomacy.h

// Andrew MacKie-Mason
// CMSC 162, University of Chicago

// Project site: http://brick.cs.uchicago.edu/Courses/CMSC-16200/2012/pmwiki/pmwiki.php/Student/Diplomacy

// Official map: http://www.dipwiki.com/index.php?title=Diplomacy
// Rules: http://www.diplom.org/~diparch/resources/rulebooks/2000AH4th.pdf

#ifndef __DIPLOMACY_H__
#define __DIPLOMACY_H__

#include <vector>
#include <string>

class DiplomacyGame;
class DiplomacyPlayer;
class DiplomacyRegion;
class DiplomacyPiece;

class Condition;
class AndCondition;
class OrCondition;
class ConditionBox;
class ConvoyCondition;

typedef struct support {
    DiplomacyPiece *supporter;
    DiplomacyPiece *supported;
} support;
typedef struct convoy {
    DiplomacyPiece *convoyer;
    DiplomacyPiece *convoyed;
} convoy;

typedef struct safe_support {
    DiplomacyPiece *attacker;
    DiplomacyPiece *safe_supporter;
} safe_support;

typedef enum conditiontype {
    convoy_condition,
    moved_condition
} conditiontype;

extern DiplomacyGame *bison_box; // parser will store things here
extern FILE *yyin;
extern int yyparse();

class AndCondition {
public:
    DiplomacyGame *game;
    std::vector<OrCondition> terms;
    bool pass();
    AndCondition();
    AndCondition(DiplomacyGame *gamep);
    AndCondition(const AndCondition& old, DiplomacyGame *newgame);
};

class OrCondition {
public:
    DiplomacyGame *game;
    std::vector<ConditionBox> factors;
    bool pass();
    OrCondition(DiplomacyGame *gamep);
    OrCondition(const OrCondition& old, DiplomacyGame *newgame);
};

class ConvoyCondition {
public:
    DiplomacyGame *game;
    DiplomacyPiece *forpiece;
    DiplomacyRegion *from;
    DiplomacyRegion *to;

    bool pass();
    ConvoyCondition(DiplomacyGame *gamep, DiplomacyPiece *forpiecep, DiplomacyRegion *fromp, DiplomacyRegion *top);
    ConvoyCondition(const ConvoyCondition& old, DiplomacyGame *newgame);
};

class MovedCondition {
public:
    DiplomacyGame *game;
    DiplomacyPiece *piece;
    DiplomacyRegion *from;

    bool pass();
    MovedCondition(DiplomacyGame *gamep, DiplomacyPiece *piecep, DiplomacyRegion *fromp);
    MovedCondition(const MovedCondition& old, DiplomacyGame *newgame);
};

class ConditionBox {
public:
    DiplomacyGame *game;
    conditiontype cond_type;
    std::vector<ConvoyCondition> convoy_cond;
    std::vector<MovedCondition> moved_cond;
    bool pass();
    ConditionBox(DiplomacyGame *gamep, conditiontype ctype);
    ConditionBox(const ConditionBox& old, DiplomacyGame *newgame);
};

class DiplomacyGame {
private:
    int season;
    std::vector<DiplomacyPlayer *> player_list; // vector of pointers to players
    std::vector<DiplomacyRegion *> region_list; // vector of pointers to regions

    // move resolution tools
    AndCondition conditions;
    DiplomacyGame *alternate;
    std::vector<safe_support> safe_supports;
    std::vector<DiplomacyPiece *> req_retreats;
public:
    DiplomacyGame(char *filename);
    DiplomacyGame();
    DiplomacyGame(const DiplomacyGame& old); // copy constructor
    ~DiplomacyGame();
    void load_from_file(char *filename);
    void clear_game();

    // game setup:
    void add_region(char *name);
    int add_abbrv(char *name, char *abbrv);
    int add_adj(char *newname, char *oldname);
    int add_par(char *newname, char *oldname);
    int add_player(char *name, char *email);
    int add_piece(char *player, char piece, char *location);

    void save_game(char *filename);

    // access:

    int check_season();
    DiplomacyPlayer *get_player_by_num(int player);
    int get_player_num_by_name(char *name);
    int get_player_num_by_email(char *email);
    DiplomacyRegion *get_region_by_name(char *name);
    bool check_if_adj(DiplomacyRegion *source, DiplomacyRegion *target);
    bool check_if_supp_adj(DiplomacyRegion *source, DiplomacyRegion *target);
    DiplomacyGame *check_alternate();

    // move handling
    //int record_move(DiplomacyPiece *piece, int move, DiplomacyRegion *target);
    void resolve();
    void add_safe_support(DiplomacyPiece *att, DiplomacyPiece *safe_supp);
    void add_convoy_condition(DiplomacyPiece *piece, DiplomacyRegion *fromp, DiplomacyRegion *top);
    void add_moved_condition(DiplomacyPiece *piecep, DiplomacyRegion *fromp);
    bool is_sea_path(DiplomacyRegion *start, DiplomacyRegion *end, const std::vector<DiplomacyRegion *>& steps);
    void add_retreat(DiplomacyPiece *retreater);
    bool pass();

    // command line display:
    void display();
};

class DiplomacyPlayer {
private:
    DiplomacyGame *game;
    char *name;
    char *email_address;
    std::vector<DiplomacyPiece *> pieces; // vector of pointers to controlled pieces
public:
    DiplomacyPlayer(char *name, char *email);
    DiplomacyPlayer();
    DiplomacyPlayer(const DiplomacyPlayer& old, DiplomacyGame *gamep); // copy constructor
    ~DiplomacyPlayer();

    // game setup:
    void add_piece(DiplomacyPiece *piece);

    // access:
    char *check_name();
    char *check_email();
    std::vector<DiplomacyPiece *> check_pieces();

    // command line display:
    void display();
};

class DiplomacyRegion {
private:
    DiplomacyGame *game;
    std::vector<char *> names; // vector of strings with valid names/abbrvs for the region
    std::vector<DiplomacyPiece *> occupiers;
    bool coast;
    std::vector<DiplomacyRegion *> coasts; // a vector of pointers to this region's costs, if coastal
    DiplomacyRegion *parent; // if a cost, pointer to the parent region
    std::vector<DiplomacyRegion *> adj_regions; // vector of pointers to adjacent regions

    // move recording
    bool occupier_defending; // false means it won't necessarily defend, it its move succeeds
    std::vector<support *> defending_support;
    std::vector<DiplomacyPiece *> attacking_pieces;
    std::vector<support *> attacking_support;
    std::vector<convoy *> attacking_convoys;

public:
    // constructors/setup
    DiplomacyRegion(char *name, DiplomacyGame *gamep);
    DiplomacyRegion();
    DiplomacyRegion(const DiplomacyRegion& old, DiplomacyGame *newgame);
    ~DiplomacyRegion();

    void copy_region_pointers(const DiplomacyRegion& old);
    void clear_region();
    void clear_move_records();
    void clear_move_records_no_convoy();

    // game setup:
    void add_abbrv(char *abbrv);
    void add_adj(DiplomacyRegion *adj);
    int add_parent(DiplomacyRegion *parent);
    void add_coast(DiplomacyRegion *child);
    void add_piece(DiplomacyPiece *newpiece);

    // move recording
    void set_occupier_defending(bool status);
    void set_hold();
    void add_def_supp(DiplomacyPiece *supporter, DiplomacyPiece *supported);
    void add_attacker(DiplomacyPiece *piece);
    void add_att_supp(DiplomacyPiece *supporter, DiplomacyPiece *supported);
    void add_att_conv(DiplomacyPiece *convoyer, DiplomacyPiece *convoyed);

    // move handling
    void clear_convoys();
    void cull_convoys();
    void cull_support();
    void remove_support(DiplomacyPiece *supporter);
    void remove_convoy(DiplomacyPiece *convoyer);
    void remove_piece(DiplomacyPiece *piecep);

    // access functions
    bool occupied();
    DiplomacyRegion *check_parent();
    bool check_coast();
    std::vector<DiplomacyRegion *> check_adj_regions();
    std::vector<DiplomacyRegion *> check_coasts();
    std::vector<char *> check_names();
    std::vector<DiplomacyPiece *> check_occupiers();
    std::vector<DiplomacyPiece *> check_all_occupiers(); // includes coastal occupiers
    DiplomacyPiece *check_occupier();
    std::vector<DiplomacyPiece *> check_attackers();
    std::vector<support *> check_att_support();
    std::vector<convoy *> check_convoys();
    std::vector<support *> check_def_support();

    // command line display:
    void display();
};

class DiplomacyPiece {
private:
    DiplomacyGame *game;
    DiplomacyPlayer *owner;
    DiplomacyRegion *location;
    int piece_type; // 0: army  1: fleet

    // move recording
    int move_type; // -1: no move yet   0: hold   1: support defend   2: attack   3: support attack   4: convey attack
    DiplomacyRegion *move_target;
public:
    DiplomacyPiece(DiplomacyPlayer *playerp, int piece, DiplomacyRegion *locationp, DiplomacyGame *gamep);
    DiplomacyPiece();
    DiplomacyPiece(const DiplomacyPiece& old, DiplomacyPlayer *own, DiplomacyGame *newgame);
    ~DiplomacyPiece();

    void copy_region_pointers(const DiplomacyPiece& old);
    void clear_move();

    // move recording
    void set_move_type(int type);
    void set_move_target(DiplomacyRegion *target);

    // move resolution
    void set_location(DiplomacyRegion *loc);

    // access
    DiplomacyRegion *check_location();
    DiplomacyPlayer *check_owner();
    int check_self_num();
    int check_type();
    int check_move_type();
    DiplomacyRegion *check_move_target();

    // command line display:
    void display();
};

#endif