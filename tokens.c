#include "tokens.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

p_tokens split(char * string)
{
	p_tokens tokens;
	p_token token = NULL, tkn = NULL;
	char * str = string;
	int lo = 0, hi = 0, cnt = 0;

	tokens = (p_tokens) malloc (sizeof(*tokens));
	token = (p_token) malloc (sizeof(*token));
	tokens->token = token;
	token->next = NULL;

	while(str[lo] != '\0'){
		while(isspace(str[lo])){
			lo++;
		}
		hi = lo;
		if(str[lo] == '\0'){
			break;
		}
		while(!isspace(str[hi]) && (str[hi] != '\0')){
			hi++;
		}

		cnt++;
		tkn = (p_token) malloc (sizeof(*token));
		tkn->word = (char *) malloc (hi-lo+1);
		memcpy(tkn->word, str+lo, hi-lo);
		tkn->word[hi-lo] = '\0';
		tkn->next = NULL;
		token->next = tkn;
		token = tkn;

		lo = hi+1;
	}

	tokens->cnt = cnt;
	token = tokens->token;
	tokens->token = token->next;
	free(token);

	return tokens;
}

void destroy_tokens(p_tokens tokens)
{
	p_token token, tk;
	for(token = tokens->token; token != NULL; token = tk){
		tk = token->next;
		free(token->word);
		free(token);
	}
	free(tokens);
}
