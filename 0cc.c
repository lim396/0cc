#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

//
// Tokenizer
//
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
	int len;		//Token length
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
bool consume(char *op)
{
	if (token->kind != TK_RESERVED || strlen(op) != token->len
		|| memcmp(token->str, op, token->len))
		return false;
	token = token->next;
	return true;
}

//If the next token is the expected symbol, read the token forward by one.
void expect(char *op)
{
	if (token->kind != TK_RESERVED || strlen(op) != token->len
		|| memcmp(token->str, op, token->len))
		error_at(token->str,"expected \"%c\"", op);
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
Token *new_token(Tokenkind kind, Token *cur, char *str, int len)
{
	Token *tok = calloc(1, sizeof(Token));
	tok->kind = kind;
	tok->str = str;
	tok->len = len;
	cur->next = tok;
	return tok;
}

bool startswith(char *p, char *q)
{
	return memcmp(p, q, strlen(q)) == 0;
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
		
		// Multi letter
		if (startswith(p, "==") || startswith(p, "!=") ||
			startswith(p, "<=") || startswith(p, ">="))
		{
			cur = new_token(TK_RESERVED, cur, p, 2);
			p += 2;
			continue ;
		}
		
		// Single letter
		if (strchr("+-*/()<>", *p))
		{
			cur = new_token(TK_RESERVED, cur, p++, 1);
			continue ;
		}
		
		// Interger
		if (isdigit(*p))
		{
			cur = new_token(TK_NUM, cur, p, 0);
			char *nh = p;						//number head
			cur->val = strtol(p, &p, 10);
			cur->len = p - nh;
			continue;
		}

		error_at(p, "invalid token");
	}

	new_token(TK_EOF, cur, p, 0);
	return head.next;
}

//
// Parser
//
typedef enum {
	ND_ADD, // +
	ND_SUB, // -
	ND_MUL, // *
	ND_DIV, // /
	ND_EQ,	// ==
	ND_NEQ,	// !=
	ND_LT,	// <  (less than)
	ND_LE,	// <= (less than or equal to)
	ND_NUM, // Integer
} Nodekind;

typedef struct Node Node;
struct Node {
	Nodekind kind; //Node kind
	Node *lhs;	   //Left-hand side
	Node *rhs;	   //Right-hand side
	int val;	   //if kind is ND_NUM
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

Node *expr();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

// expr = equality
Node *expr()
{
	return equality();
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality()
{
	Node *node = relational();
	
	for (;;)
	{
		if (consume("=="))
			node = new_binary(ND_EQ, node, relational());
		else if (consume("!="))
			node = new_binary(ND_NEQ, node, relational());
		return node;
	}
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational()
{
	Node *node = add();

	for (;;)
	{
		if (consume("<"))
			node = new_binary(ND_LT, node, add());
		else if (consume("<="))
			node = new_binary(ND_LE, node, add());
		else if (consume(">"))
			node = new_binary(ND_LT, add(), node);
		else if (consume(">="))
			node = new_binary(ND_LE, add(), node);
		else
			return node;
	}
}

// add = mul ("+" mul | "-" mul)*
Node *add()
{
	Node *node = mul();

	for (;;)
	{
		if (consume("+"))
			node = new_binary(ND_ADD, node, mul());
		else if (consume("-"))
			node = new_binary(ND_SUB, node, mul());
		else
			return node;
	}
}

// mul = unary ("*" unary | "/" unary)*
Node *mul()
{
	Node *node = unary();

	for (;;)
	{
		if (consume("*"))
			node = new_binary(ND_MUL, node, unary());
		else if (consume("/"))
			node = new_binary(ND_DIV, node, unary());
		else
			return node;
	}
}

// unary = ("+" | "-")? unary
//		 | primary
Node *unary()
{
	if (consume("+"))
		return unary();
	if (consume("-"))
		return new_binary(ND_SUB, new_num(0), unary());
	return primary();
}

// primary = "(" expr ")" | num
Node *primary()
{
	if (consume("("))
	{
		Node *node = expr();
		expect(")");
		return node;
	}
	return new_num(expect_number());
}

//
// Code generator
//
void gen(Node *node)
{
	if (node->kind == ND_NUM)
	{
		printf("  push %d\n", node->val);
		return ;
	}

	gen(node->lhs);
	gen(node->rhs);

	printf("  pop rdi\n");
	printf("  pop rax\n");

	switch (node->kind) {
		case ND_ADD: {
			printf("  add rax, rdi\n");
			break ;
		}
		case ND_SUB: {
			printf("  sub rax, rdi\n");
			break ;
		}
		case ND_MUL: {
			printf("  imul rax, rdi\n");
			break ;
		}
		case ND_DIV: {
			printf("  cqo\n");
			printf("  idiv rdi\n");
			break ;
		}
		case ND_EQ: {
			printf("  cmp rax, rdi\n");
			printf("  sete al\n");
			printf("  movzb rax, al\n");
			break ;
		}
		case ND_NEQ: {
			printf("  cmp rax, rdi\n");
			printf("  setne al\n");
			printf("  movzb rax, al\n");
			break ;
		}
		case ND_LT: {
			printf("  cmp rax, rdi\n");
			printf("  setl al\n");
			printf("  movzb rax, al\n");
			break ;
		}
		case ND_LE: {
			printf("  cmp rax, rdi\n");
			printf("  setle al\n");
			printf("  movzb rax, al\n");
			break ;
		}
	}
	printf("  push rax\n");
}

int main(int argc, char **argv)
{
	if (argc != 2)
		error("%s: invalid arguments", argv[0]);
	
	user_input = argv[1];

	//Tokenize and parse.
	token = tokenize(user_input);
	Node *node = expr();
	
	//Output the first part of the assembly
	printf(".intel_syntax noprefix\n");
	printf(".globl main\n");
	printf("main:\n");
	
	//Code generation while descending the abstract syntax tree
	gen(node);

	//There should still be the value of the entire expression on the stack top,
	//so Load it into RAX and use it as the return value from the function.
	printf("  pop rax\n");
	printf("  ret\n");
	return 0;
}
