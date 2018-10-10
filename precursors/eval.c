/*
	eval.c
	Simple unsigned arithmetic evaluator. Used to process precursor scripts.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

static char sym[] = "+-*/^<>|&)(", ops[256], tok[256];
static int opp, asp, par, sta = 0;
static uint64_t arg[256];

static void opsh(char), apsh(uint64_t);
static int apop(uint64_t*), opop(int*);
static char *eget(char*), *oget(char*);
static int oeva(), peva();

int eval(char* str, uint64_t* val)
{
	int r;
	uint64_t a;
	char *p, *s, *e, *t = str;
	str = strupr(str);
	for (; *t; ++t) { p = t; while (*p && isspace(*p)) ++p; if (t != p) strcpy(t, p); }
	p = str;
	while (*p)
	{
		if (sta == 0)
		{
			if (NULL != (s = eget(p)))
			{
				if ('(' == *s) { opsh(*s), p += strlen(s); break; }
				if (0ULL == (a = strtoul(s, &e, 10)) && NULL == strchr(s, '0')) return -1;
				apsh(a);
				p += strlen(s);
			}
			else return -1;
			sta = 1;
			break;
		}
		else
		{
			if (NULL == (s = oget(p))) return -1;
			if (strchr(sym, *s))
			{
				if (')' == *s) { if (0 > (r = peva())) return r; }
				else opsh(*s), sta = 0; 
				p += strlen(s);
			}
			else return -1;
			break;
		}
	}
	while (1 < asp) if (0 > (r = oeva())) return r;
	if (!opp) return apop(val);
	else return -1;
}

static int peva() { int o; if (1 > par--) return -1; do if (0 > (o = oeva())) break; while ('(' != o); return o; }
static void opsh(char o) { if ('(' == o) ++par; ops[opp++] = o; }
static void apsh(uint64_t a) { arg[asp++] = a; }
static int apop(uint64_t* a) { *a = arg[--asp]; return (0 > asp) ? -1 : 0; }
static int opop(int* o) { if (!opp) return -1; *o = ops[--opp]; return 0; }
static char* oget(char* s) { *tok = *s; tok[1] = '\0'; return tok; }

static char* eget(char* s)
{
	char *p = s, *t = tok;
	while (*p)
	{
		if (strchr(sym, *p))
		{
			if ('-' == *p) { if (s != p && 'E' != p[-1]) break; }
			else if (s == p) return oget(s);
			else if ('E' == *p) { if (!isdigit(p[1]) && '-' != p[1]) return NULL; }
			else break;
		}
		*t++ = *p++;
	}
	*t = '\0';
	return tok;
}

static int oeva()
{
	uint64_t a, b;
	int o;
	if (-1 == opop(&o)) return -1;
	apop(&a);
	apop(&b);
	switch (o)
	{
	case '+': apsh(b + a); break;
	case '-': apsh(b - a); break;
	case '*': apsh(b * a); break;
	case '/': apsh(b / a); break;
	case '<': apsh(b << a); break;
	case '>': apsh(b >> a); break;
	case '|': apsh(b | a); break;
	case '&': apsh(b & a); break;
	case '^': apsh(b ^ a); break;
	case '(': asp += 2; break;
	default: return -1;
	}
	if (1 > asp) return -1;
	else return o;
}

