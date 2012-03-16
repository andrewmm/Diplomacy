%{

    // diplomacygame_filein_bison.y
    // Bison file for diplomacy savefile parsing

    // Andrew MacKie-Mason
    // CMSC 162, University of Chicago
    // Project site: http://brick.cs.uchicago.edu/Courses/CMSC-16200/2012/pmwiki/pmwiki.php/Student/Diplomacy

    #include <stdio.h>
    #include <stdlib.h>

    #include "diplomacy.h"

    int yyerror(char *);
    int yylex(void);

    DiplomacyGame *bison_box;

    void verify_region(DiplomacyRegion *region, char *region_name);
    void verify_piece(DiplomacyPiece *piece, char *region_name);
    void verify_type(DiplomacyPiece *piece, char label, char *region_name);
    void verify_fleet(DiplomacyPiece *piece);
    void verify_army(DiplomacyPiece *piece);
    void verify_owner(DiplomacyPiece *piece, char *region_name);

    int i = 0;
%}

%union {
    char *word;
}

%token<word> NAME EMAIL UNIT
%token CONF_BEGIN_TOK ORD_BEGIN_TOK
%token CONF_END_TOK ORD_END_TOK

%token ADJ PAR EQ SUPP CONV HOLD
%token PLAYER_TOK

%token EOLN_TOK EOF_TOK
%token ERROR

%start input

%%

input
    : beginconf conflines CONF_END_TOK EOF_TOK { YYACCEPT; }
    | beginord orderlines ORD_END_TOK EOF_TOK  { YYACCEPT; }
    | error EOF_TOK     {   fprintf(stderr, "Could not parse file.\n");
                            exit(-1);
                        }
    ;

beginconf : CONF_BEGIN_TOK  { bison_box->clear_game(); }
          ;

beginord : ORD_BEGIN_TOK
         ;

conflines
    :
    | conflines confline  
    ;

confline
    : command EOLN_TOK
    | error EOLN_TOK    {   fprintf(stderr,"syntax error\n");
                            exit(-1);
                        }
    ;

command
    : NAME                      { bison_box->add_region($1); }
    | NAME EQ NAME              {   int res = bison_box->add_abbrv($1,$3);
                                    if (res != 0) {
                                        fprintf(stderr, "Error on line: %s = %s (abbreviation already used)\n",$1,$3);
                                        exit(-1);
                                    }
                                }
    | NAME ADJ NAME             {   int res = bison_box->add_adj($1,$3);
                                    if (res != 0) {
                                        fprintf(stderr, "Error on line: %s - %s (cannot make region adjacent to itself)\n",$1,$3);
                                        exit(-1);
                                    }
                                }
    | NAME PAR NAME             {   int res = bison_box->add_par($1,$3);
                                    if (res != 0) {
                                        fprintf(stderr, "Error on line: %s -> %s (cannot make region parent of itself)\n", $1,$3);
                                        exit(-1);
                                    }
                                }
    | PLAYER_TOK NAME EMAIL     {   int res = bison_box->add_player($2,$3);
                                    if (res != 0) {
                                        fprintf(stderr, "Error on line: PLAYER %s %s (name or email already used)\n",$2,$3);
                                        exit(-1);
                                    }
                                }
    | NAME UNIT NAME            {   int res = bison_box->add_piece($1,$2[0],$3);
                                    if (res != 0) {
                                        fprintf(stderr,"Error on line: %s %c %s (player or location don't exist or occupied)\n",$1,$2[0],$3);
                                        exit(-1);
                                    }
                                }
    |
    ;




orderlines
    :
    | orderlines orderline
    ;

orderline 
    : order EOLN_TOK
    | error EOLN_TOK    {   fprintf(stderr, "syntax error\n");
                            exit(-1);
                        }
    ;

order
    : UNIT NAME HOLD        {   // hold
                                DiplomacyRegion *region = bison_box->get_region_by_name($2);
                                verify_region(region,$2);
                                DiplomacyPiece *piece = region->check_occupier();
                                verify_piece(piece,$2);
                                verify_type(piece,$1[0],$2);
                                verify_owner(piece,$2);
                                piece->clear_move();
                                region->set_hold();
                            }
    | UNIT NAME ADJ NAME    {   // move
                                DiplomacyRegion *region = bison_box->get_region_by_name($2);
                                DiplomacyRegion *target_region = bison_box->get_region_by_name($4);
                                verify_region(region,$2);
                                verify_region(target_region,$4);
                                DiplomacyPiece *piece = region->check_occupier();
                                verify_piece(piece,$2);
                                verify_type(piece,$1[0],$2);
                                verify_owner(piece,$2);
                                piece->clear_move();
                                target_region->add_attacker(piece);
                            }
    | UNIT NAME CONV UNIT NAME ADJ NAME     {   // set up a convoy
                                                DiplomacyRegion *region = bison_box->get_region_by_name($2);
                                                DiplomacyRegion *start_region = bison_box->get_region_by_name($5);
                                                DiplomacyRegion *end_region = bison_box->get_region_by_name($7);
                                                verify_region(region,$2);
                                                verify_region(start_region,$5);
                                                verify_region(end_region,$7);
                                                DiplomacyPiece *piece = region->check_occupier();
                                                verify_piece(piece,$2);
                                                verify_type(piece,$1[0],$2);
                                                verify_fleet(piece);
                                                verify_owner(piece,$2);
                                                DiplomacyPiece *convoyed = start_region->check_occupier();
                                                verify_piece(convoyed, $5);
                                                verify_type(convoyed,$4[0],$5);
                                                verify_army(convoyed);
                                                piece->clear_move();
                                                end_region->add_att_conv(piece,convoyed);
                                            }
    | UNIT NAME SUPP UNIT NAME  {   // support a defending piece
                                    DiplomacyRegion *region = bison_box->get_region_by_name($2);
                                    DiplomacyRegion *target_region = bison_box->get_region_by_name($5);
                                    verify_region(region,$2);
                                    verify_region(target_region,$5);
                                    DiplomacyPiece *piece = region->check_occupier();
                                    verify_piece(piece,$2);
                                    verify_type(piece,$1[0],$2);
                                    verify_owner(piece,$2);
                                    DiplomacyPiece *supported = target_region->check_occupier();
                                    verify_piece(supported,$5);
                                    verify_type(supported,$4[0],$5);
                                    piece->clear_move();
                                    target_region->add_def_supp(piece,supported);
                                }
    | UNIT NAME SUPP UNIT NAME ADJ NAME     {   // support an attack/move
                                                DiplomacyRegion *region = bison_box->get_region_by_name($2);
                                                DiplomacyRegion *start_region = bison_box->get_region_by_name($5);
                                                DiplomacyRegion *end_region = bison_box->get_region_by_name($7);
                                                verify_region(region,$2);
                                                verify_region(start_region,$5);
                                                verify_region(end_region,$7);
                                                DiplomacyPiece *piece = region->check_occupier();
                                                verify_piece(piece,$2);
                                                verify_type(piece,$1[0],$2);
                                                verify_owner(piece,$2);
                                                DiplomacyPiece *supported = start_region->check_occupier();
                                                verify_piece(supported,$5);
                                                verify_type(supported,$4[0],$5);
                                                piece->clear_move();
                                                end_region->add_att_supp(piece,supported);
                                            }
    | UNIT NAME SUPP NAME UNIT NAME     {   // support other player's defend/convoy/support
                                            DiplomacyRegion *region = bison_box->get_region_by_name($2);
                                            DiplomacyRegion *target_region = bison_box->get_region_by_name($5);
                                            verify_region(region,$2);
                                            verify_region(target_region,$5);
                                            DiplomacyPiece *piece = region->check_occupier();
                                            verify_piece(piece,$2);
                                            verify_type(piece,$1[0],$2);
                                            verify_owner(piece,$2);
                                            DiplomacyPiece *supported = target_region->check_occupier();
                                            verify_piece(supported,$5);
                                            verify_type(supported,$4[0],$5);
                                            piece->clear_move();
                                            target_region->add_def_supp(piece,supported);
                                        }
    | UNIT NAME SUPP NAME UNIT NAME ADJ NAME    {   // support another player's attack/move
                                                    DiplomacyRegion *region = bison_box->get_region_by_name($2);
                                                    DiplomacyRegion *start_region = bison_box->get_region_by_name($6);
                                                    DiplomacyRegion *end_region = bison_box->get_region_by_name($8);
                                                    verify_region(region,$2);
                                                    verify_region(start_region,$6);
                                                    verify_region(end_region,$8);
                                                    DiplomacyPiece *piece = region->check_occupier();
                                                    verify_piece(piece,$2);
                                                    verify_type(piece,$1[0],$2);
                                                    verify_owner(piece,$2);
                                                    DiplomacyPiece *supported = start_region->check_occupier();
                                                    verify_piece(supported,$6);
                                                    verify_type(supported,$5[0],$6);
                                                    piece->clear_move();
                                                    end_region->add_att_supp(piece,supported); }
    |
    ;
%%

int yyerror (char *msg) { }

void verify_region(DiplomacyRegion *region, char *region_name) {
    if (region == NULL) {
        fprintf(stderr, "Error: region '%s' does not exist\n", region_name);
        exit(-1);
    }
}

void verify_piece(DiplomacyPiece *piece, char *region_name) {
    if (piece == NULL) {
        fprintf(stderr, "Error: region '%s' is not occupied\n", region_name);
        exit(-1);
    }
}

void verify_type(DiplomacyPiece *piece, char label, char *region_name) {
    if (piece->check_type() != (label == 'F')) {
        fprintf(stderr, "Error: incorrect unit type in region %s\n",region_name);
        exit(-1);
    }
}

void verify_fleet(DiplomacyPiece *piece) {
    if (piece->check_type() != 1) {
        fprintf(stderr, "Error: only fleets can convoy armies\n");
        exit(-1);
    }
}

void verify_army(DiplomacyPiece *piece) {
    if (piece->check_type() != 0) {
        fprintf(stderr, "Error: only armies can be convoyed\n");
        exit(-1);
    }
}

// TODO: implement (will need some global variable that stores the person issuing the orders)
void verify_owner(DiplomacyPiece *piece, char *region_name) {
    if (!true) {
        fprintf(stderr, "Error: you do not own the unit in region %s\n",region_name);
        exit(-1);
    }
}