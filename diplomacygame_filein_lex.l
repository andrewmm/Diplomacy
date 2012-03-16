%{
    // diplomacygame_filein_lex.l

    // Andrew MacKie-Mason
    // CMSC 162, University of Chicago
    
    // Project Site: http://brick.cs.uchicago.edu/Courses/CMSC-16200/2012/pmwiki/pmwiki.php/Student/Diplomacy

    #include <stdio.h>
    #include <stdlib.h>
    #include "diplomacy.h"
    #include "diplomacygame_filein_bison.tab.h"

    int yyparse();
%}

%option noyywrap

name [a-zA-Z0-9_]+
email [^ \t@.\n]+@[^ \t@.\n]+"."[^ \t@.\n]+

%x CONF_STATE
%x ORD_STATE
%x DONE_STATE

%%

#!BEGIN-CONF    {   BEGIN CONF_STATE;
                    return CONF_BEGIN_TOK;
                }
#!BEGIN-ORD     {   BEGIN ORD_STATE;
                    return ORD_BEGIN_TOK;
                }

<INITIAL>.|\n {}

<CONF_STATE,ORD_STATE>{
#!END-CONF      {   BEGIN DONE_STATE;
                    return CONF_END_TOK;
                }
#!END-ORD       {   BEGIN DONE_STATE;
                    return ORD_END_TOK;
                }

A|F     {   yylval.word = strdup(yytext);
            return UNIT;
        }
S       { return SUPP; }
C       { return CONV; }
->      { return PAR; }
-       { return ADJ; }
=       { return EQ; }

[Hh][Oo][Ll][Dd][Ss]? { return HOLD; }
PLAYER  { return PLAYER_TOK; }

{name}  {   yylval.word = strdup(yytext);
            return NAME;
        }
{email} {   yylval.word = strdup(yytext);
            return EMAIL;
        }


[ \t] { /* skip blanks */ }
\n  { return EOLN_TOK; }
.   { return ERROR; }
}

<DONE_STATE>.|\n {}
<DONE_STATE><<EOF>> {   BEGIN INITIAL; // set up for next call to lexer
                        return EOF_TOK;
                    }

<<EOF>> {return ERROR; }

%%