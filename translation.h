#ifndef AST_ASSEMBLER_GENERATOR_H
#define AST_ASSEMBLER_GENERATOR_H

#include "parser.h"
#include <QString>
#include <QList>
#include <QMap>

class ASTAssemblerGenerator {
private:
    QList<QString> assembly_code;
    QMap<QString, QString> variable_map;
    int label_counter;
    int temp_counter;

    QString newLabel(const QString& prefix = "L");
    QString newTemp();
    QString getVariableName(const QString& var);

    // Code generation from AST
    void generateProgram(ProgramNode* program);
    void generateStatement(ASTNode* stmt);
    void generateAssignment(AssignmentNode* assign);
    void generateInput(InputNode* input);
    void generateOutput(OutputNode* output);
    void generateWhileLoop(WhileLoopNode* while_loop);
    void generateForLoop(ForLoopNode* for_loop);
    void generateIfStatement(IfStatementNode* if_stmt);
    void generateIfElse(IfElseNode* if_else);
    void generateBlock(BlockNode* block);
    QString generateExpression(ASTNode* expr);

public:
    ASTAssemblerGenerator();

    QList<QString> generateFromAST(ASTNode* ast_root);

    void print() const;
    void save(const QString& filename) const;
};

#endif // AST_ASSEMBLER_GENERATOR_H
