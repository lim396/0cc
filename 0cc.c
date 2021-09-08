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

char *user_input; //Input program

Token *token; //Current token


void error(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

//Reports an error location
void error_at(char *loc, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	int pos = loc - user_input;
	fprintf(stderr, "%s\n", user_input);
	fprintf(stderr, "%*s", pos, " ");
	fprintf(stderr, "^ ");
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
		error_at(token->str,"expected '%c'", op);
	token = token->next;
}

//If the next token is a number, read the token one more time and return that number.
int expect_number()
{
	if (token->kind != TK_NUM)
		error_at(token->str, "expected a number");
	int val = token->val;
	token = token->next;
	return val;
}

bool at_eof()
{
	return token->kind == TK_EOF;
}

//Create a new token and connect it to 'cur'
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

		if (strchr("+-*/()", *p)) //Punctuator
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

		error_at(p, "invalid token");
	}

	new_token(TK_EOF, cur, p);
	return head.next;
}

typedef enum {
	ND_ADD, // +
	ND_SUB, // -
	ND_MUL, // *
	ND_DIV, // /
	ND_NUM, // Integer
} Nodekind;

typedef struct Node Node;
struct Node {
	Nodekind kind;
	Node *lhs;
	Node *rhs;
	int val;
};

Node *new_node(Nodekind kind)
{
	Node *node = calloc(1, sizeof(Node));
	node->kind = kind;
	return node;
}

Node *new_binary(Nodekind kind, Node *lhs, Node *rhs)
{
	Node *node = new_node(kind);
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

Node *new_num(int val)
{
	Node *node = new_node(ND_NUM);
	node->val = val;
	return node;
}

Node *mul();
Node *primary();

// expr = mul ("+" mul | "-" mul)*
Node *expr()
{
	Node *node = mul();

	for (;;)
	{
		if (consume('+'))
			node = new_binary(ND_ADD, node, mul());
		else if (consume('-'))
			node = new_binary(ND_SUB, node, mul());
		else
			return node;
	}
}

// mul = primary ("*" primary | "/" primary)*
Node *mul()
{
	Node *node = primary();


	for (;;)
	{
		if (consume('*'))
			node = new_binary(ND_ADD, node, primary());
		else if (consume('/'))
			node = new_binary(ND_SUB, node, primary());
		else
			return node;
	}
}

// primary = "(" expr ")" | num
Node *primary()
{
	if (consume('('))
	{
		Node *node = expr();
		expect(')');
		return node;
	}
	return new_num(expect_number());
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		error("%s: invalid arguments", argv[0]);
		return 1;
	}
	
	user_input = argv[1];
	token = tokenize(user_input);

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
