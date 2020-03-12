%{
#include <stdio.h>
#include <parser.h>

void t2gerror (const char *s);

#define YY_EXTRA_TYPE struct t2g_t *

%}

%option noyywrap
%option 8bit prefix="t2g"


VOID [ \t]+
COMMENT #.*\n
DOT \.
WORD [a-zA-Z0-9]

%%

{VOID} {}
{COMMENT} {}

{DOT} { return TOK_DOT; }

{WORD} { t2glval.string = strdup(t2gtext);
         return TOK_WORD;
       }

\"(.*)\" {
           t2glval.string = strdup(t2gtext);
           return TOK_STRING;
         }
[0-9]+   {
           t2glval.integer = (int)strtol(t2gtext, NULL, 10);
           return TOK_INTEGER;
         }

\n { t2gset_lineno(t2gget_lineno()+1); }

%%

