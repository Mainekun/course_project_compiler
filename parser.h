#ifndef PARSER_H
#define PARSER_H

#include "parser_rules.h"

class Parser
{
    Lexer* __lexer;
    QStack<Lexema> __stack;
    QList<Lexema> __line;
    QStack<QString> __conv_seq;


public:
    Parser(Lexer* lex) : __lexer(lex) {};
    Parser() {};

    [[nodiscard]] bool analyze();

    QStack<Lexema> stack() const { return __stack; }
    QList<Lexema> line() const { return __line; }
    QStack<QString> conv_seq() const { return __conv_seq; }
};

#endif // PARSER_H

