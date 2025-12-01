#include "sema.h"
#include <QDebug>

SemanticAnalyzer::SemanticAnalyzer() {
    declared_variables.clear();
    semantic_errors.clear();
}

bool SemanticAnalyzer::analyze(const QList<QPair<QString, QList<Lexema>>>& conv_sequence) {
    semantic_errors.clear();

    try {
        for (const auto& [rule_name, operands] : conv_sequence) {
            // Find the rule type
            RuleType rule_type = RuleType::PROGRAM;
            for (const auto& rule : rules) {
                if (rule.name() == rule_name) {
                    rule_type = rule.type();
                    break;
                }
            }

            // Process variable declarations
            if (rule_type == RuleType::VAR) {
                processVariableDeclaration(operands);
            }

            // Process variable usage in assignments, expressions, etc.
            else if (rule_type == RuleType::EXPR ||
                     rule_type == RuleType::IN || rule_type == RuleType::OUT ||
                     rule_type == RuleType::FOR || rule_type == RuleType::WHILE ||
                     rule_type == RuleType::IF) {
                processIdentifierUsage(operands);
            }
        }

        return semantic_errors.isEmpty();

    } catch (const std::exception& e) {
        semantic_errors.append(QString("Analysis error: %1").arg(e.what()));
        return false;
    }
}

void SemanticAnalyzer::processVariableDeclaration(const QList<Lexema>& operands) {
    // Extract variable names from VAR declaration
    for (const auto& lex : operands) {
        if (lex.type() == TokenType::Id) {
            QString var_name = lex.const_name();
            if (declared_variables.contains(var_name)) {
                semantic_errors.append(QString("Variable '%1' is already declared").arg(var_name));
            } else {
                declared_variables.insert(var_name);
                current_scope_vars.insert(var_name);
            }
        }
    }
}

void SemanticAnalyzer::processIdentifierUsage(const QList<Lexema>& operands) {
    // Check all identifiers in the operands
    for (const auto& lex : operands) {
        if (lex.type() == TokenType::Id) {
            checkVariableDeclaration(lex.const_name());
        }
    }
}

void SemanticAnalyzer::checkVariableDeclaration(const QString& var_name) {
    if (!declared_variables.contains(var_name)) {
        semantic_errors.append(QString("Variable '%1' is used before declaration").arg(var_name));
    }
}

void SemanticAnalyzer::printErrors() const {
    if (semantic_errors.isEmpty()) {
        qDebug() << "No semantic errors found. All variables are properly declared.";
        return;
    }

    qDebug() << "=== VARIABLE DECLARATION ERRORS ===";
    for (const auto& error : semantic_errors) {
        qDebug() << "ERROR:" << error;
    }
}
