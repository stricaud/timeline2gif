%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <t2g.h>
#include <parser.h>

void t2gerror (const char *s);

#define YY_EXTRA_TYPE struct t2g_t *

/* ── Heredoc capture for `define.svg name << TERM … TERM` ───────────────── */
static char   hd_name[128];
static char   hd_term[128];
static char  *hd_buf = NULL;
static size_t hd_len = 0, hd_cap = 0;

static void hd_reset(void)
{
        free(hd_buf);
        hd_buf = NULL;
        hd_len = hd_cap = 0;
}

static void hd_append(const char *s, size_t n)
{
        if (hd_len + n + 1 > hd_cap) {
                hd_cap = (hd_len + n + 1) * 2;
                hd_buf = realloc(hd_buf, hd_cap);
        }
        memcpy(hd_buf + hd_len, s, n);
        hd_len += n;
        hd_buf[hd_len] = '\0';
}

/* Copy a whitespace-delimited token from `src` into `dst` (size `cap`). */
static void hd_token(const char *src, char *dst, size_t cap)
{
        while (*src == ' ' || *src == '\t') src++;
        size_t i = 0;
        while (*src && *src != ' ' && *src != '\t' && *src != '<' &&
               *src != '\r' && *src != '\n' && i < cap - 1)
                dst[i++] = *src++;
        dst[i] = '\0';
}

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

%x HEREDOC

VOID [ \t]+
ARGB argb\([0-9]+,[0-9]+,[0-9]+,[0-9]+\)
COMMENT #.*\n
DOT \.
WORD [a-zA-Z][a-zA-Z0-9\-_]*
STRING (\"|\')([^\\(\"|\')]|\\.)*(\"|\')

%%

 /* define.svg <name> << TERM  — capture following lines until a line == TERM */
define\.svg{VOID}{WORD}[ \t]*<<[ \t]*{WORD}[ \t]*\r?\n {
        hd_token(t2gtext + 10, hd_name, sizeof(hd_name));   /* after "define.svg" */
        char *p = strstr(t2gtext, "<<");
        hd_token(p ? p + 2 : "", hd_term, sizeof(hd_term));
        hd_reset();
        t2gset_lineno(t2gget_lineno() + 1);
        BEGIN(HEREDOC);
}

<HEREDOC>[^\n]*\n {
        /* Trim surrounding whitespace to test for the terminator line. */
        int len = (int)strlen(t2gtext), n = len, start = 0;
        while (n > 0 && (t2gtext[n-1]=='\n' || t2gtext[n-1]=='\r' ||
                         t2gtext[n-1]==' '  || t2gtext[n-1]=='\t')) n--;
        while (start < n && (t2gtext[start]==' ' || t2gtext[start]=='\t')) start++;
        char line[256];
        int m = n - start; if (m > 255) m = 255;
        memcpy(line, t2gtext + start, m); line[m] = '\0';

        t2gset_lineno(t2gget_lineno() + 1);
        if (strcmp(line, hd_term) == 0) {
                t2g_define_svg(hd_name, hd_buf ? hd_buf : "");
                hd_reset();
                BEGIN(INITIAL);
        } else {
                hd_append(t2gtext, (size_t)len);
        }
}

<HEREDOC><<EOF>> {
        /* Unterminated block: register what we have and stop. */
        if (hd_buf) t2g_define_svg(hd_name, hd_buf);
        hd_reset();
        BEGIN(INITIAL);
        yyterminate();
}

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

