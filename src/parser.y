%define api.prefix {t2g}
%param {t2g_t *timeline}

%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "t2g.h"	
#include <parser.h>
#include <regex.h>

int t2glex(); 
extern void t2gerror(t2g_t *t2g, const char *p);

regex_t rex_argb;
t2g_t *t2g_current = NULL;

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

	      if (!strcmp($1, "time")) {
		      if (!strcmp($3, "font")) {
			      if (!t2g_current) {
				      t2g_current = t2g_new();
			      }	       
			      t2g_current->time_font = strdup($4);
		      }
	      }
	      if (!strcmp($1, "description")) {
		      if (!strcmp($3, "font")) {
			      if (!t2g_current) {
				      t2g_current = t2g_new();
			      }	       
			      t2g_current->description_font = strdup($4);
		      }
	      }
	      
	      free($1);
	      free($3);
	      free($4);
      }
      ;

setting_argb: TOK_WORD TOK_DOT TOK_WORD TOK_ARGB
      {
	      regmatch_t pmatch[5];
	      int retval;
	      char A[4] = {0,0,0,0};
	      char R[4] = {0,0,0,0};
	      char G[4] = {0,0,0,0};
	      char B[4] = {0,0,0,0};
	      unsigned char a,r,g,b;
	      
	      retval = regexec(&rex_argb, $4, 5, pmatch, 0);
	      if (retval) {
		      fprintf(stderr, "Unexpected no matching for argb!\n");
		      goto out;
	      }

	      for (int i=1;i<=4;i++) {
		      if ((pmatch[i].rm_eo - pmatch[i].rm_so) > 3) {
			      fprintf(stderr,"Error parsing number at line %d. Greater than expected!\n", t2gget_lineno());
			      goto out;
		      }
		      switch(i) {
		      case 1:
			      memcpy(&A, $4 + pmatch[i].rm_so, pmatch[i].rm_eo - pmatch[i].rm_so);
			      /* printf("A=%s\n", A); */
			      break;
		      case 2:
			      memcpy(&R, $4 + pmatch[i].rm_so, pmatch[i].rm_eo - pmatch[i].rm_so);
			      /* printf("R=%s\n", R); */
			      break;
		      case 3:
			      memcpy(&G, $4 + pmatch[i].rm_so, pmatch[i].rm_eo - pmatch[i].rm_so);
			      /* printf("G=%s\n", G); */
			      break;
		      case 4:
			      memcpy(&B, $4 + pmatch[i].rm_so, pmatch[i].rm_eo - pmatch[i].rm_so);
			      /* printf("B=%s\n", B); */
			      break;
		      default:
			      fprintf(stderr, "Uh? Should not be there!\n");
			      goto out;
			      break;
		      }
	      }
	      a = (unsigned char)strtol(A, NULL, 10);
	      r = (unsigned char)strtol(R, NULL, 10);
	      g = (unsigned char)strtol(G, NULL, 10);
	      b = (unsigned char)strtol(B, NULL, 10);

	      if (!strcmp($3, "color")) {
		      if (!strcmp($1, "timeline")) {
			      /* printf("set timeline\n"); */
		      } else if (!strcmp($1, "mark")) {
			      /* printf("set mark\n"); */
		      } else {
			      fprintf(stderr, "ERR[%d]: unknown object %s\n", t2gget_lineno(), $1);
		      }
	      } else {
		      fprintf(stderr, "ERR[%d]: argb() function must be set for a color definition\n", t2gget_lineno());
	      }
	      
      out:
	      free($1);
	      free($3);
	      free($4);
      }
      ;

setting_int: TOK_WORD TOK_DOT TOK_WORD TOK_INTEGER
      {
	      if (!strcmp($1, "speed")) {
		      if (!strcmp($3, "nextitem")) {
			      if (!t2g_current) {
				      t2g_current = t2g_new();
			      }	       
			      t2g_current->speed_nextitem = $4;
		      }
		      if (!strcmp($3, "frames")) {
			      if (!t2g_current) {
				      t2g_current = t2g_new();
			      }	       
			      t2g_current->speed_frames = $4;
		      }
	      }

	      if (!strcmp($1, "time")) {
		      if (!strcmp($3, "font_size")) {
			      if (!t2g_current) {
				      t2g_current = t2g_new();
			      }	       
			      t2g_current->time_font_size = $4;
		      }
	      }
	      if (!strcmp($1, "description")) {
		      if (!strcmp($3, "font_size")) {
			      if (!t2g_current) {
				      t2g_current = t2g_new();
			      }	       
			      t2g_current->description_font_size = $4;
		      }
	      }

	      
	      free($1);
	      free($3);
      }
      ;

item: TOK_STRING TOK_STRING
       {
	       if (!t2g_current) {
		       t2g_current = t2g_new();
	       }	       
	       t2g_current->time_text = strdup($1);
	       t2g_current->label_text = strdup($2);

	       t2g_append(timeline, t2g_current);
	       t2g_current = NULL;
	       
	       free($1);
	       free($2);
       }
       ;


%%

void t2g_parser_init(void)
{
	int retval;
	
	retval = regcomp(&rex_argb, "argb\\(([0-9]+),([0-9]+),([0-9]+),([0-9]+)\\)", REG_EXTENDED);
	if (retval) {
		fprintf(stderr, "Cannot compile regex\n");
	}

}

void t2g_parser_terminate(void)
{
	regfree(&rex_argb);
}

char *t2gget_text(void);
int t2gget_lineno (void);

void t2gerror(t2g_t *t2g, const char *str)
{
	fprintf(stderr, "Parsing error: invalid token '%s' line %d: %s\n", t2gget_text(), t2gget_lineno()+1, str);
	exit(1);
}
