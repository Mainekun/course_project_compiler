#ifndef PARSER_H
#define PARSER_H

#include "parser_rules.h"
#include "sema.h"
#include "lexer.h"
#include "syntaxtree.h"


class Parser
{
    Lexer* __lexer;
    QStack<Lexema> __stack;
    QList<Lexema> __line;
    //QStack<QString> __conv_seq;
    QList<QPair<QString, QList<Lexema>>> __conv_sequance;
    SemanticAnalyzer __semantic_analyzer;

    // Syntax tree
    ASTNode* __syntax_tree = nullptr;
    QStack<ASTNode*> __ast_stack;

    // AST construction helpers
    ASTNode* buildASTFromSequence(const QString& rule_name, const QList<Lexema>& sequence);
    ASTNode* createExpressionNode(const QList<Lexema>& operands);
    QString extractIdentifier(const Lexema& lex);
    int extractConstant(const Lexema& lex);

    // Store original lexemas for AST construction
    QMap<int, Lexema> __original_lexemas; // position -> original lexema

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

    // Syntax tree access
    ASTNode* getSyntaxTree() const { return __syntax_tree; }
};

#endif // PARSER_H

