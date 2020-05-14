%{
#include <stdio.h>
#include <t2g.h>
#include <parser.h>

void t2gerror (const char *s);

#define YY_EXTRA_TYPE struct t2g_t *


static char *escape_str(char *str)
{
        size_t w = 0, i = 0;
        int escape_next = 0;

        for ( i = 1; str[i]; i++ ) {

                if ( ! escape_next && str[i] == '\\' ) {
                        escape_next = 1;
                        continue;
                }

                str[w++] = str[i];
                escape_next = 0;
        }

        str[w - 1] = '\0';
        return str;
}

%}

%option noyywrap
%option 8bit prefix="t2g"


VOID [ \t]+
ARGB argb\([0-9]+,[0-9]+,[0-9]+,[0-9]+\)
COMMENT #.*\n
DOT \.
WORD [a-zA-Z0-9\-_]+
STRING (\"|\')([^\\(\"|\')]|\\.)*(\"|\')

%%

{VOID} {}

{DOT} { return TOK_DOT; }

{WORD} { t2glval.string = strdup(t2gtext);
         return TOK_WORD;
       }

{ARGB} { t2glval.string = strdup(t2gtext);
             return TOK_ARGB;
           }
{STRING} {
           t2glval.string = strdup(escape_str(t2gtext));
           return TOK_STRING;
         }
[0-9]+   {
           t2glval.integer = (int)strtol(t2gtext, NULL, 10);
           return TOK_INTEGER;
         }

{COMMENT} {}


\n { t2gset_lineno(t2gget_lineno()+1); }

%%

