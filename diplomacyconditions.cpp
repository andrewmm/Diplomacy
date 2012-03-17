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

DiplomacyGame *AndCondition::pass() {
	DiplomacyGame *acc = NULL;
	for (int i = 0; i < terms.size(); ++i) {
		DiplomacyGame *step = terms[i].pass();
		if (step != NULL || acc == NULL) {
			acc = step;
		}
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

DiplomacyGame *OrCondition::pass() {
	DiplomacyGame *acc = NULL;
	bool changed = false;
	for (int i = 0; i < factors.size(); ++i) {
		DiplomacyGame *step = factors[i].pass();
		if (!changed) {
			acc = step;
			changed = true;
		}
		if (acc != NULL) {
			acc = step;
		}
		if (acc == NULL && changed) {
			return acc;
		}
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
	else if (cond_type == support_condition && old.supp_cond.size() > 0) {
		supp_cond.resize(1,SupportCondition(old.supp_cond[0], newgame));
	}
	else if (cond_type == moved_condition && old.moved_cond.size() > 0) {
		moved_cond.resize(1,MovedCondition(old.moved_cond[0],newgame));
	}
	else if (cond_type == no_trade_condition && old.no_trade_cond.size() > 0) {
		no_trade_cond.resize(1,NoTradeCondition(old.no_trade_cond[0],newgame));
	}
	else if (cond_type == not_dislodged_condition && old.not_disl_cond.size() > 0) {
		not_disl_cond.resize(1,NotDislodgedByCondition(old.not_disl_cond[0],newgame));
	}
	else {
		fprintf(stderr,"ERROR: Asked to copy ConditionBox of unknown type.\n");
	}
}

DiplomacyGame *ConditionBox::pass() {
	if (cond_type == convoy_condition && convoy_cond.size() > 0) {
		return convoy_cond[0].pass();
	}
	else if (cond_type == support_condition && supp_cond.size() > 0) {
		return supp_cond[0].pass();
	}
	else if (cond_type == moved_condition && moved_cond.size() > 0) {
		return moved_cond[0].pass();
	}
	else if (cond_type == no_trade_condition && no_trade_cond.size() > 0) {
		return no_trade_cond[0].pass();
	}
	else if (cond_type == not_dislodged_condition && not_disl_cond.size() > 0) {
		return not_disl_cond[0].pass();
	}
	else {
		fprintf(stderr,"ERROR: unknown condition type in the box.\n");
		return NULL;
	}
}

ConvoyCondition::ConvoyCondition(DiplomacyGame *gamep, DiplomacyPiece *forpiecep, DiplomacyRegion *fromp, DiplomacyRegion *top) {
	game = gamep;
	alternate = game->check_alternate();
	forpiece = forpiecep;
	from = fromp;
	to = top;
}

ConvoyCondition::ConvoyCondition(const ConvoyCondition& old, DiplomacyGame *newgame) {
	game = newgame;
	alternate = old.alternate;
	forpiece = game->find_copied_piece(old.forpiece);
	from = game->get_region_by_name(old.from->check_names()[0]);
	to = game->get_region_by_name(old.to->check_names()[0]);
}

DiplomacyGame *ConvoyCondition::pass() {
	std::vector<DiplomacyRegion *> steps;
	std::vector<convoy *> convoybox = to->check_convoys();
	for (int i = 0; i < convoybox.size(); ++i) {
		if (convoybox[i]->convoyed == forpiece) {
			steps.push_back(convoybox[i]->convoyer->check_location());
		}
	}
	bool res = game->is_sea_path(from, to, steps);
	if (!res) {
		fprintf(stderr,"FAILED ConvoyCondition from %s to %s.\n",from->check_names()[0],to->check_names()[0]);
		return alternate;
	}
	return NULL;
}

SupportCondition::SupportCondition(DiplomacyGame *gamep, DiplomacyRegion *inp, DiplomacyPiece *forp, DiplomacyPiece *fromp) {
	game = gamep;
	alternate = game->check_alternate();
	in = inp;
	forpiece = forp;
	frompiece = fromp;
}

SupportCondition::SupportCondition(const SupportCondition& old, DiplomacyGame *newgame) {
	game = newgame;
	alternate = old.alternate;
	in = game->get_region_by_name(old.in->check_names()[0]);
	forpiece = game->find_copied_piece(old.forpiece);
	frompiece = game->find_copied_piece(old.frompiece);
}

DiplomacyGame *SupportCondition::pass() {
	bool acc = false;
	std::vector<support *> att_support = in->check_att_support();
	std::vector<support *> def_support = in->check_def_support();
	for (int i = 0; i < att_support.size(); ++i) {
		acc = acc || (att_support[i]->supported == forpiece && att_support[i]->supporter == frompiece);
	}
	for (int i = 0; i < def_support.size(); ++i) {
		acc = acc || (def_support[i]->supported == forpiece && def_support[i]->supporter == frompiece);
	}
	if (!acc) {
		fprintf(stderr,"FAILED SupportCondition for %s's piece %d from %s's piece %d.\n", forpiece->check_owner()->check_name(),
			forpiece->check_self_num(),frompiece->check_owner()->check_name(),frompiece->check_self_num());
		return alternate;
	}
	return NULL;
}

MovedCondition::MovedCondition(DiplomacyGame *gamep, DiplomacyPiece *piecep, DiplomacyRegion *fromp) {
	game = gamep;
	alternate = game->check_alternate();
	piece = piecep;
	from = fromp;
}

MovedCondition::MovedCondition(const MovedCondition& old, DiplomacyGame *newgame) {
	game = newgame;
	alternate = old.alternate;
	piece = game->find_copied_piece(old.piece);
	from = game->get_region_by_name(old.from->check_names()[0]);
}

DiplomacyGame *MovedCondition::pass() {
	bool res = piece->check_location() != from;
	if (!res) {
		fprintf(stderr,"FAILED MovedCondition: %s's piece %d did not leave %s.\n",piece->check_owner()->check_name(),
			piece->check_self_num(),from->check_names()[0],alternate);
		return alternate;
	}
	return NULL;
}

NoTradeCondition::NoTradeCondition(DiplomacyGame *gamep, DiplomacyPiece *piece1p, DiplomacyPiece *piece2p) {
	game = gamep;
	alternate = game->check_alternate();
	piece1 = piece1p;
	reg1 = piece1->check_location();
	piece2 = piece2p;
	reg2 = piece2->check_location();
}

NoTradeCondition::NoTradeCondition(const NoTradeCondition& old, DiplomacyGame *newgame) {
	game = newgame;
	alternate = old.alternate;
	piece1 = game->find_copied_piece(old.piece1);
	reg1 = game->get_region_by_name(old.reg1->check_names()[0]);
	piece2 = game->find_copied_piece(old.piece2);
	reg2 = game->get_region_by_name(old.reg2->check_names()[0]);
}

DiplomacyGame *NoTradeCondition::pass() {
	DiplomacyRegion *par1 = reg1;
	if (reg1->check_parent() != NULL) {
		par1 = reg1->check_parent();
	}
	DiplomacyRegion *par2 = reg2;
	if (reg2->check_parent() != NULL) {
		par2 = reg2->check_parent();
	}
	bool p1trade = piece1->check_location() == par2 || piece1->check_location()->check_parent() == par2;
	bool p2trade = piece2->check_location() == par1 || piece2->check_location()->check_parent() == par1;
	if (p1trade && p2trade) {
		fprintf(stderr,"FAILED NoTradeCondition between %s and %s.\n",par1->check_names()[0],par2->check_names()[0]);
		return alternate;
	}
	return NULL;
}

NotDislodgedByCondition::NotDislodgedByCondition(DiplomacyGame *gamep, DiplomacyPiece *piecep, DiplomacyRegion *byp) {
	game = gamep;
	alternate = game->check_alternate();
	piece = piecep;
	by = byp;
}

NotDislodgedByCondition::NotDislodgedByCondition(const NotDislodgedByCondition& old, DiplomacyGame *newgame) {
	game = newgame;
	alternate = old.alternate;
	piece = game->find_copied_piece(old.piece);
	by = game->get_region_by_name(old.by->check_names()[0]);
}

DiplomacyGame *NotDislodgedByCondition::pass() {
	fprintf(stderr,"Checking NotDislodgedByCondition\n",piece);
	bool acc = false;
	std::vector<dislodgment *> dislodgments = game->check_dislodgments();
	for (int i = 0; i < dislodgments.size(); ++i) {
		bool found_coast = false;
		for (int j = 0; j < by->check_coasts().size(); ++j) {
			found_coast = found_coast || by->check_coasts()[j] == dislodgments[i]->by;
		}
		acc = acc || (dislodgments[i]->dislodged == piece && (dislodgments[i]->by == by || dislodgments[i]->by == by->check_parent()
			|| found_coast)) ;
	}
	if (acc) {
		fprintf(stderr,"FAILED NotDislodgedByCondition: %s's unit %d dislodged by piece in %s.\n",piece->check_owner()->check_name(),
			piece->check_self_num(),by->check_names()[0]);
		return alternate;
	}
	return NULL;
}