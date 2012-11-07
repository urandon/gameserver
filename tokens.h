#ifndef TOKENS_H_
#define TOKENS_H_

struct token{
	char * word;
	struct token * next;
};

typedef struct token *p_token;

struct tokens{
	p_token token;
	int cnt;
};

typedef struct tokens *p_tokens;


p_tokens split(char * string);
void destroy_tokens(p_tokens tokens);

#endif /* TOKENS_H_ */
