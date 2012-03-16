// diplomacyconditions.cpp
// Class methods for Condition classes

// Andrew MacKie-Mason
// CMSC 162, University of Chicago

// Project site: http://brick.cs.uchicago.edu/Courses/CMSC-16200/2012/pmwiki/pmwiki.php/Student/Diplomacy

#include "diplomacy.h"

AndCondition::AndCondition() {
	game = NULL;
}

AndCondition::AndCondition(DiplomacyGame *gamep) {
	game = gamep;
}

AndCondition::AndCondition(const AndCondition& old, DiplomacyGame *newgame) {
	game = newgame;
	for (int i = 0; i < old.terms.size(); ++i) {
		terms.push_back(OrCondition(old.terms[i],newgame));
	}
}

bool AndCondition::pass() {
	bool acc = true;
	for (int i = 0; i < terms.size(); ++i) {
		acc = acc && terms[i].pass();
	}
	return acc;
}

OrCondition::OrCondition(DiplomacyGame *gamep) {
	game = gamep;
}

OrCondition::OrCondition(const OrCondition& old, DiplomacyGame *newgame) {
	game = newgame;
	for (int i = 0; i < old.factors.size(); ++i) {
		factors.push_back(ConditionBox(old.factors[i],newgame));
	}
}

bool OrCondition::pass() {
	bool acc = factors.empty();
	for (int i = 0; i < factors.size(); ++i) {
		acc = acc || factors[i].pass();
	}
	return acc;
}

ConditionBox::ConditionBox(DiplomacyGame *gamep, conditiontype ctype) {
	game = gamep;
	cond_type = ctype;
}

ConditionBox::ConditionBox(const ConditionBox& old, DiplomacyGame *newgame) {
	game = newgame;
	cond_type = old.cond_type;
	if (cond_type == convoy_condition && old.convoy_cond.size() > 0) {
		convoy_cond.resize(1,ConvoyCondition(old.convoy_cond[0], newgame));
	}
	else if (cond_type == moved_condition && old.moved_cond.size() > 0) {
		moved_cond.resize(1,MovedCondition(old.moved_cond[0],newgame));
	}
	else {
		fprintf(stderr,"ERROR: Asked to create ConditionBox of unknown type.\n");
	}
}

bool ConditionBox::pass() {
	if (cond_type == convoy_condition && convoy_cond.size() > 0) {
		return convoy_cond[0].pass();
	}
	else if (cond_type == moved_condition && moved_cond.size() > 0) {
		return moved_cond[0].pass();
	}
	return false;
}

ConvoyCondition::ConvoyCondition(DiplomacyGame *gamep, DiplomacyPiece *forpiecep, DiplomacyRegion *fromp, DiplomacyRegion *top) {
	game = gamep;
	forpiece = forpiecep;
	from = fromp;
	to = top;
}

ConvoyCondition::ConvoyCondition(const ConvoyCondition& old, DiplomacyGame *newgame) {
	game = newgame;

	char *ownername = old.forpiece->check_owner()->check_name();
	DiplomacyPlayer *owner = game->get_player_by_num(game->get_player_num_by_name(ownername));
	forpiece = owner->check_pieces()[old.forpiece->check_self_num()];

	char *fromname = old.from->check_names()[0];
	from = game->get_region_by_name(fromname);

	char *toname = old.to->check_names()[0];
	to = game->get_region_by_name(toname);
}

bool ConvoyCondition::pass() {
	std::vector<DiplomacyRegion *> steps;
	std::vector<convoy *> convoybox = to->check_convoys();
	for (int i = 0; i < convoybox.size(); ++i) {
		if (convoybox[i]->convoyed == forpiece) {
			steps.push_back(convoybox[i]->convoyer->check_location());
		}
	}
	return game->is_sea_path(from, to, steps);
}

MovedCondition::MovedCondition(DiplomacyGame *gamep, DiplomacyPiece *piecep, DiplomacyRegion *fromp) {
	game = gamep;
	piece = piecep;
	from = fromp;
}

MovedCondition::MovedCondition(const MovedCondition& old, DiplomacyGame *newgame) {
	game = newgame;
	piece = game->get_player_by_num(game->get_player_num_by_name(old.piece->check_owner()->check_name()))->
																				check_pieces()[old.piece->check_self_num()];
	from = game->get_region_by_name(old.from->check_names()[0]);
}

bool MovedCondition::pass() {
	return piece->check_location() != from;
}