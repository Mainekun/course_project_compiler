#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include <QSet>
#include <QString>
#include <QList>
#include <QPair>
#include "lexer.h"
#include "parser_rules.h"

class SemanticAnalyzer {
private:
    QSet<QString> declared_variables;
    QList<QString> semantic_errors;

    // Track variables declared in current scope
    QSet<QString> current_scope_vars;

    void processVariableDeclaration(const QList<Lexema>& operands);
    void processIdentifierUsage(const QList<Lexema>& operands);
    void checkVariableDeclaration(const QString& var_name);

public:
    SemanticAnalyzer();

    // Main analysis method - only checks variable declaration before use
    bool analyze(const QList<QPair<QString, QList<Lexema>>>& conv_sequence);

    // Utility methods
    void printErrors() const;

    // Getters
    bool hasErrors() const { return !semantic_errors.isEmpty(); }
    const QList<QString>& getErrors() const { return semantic_errors; }
    const QSet<QString>& getDeclaredVariables() const { return declared_variables; }
};

#endif // SEMANTIC_ANALYZER_H
