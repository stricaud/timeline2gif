%define api.prefix {t2g}
%param {t2g_t **timeline}

%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "t2g.h"	
#include <parser.h>

int t2glex(); 
extern void t2gerror(t2g_t **t2g, const char *p);
	
#define T2G_ABORT return -1;
#define T2GERROR_VERBOSE

%}

%union {
	char *string;
	int integer;
}

%token <string>TOK_STRING
%token <integer>TOK_INTEGER
%token <string>TOK_WORD
%token TOK_DOT
%token <string>TOK_ARGB

%start input
%%

input:
      | input setting_str
      | input setting_argb
      | input setting_int
      | input item
      ;

setting_str: TOK_WORD TOK_DOT TOK_WORD TOK_WORD
      {
	      printf("Setting element:%s.%s [%s]\n", $1, $3, $4);
	      free($1);
	      free($3);
	      free($4);
      }
      ;

setting_argb: TOK_WORD TOK_DOT TOK_WORD TOK_ARGB
      {
	      printf("Setting HTML element:%s.%s [%s]\n", $1, $3, $4);
	      free($1);
	      free($3);
	      free($4);
      }
      ;

setting_int: TOK_WORD TOK_DOT TOK_WORD TOK_INTEGER
      {
	      free($1);
	      free($3);

      }
      ;

item: TOK_STRING TOK_STRING
       {
	       printf("key:%s;val:%s\n", $1, $2);
	       free($1);
	       free($2);
       }
       ;


%%

char *t2gget_text(void);
int t2gget_lineno (void);

void t2gerror(t2g_t **t2g, const char *str)
{
	fprintf(stderr, "Parsing error: invalid token '%s' line %d: %s\n", t2gget_text(), t2gget_lineno()+1, str);
	exit(1);
}
