#ifndef PARSER_H
#define PARSER_H

#include "parser_rules.h"
#include "sema.h"
#include "lexer.h"

class Parser
{
    Lexer* __lexer;
    QStack<Lexema> __stack;
    QList<Lexema> __line;
    //QStack<QString> __conv_seq;
    QList<QPair<QString, QList<Lexema>>> __conv_sequance;
    SemanticAnalyzer __semantic_analyzer;


public:
    Parser(Lexer* lex) : __lexer(lex) {};
    Parser() {};

    [[nodiscard]] bool analyze();

    QStack<Lexema> stack() const { return __stack; }
    QList<Lexema> line() const { return __line; }

    QList<QPair<QString, QList<Lexema>>>
        conv_sequance() const { return __conv_sequance; }


    bool hasSemanticErrors() const { return __semantic_analyzer.hasErrors(); }
    void printSemanticErrors() const { __semantic_analyzer.printErrors(); }
    const QList<QString>& getSemanticErrors() const { return __semantic_analyzer.getErrors(); }

};

#endif // PARSER_H

