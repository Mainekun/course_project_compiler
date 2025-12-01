#ifndef SYNTAX_TREE_H
#define SYNTAX_TREE_H

#include "lexer.h"
#include <QList>

// AST Node types (updated)
enum class ASTNodeType {
    PROGRAM,
    VAR_DECL,
    ASSIGNMENT,
    INPUT,
    OUTPUT,
    WHILE_LOOP,
    FOR_LOOP,
    IF_STATEMENT,
    IF_ELSE,
    BLOCK,
    EXPRESSION,
    IDENTIFIER,
    CONSTANT,
    BINARY_OP,
    UNARY_OP
};

// Forward declaration
class ASTNode;

// AST Node base class
class ASTNode {
public:
    ASTNodeType type;
    int line_number;

    ASTNode(ASTNodeType t) : type(t), line_number(0) {}
    virtual ~ASTNode() = default;

    // Helper to get type as string
    QString typeName() const {
        switch(type) {
        case ASTNodeType::PROGRAM: return "PROGRAM";
        case ASTNodeType::VAR_DECL: return "VAR_DECL";
        case ASTNodeType::ASSIGNMENT: return "ASSIGNMENT";
        case ASTNodeType::INPUT: return "INPUT";
        case ASTNodeType::OUTPUT: return "OUTPUT";
        case ASTNodeType::WHILE_LOOP: return "WHILE_LOOP";
        case ASTNodeType::FOR_LOOP: return "FOR_LOOP";
        case ASTNodeType::IF_STATEMENT: return "IF_STATEMENT";
        case ASTNodeType::IF_ELSE: return "IF_ELSE";
        case ASTNodeType::BLOCK: return "BLOCK";
        case ASTNodeType::EXPRESSION: return "EXPRESSION";
        case ASTNodeType::IDENTIFIER: return "IDENTIFIER";
        case ASTNodeType::CONSTANT: return "CONSTANT";
        case ASTNodeType::BINARY_OP: return "BINARY_OP";
        default: return "UNKNOWN";
        }
    }
};

// Block node for multiple statements
class BlockNode : public ASTNode {
public:
    QList<ASTNode*> statements;

    BlockNode() : ASTNode(ASTNodeType::BLOCK) {}

    ~BlockNode() {
        for (auto stmt : statements) {
            delete stmt;
        }
    }

    void addStatement(ASTNode* stmt) {
        if (stmt) statements.append(stmt);
    }
};

// Identifier node
class IdentifierNode : public ASTNode {
public:
    QString name;

    IdentifierNode(const QString& n)
        : ASTNode(ASTNodeType::IDENTIFIER), name(n) {}
};

// Constant node
class ConstantNode : public ASTNode {
public:
    int value;

    ConstantNode(int v)
        : ASTNode(ASTNodeType::CONSTANT), value(v) {}
};

// Binary operation node
class BinaryOpNode : public ASTNode {
public:
    QString op;
    ASTNode* left;
    ASTNode* right;

    BinaryOpNode(const QString& o, ASTNode* l, ASTNode* r)
        : ASTNode(ASTNodeType::BINARY_OP), op(o), left(l), right(r) {}

    ~BinaryOpNode() {
        delete left;
        delete right;
    }
};

// Assignment node
class AssignmentNode : public ASTNode {
public:
    QString var_name;
    ASTNode* expression;

    AssignmentNode(const QString& var, ASTNode* expr)
        : ASTNode(ASTNodeType::ASSIGNMENT), var_name(var), expression(expr) {}

    ~AssignmentNode() {
        delete expression;
    }
};

// Input node
class InputNode : public ASTNode {
public:
    QString var_name;

    InputNode(const QString& var)
        : ASTNode(ASTNodeType::INPUT), var_name(var) {}
};

// Output node
class OutputNode : public ASTNode {
public:
    ASTNode* expression;

    OutputNode(ASTNode* expr)
        : ASTNode(ASTNodeType::OUTPUT), expression(expr) {}

    ~OutputNode() {
        delete expression;
    }
};

// While loop node
class WhileLoopNode : public ASTNode {
public:
    ASTNode* condition;
    BlockNode* body;

    WhileLoopNode(ASTNode* cond, BlockNode* b = nullptr)
        : ASTNode(ASTNodeType::WHILE_LOOP), condition(cond), body(b) {}

    ~WhileLoopNode() {
        delete condition;
        delete body;
    }

    void setBody(BlockNode* b) {
        body = b;
    }
};

// For loop node
class ForLoopNode : public ASTNode {
public:
    ASTNode* initialization;
    ASTNode* condition;
    ASTNode* increment;
    BlockNode* body;

    ForLoopNode(ASTNode* init, ASTNode* cond, ASTNode* inc, BlockNode* b = nullptr)
        : ASTNode(ASTNodeType::FOR_LOOP),
        initialization(init), condition(cond), increment(inc), body(b) {}

    ~ForLoopNode() {
        delete initialization;
        delete condition;
        delete increment;
        delete body;
    }

    void setBody(BlockNode* b) {
        body = b;
    }
};

// If statement node
class IfStatementNode : public ASTNode {
public:
    ASTNode* condition;
    BlockNode* then_block;
    BlockNode* else_block; // nullptr if no else

    IfStatementNode(ASTNode* cond, BlockNode* then_blk, BlockNode* else_blk = nullptr)
        : ASTNode(ASTNodeType::IF_STATEMENT),
        condition(cond), then_block(then_blk), else_block(else_blk) {}

    ~IfStatementNode() {
        delete condition;
        delete then_block;
        delete else_block;
    }
};

// If-else node (special case)
class IfElseNode : public ASTNode {
public:
    ASTNode* condition;
    BlockNode* then_block;
    BlockNode* else_block;

    IfElseNode(ASTNode* cond, BlockNode* then_blk, BlockNode* else_blk)
        : ASTNode(ASTNodeType::IF_ELSE),
        condition(cond), then_block(then_blk), else_block(else_blk) {}

    ~IfElseNode() {
        delete condition;
        delete then_block;
        delete else_block;
    }
};

// Variable declaration node
class VarDeclNode : public ASTNode {
public:
    QList<QString> variable_names;

    VarDeclNode(const QList<QString>& vars)
        : ASTNode(ASTNodeType::VAR_DECL), variable_names(vars) {}
};

// Program node
class ProgramNode : public ASTNode {
public:
    QString name;
    QList<QString> variables;
    BlockNode* body;

    ProgramNode(const QString& n)
        : ASTNode(ASTNodeType::PROGRAM), name(n), body(new BlockNode()) {}

    ~ProgramNode() {
        delete body;
    }

    void addVariable(const QString& var) {
        if (!variables.contains(var)) {
            variables.append(var);
        }
    }

    void addStatement(ASTNode* stmt) {
        if (stmt) body->addStatement(stmt);
    }
};

#endif // SYNTAX_TREE_H
