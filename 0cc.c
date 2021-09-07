#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

typedef enum {
	TK_RESERVED, //keywords or punctuators
	TK_NUM,		 //Integer literals
	TK_EOF,		 //End of file markers
} Tokenkind;


typedef struct Token Token; //Token type
struct Token {
	Tokenkind kind; //Token kind
	Token *next;	//Next token
	int val;		//if kind is TK_NUM
	char *str;		//Token string
};

Token *token; //Current token


void error(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

//If the next token is the expected symbol, read the token forward by one and retrun true
bool consume(char op)
{
	if (token->kind != TK_RESERVED || token->str[0] != op)
		return false;
	token = token->next;
	return true;
}

//If the next token is the expected symbol, read the token forward by one.
void expect(char op)
{
	if (token->kind != TK_RESERVED || token->str[0] != op)
		error("expected '%c'", op);
	token = token->next;
}

//If the next token is a number, read the token one more time and return that number.
int expect_number()
{
	if (token->kind != TK_NUM)
		error("expected a number");
	int val = token->val;
	token = token->next;
	return val;
}

bool at_eof()
{
	return token->kind == TK_EOF;
}

Token *new_token(Tokenkind kind, Token *cur, char *str)
{
	Token *tok = calloc(1, sizeof(Token));
	tok->kind = kind;
	tok->str = str;
	cur->next = tok;
	return tok;
}

Token *tokenize(char *p)
{
	Token head;
	head.next = NULL;
	Token *cur = &head;

	while (*p)
	{
		if (isspace(*p)) //Skip whitespace characters. 
		{
			p++;
			continue ;
		}

		if (*p == '+' || *p == '-')
		{
			cur = new_token(TK_RESERVED, cur, p++);
			continue ;
		}

		if (isdigit(*p))
		{
			cur = new_token(TK_NUM, cur, p);
			cur->val = strtol(p, &p, 10);
			continue;
		}

		error("invalid token");
	}

	new_token(TK_EOF, cur, p);
	return head.next;
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		error("%s: invalid arguments", argv[0]);
		return 1;
	}

	token = tokenize(argv[1]);

	printf(".intel_syntax noprefix\n");
	printf(".globl main\n");
	printf("main:\n");
	
	//The first token must be a number
	printf(" mov rax, %d\n", expect_number());

	
	while (!at_eof())
	{
		if (consume('+'))
		{
			printf("  add rax, %d\n", expect_number());
			continue ;
		}

		expect('-');
		printf("  sub rax, %d\n", expect_number());
	}

	printf("  ret\n");
	return 0;
}
