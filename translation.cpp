#include "translation.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

// ... (previous code remains the same until generateStatement)

ASTAssemblerGenerator::ASTAssemblerGenerator()
    : label_counter(0), temp_counter(0) {
    assembly_code.clear();
    variable_map.clear();
}

QList<QString> ASTAssemblerGenerator::generateFromAST(ASTNode* ast_root) {
    assembly_code.clear();
    variable_map.clear();
    label_counter = 0;
    temp_counter = 0;

    // Generate assembly header
    assembly_code.append("; =========================================");
    assembly_code.append("; Generated Assembly Code from AST");
    assembly_code.append("; =========================================");
    assembly_code.append("");
    assembly_code.append(".386");
    assembly_code.append(".model flat, stdcall");
    assembly_code.append("option casemap :none");
    assembly_code.append("");
    assembly_code.append("includelib kernel32.lib");
    assembly_code.append("includelib msvcrt.lib");
    assembly_code.append("");
    assembly_code.append("extern printf:proc");
    assembly_code.append("extern scanf:proc");
    assembly_code.append("");
    assembly_code.append(".data");
    assembly_code.append("  fmt_int_out db '%d', 0");
    assembly_code.append("  fmt_int_in db '%d', 0");
    assembly_code.append("  newline db 10, 0");
    assembly_code.append("");

    // Generate code section
    assembly_code.append(".code");
    assembly_code.append("");

    // I/O functions (same as before)
    assembly_code.append("print_int proc value:dword");
    assembly_code.append("  push ebp");
    assembly_code.append("  mov ebp, esp");
    assembly_code.append("  push value");
    assembly_code.append("  push offset fmt_int_out");
    assembly_code.append("  call printf");
    assembly_code.append("  add esp, 8");
    assembly_code.append("  pop ebp");
    assembly_code.append("  ret");
    assembly_code.append("print_int endp");
    assembly_code.append("");

    assembly_code.append("read_int proc");
    assembly_code.append("  push ebp");
    assembly_code.append("  mov ebp, esp");
    assembly_code.append("  sub esp, 4");
    assembly_code.append("  lea eax, [ebp-4]");
    assembly_code.append("  push eax");
    assembly_code.append("  push offset fmt_int_in");
    assembly_code.append("  call scanf");
    assembly_code.append("  add esp, 8");
    assembly_code.append("  mov eax, [ebp-4]");
    assembly_code.append("  mov esp, ebp");
    assembly_code.append("  pop ebp");
    assembly_code.append("  ret");
    assembly_code.append("read_int endp");
    assembly_code.append("");

    assembly_code.append("print_nl proc");
    assembly_code.append("  push offset newline");
    assembly_code.append("  call printf");
    assembly_code.append("  add esp, 4");
    assembly_code.append("  ret");
    assembly_code.append("print_nl endp");
    assembly_code.append("");

    // Main program
    assembly_code.append("main proc");
    assembly_code.append("");

    if (ast_root && ast_root->type == ASTNodeType::PROGRAM) {
        generateProgram(dynamic_cast<ProgramNode*>(ast_root));
    }

    assembly_code.append("");
    assembly_code.append("  mov eax, 0");
    assembly_code.append("  ret");
    assembly_code.append("main endp");
    assembly_code.append("");
    assembly_code.append("end main");

    return assembly_code;
}

void ASTAssemblerGenerator::generateProgram(ProgramNode* program) {
    if (!program) return;

    // Declare variables in data section
    for (const auto& var : program->variables) {
        QString asm_var = QString("_%1").arg(var);
        variable_map[var] = asm_var;

        // Insert variable declaration in data section
        for (int i = 0; i < assembly_code.size(); i++) {
            if (assembly_code[i].contains(".data")) {
                // Find where to insert (after .data line)
                for (int j = i + 1; j < assembly_code.size(); j++) {
                    if (assembly_code[j].contains(".code")) {
                        assembly_code.insert(j, QString("  %1 dd 0").arg(asm_var));
                        break;
                    }
                }
                break;
            }
        }
    }

    // Generate statements
    for (auto stmt : program->body->statements) {
        generateStatement(stmt);
    }
}

void ASTAssemblerGenerator::generateStatement(ASTNode* stmt) {
    if (!stmt) return;

    // Add comment for statement type
    assembly_code.append(QString("  ; %1 statement").arg(stmt->typeName()));

    switch (stmt->type) {
    case ASTNodeType::ASSIGNMENT:
        generateAssignment(dynamic_cast<AssignmentNode*>(stmt));
        break;
    case ASTNodeType::INPUT:
        generateInput(dynamic_cast<InputNode*>(stmt));
        break;
    case ASTNodeType::OUTPUT:
        generateOutput(dynamic_cast<OutputNode*>(stmt));
        break;
    case ASTNodeType::WHILE_LOOP:
        generateWhileLoop(dynamic_cast<WhileLoopNode*>(stmt));
        break;
    case ASTNodeType::FOR_LOOP:
        generateForLoop(dynamic_cast<ForLoopNode*>(stmt));
        break;
    case ASTNodeType::IF_STATEMENT:
        generateIfStatement(dynamic_cast<IfStatementNode*>(stmt));
        break;
    case ASTNodeType::IF_ELSE:
        generateIfElse(dynamic_cast<IfElseNode*>(stmt));
        break;
    case ASTNodeType::BLOCK:
        generateBlock(dynamic_cast<BlockNode*>(stmt));
        break;
    default:
        assembly_code.append(QString("  ; Unknown statement type: %1").arg(stmt->typeName()));
        break;
    }
}

void ASTAssemblerGenerator::generateWhileLoop(WhileLoopNode* while_loop) {
    if (!while_loop) return;

    QString start_label = newLabel("while_start");
    QString end_label = newLabel("while_end");

    // Loop start
    assembly_code.append(QString("%1:").arg(start_label));

    // Evaluate condition
    QString cond_result = generateExpression(while_loop->condition);

    // Check condition (assuming condition is integer, 0 = false)
    if (cond_result.startsWith("constant:")) {
        QString value = cond_result.mid(9);
        assembly_code.append(QString("  cmp dword ptr [%1], 0").arg(getVariableName("zero_check")));
    } else if (cond_result.startsWith("register:")) {
        QString reg = cond_result.mid(9);
        assembly_code.append(QString("  cmp %1, 0").arg(reg));
    } else {
        assembly_code.append(QString("  mov eax, [%1]").arg(cond_result));
        assembly_code.append("  cmp eax, 0");
    }

    // Jump to end if condition is false (0)
    assembly_code.append(QString("  je %1").arg(end_label));

    // Generate loop body
    if (while_loop->body) {
        generateBlock(while_loop->body);
    }

    // Jump back to start
    assembly_code.append(QString("  jmp %1").arg(start_label));

    // Loop end
    assembly_code.append(QString("%1:").arg(end_label));
}

void ASTAssemblerGenerator::generateForLoop(ForLoopNode* for_loop) {
    if (!for_loop) return;

    QString start_label = newLabel("for_start");
    QString end_label = newLabel("for_end");
    QString inc_label = newLabel("for_inc");

    // Generate initialization
    if (for_loop->initialization) {
        // For initialization like "let i = 0"
        if (for_loop->initialization->type == ASTNodeType::ASSIGNMENT) {
            generateAssignment(dynamic_cast<AssignmentNode*>(for_loop->initialization));
        } else {
            QString init_result = generateExpression(for_loop->initialization);
            // Store initialization result if needed
        }
    }

    // Loop start
    assembly_code.append(QString("%1:").arg(start_label));

    // Evaluate condition
    if (for_loop->condition) {
        QString cond_result = generateExpression(for_loop->condition);

        // Check condition (in FOR loop, continue while condition is true/non-zero)
        if (cond_result.startsWith("constant:")) {
            QString value = cond_result.mid(9);
            assembly_code.append(QString("  cmp dword ptr [%1], 0").arg(getVariableName("zero_check")));
        } else if (cond_result.startsWith("register:")) {
            QString reg = cond_result.mid(9);
            assembly_code.append(QString("  cmp %1, 0").arg(reg));
        } else {
            assembly_code.append(QString("  mov eax, [%1]").arg(cond_result));
            assembly_code.append("  cmp eax, 0");
        }

        // Jump to end if condition is false (0)
        assembly_code.append(QString("  je %1").arg(end_label));
    }

    // Generate loop body
    if (for_loop->body) {
        generateBlock(for_loop->body);
    }

    // Increment label
    assembly_code.append(QString("%1:").arg(inc_label));

    // Generate increment
    if (for_loop->increment) {
        // For increment like "let i = i + 1"
        if (for_loop->increment->type == ASTNodeType::ASSIGNMENT) {
            generateAssignment(dynamic_cast<AssignmentNode*>(for_loop->increment));
        } else {
            QString inc_result = generateExpression(for_loop->increment);
            // Store increment result if needed
        }
    } else {
        // Default increment (if none specified)
        assembly_code.append("  ; Default increment (none specified)");
    }

    // Jump back to start (check condition again)
    assembly_code.append(QString("  jmp %1").arg(start_label));

    // Loop end
    assembly_code.append(QString("%1:").arg(end_label));
}

void ASTAssemblerGenerator::generateIfStatement(IfStatementNode* if_stmt) {
    if (!if_stmt) return;

    QString else_label = newLabel("if_else");
    QString end_label = newLabel("if_end");

    // Evaluate condition
    QString cond_result = generateExpression(if_stmt->condition);

    // Check condition
    if (cond_result.startsWith("constant:")) {
        QString value = cond_result.mid(9);
        assembly_code.append(QString("  cmp %1, 0").arg(value));
    } else if (cond_result.startsWith("register:")) {
        QString reg = cond_result.mid(9);
        assembly_code.append(QString("  cmp %1, 0").arg(reg));
    } else {
        assembly_code.append(QString("  mov eax, [%1]").arg(cond_result));
        assembly_code.append("  cmp eax, 0");
    }

    // Jump to else (or end if no else) if condition is false
    assembly_code.append(QString("  je %1").arg(else_label));

    // Generate then block
    if (if_stmt->then_block) {
        generateBlock(if_stmt->then_block);
    }

    // Jump to end (skip else block)
    assembly_code.append(QString("  jmp %1").arg(end_label));

    // Else label (or end if no else)
    assembly_code.append(QString("%1:").arg(else_label));

    // Generate else block if it exists
    if (if_stmt->else_block) {
        generateBlock(if_stmt->else_block);
    }

    // End label
    assembly_code.append(QString("%1:").arg(end_label));
}

void ASTAssemblerGenerator::generateIfElse(IfElseNode* if_else) {
    if (!if_else) return;

    QString else_label = newLabel("if_else");
    QString end_label = newLabel("if_end");

    // Evaluate condition
    QString cond_result = generateExpression(if_else->condition);

    // Check condition
    if (cond_result.startsWith("constant:")) {
        QString value = cond_result.mid(9);
        assembly_code.append(QString("  cmp %1, 0").arg(value));
    } else if (cond_result.startsWith("register:")) {
        QString reg = cond_result.mid(9);
        assembly_code.append(QString("  cmp %1, 0").arg(reg));
    } else {
        assembly_code.append(QString("  mov eax, [%1]").arg(cond_result));
        assembly_code.append("  cmp eax, 0");
    }

    // Jump to else if condition is false
    assembly_code.append(QString("  je %1").arg(else_label));

    // Generate then block
    if (if_else->then_block) {
        generateBlock(if_else->then_block);
    }

    // Jump to end (skip else block)
    assembly_code.append(QString("  jmp %1").arg(end_label));

    // Else label
    assembly_code.append(QString("%1:").arg(else_label));

    // Generate else block
    if (if_else->else_block) {
        generateBlock(if_else->else_block);
    }

    // End label
    assembly_code.append(QString("%1:").arg(end_label));
}

void ASTAssemblerGenerator::generateBlock(BlockNode* block) {
    if (!block) return;

    // Add block comment
    assembly_code.append("  ; {");

    // Generate all statements in the block
    for (auto stmt : block->statements) {
        generateStatement(stmt);
    }

    // End block comment
    assembly_code.append("  ; }");
}

void ASTAssemblerGenerator::generateAssignment(AssignmentNode* assign) {
    if (!assign) return;

    QString var_name = getVariableName(assign->var_name);
    QString expr_code = generateExpression(assign->expression);

    assembly_code.append(QString("  ; %1 = expression").arg(assign->var_name));

    if (expr_code.startsWith("constant:")) {
        // Constant assignment
        QString value = expr_code.mid(9); // Skip "constant:"
        assembly_code.append(QString("  mov dword ptr [%1], %2").arg(var_name).arg(value));
    } else if (expr_code.startsWith("register:")) {
        // Result is in a register
        QString reg = expr_code.mid(9); // Skip "register:"
        assembly_code.append(QString("  mov [%1], %2").arg(var_name).arg(reg));
    } else {
        // Variable assignment
        assembly_code.append(QString("  mov eax, [%1]").arg(expr_code));
        assembly_code.append(QString("  mov [%1], eax").arg(var_name));
    }
}

void ASTAssemblerGenerator::generateInput(InputNode* input) {
    if (!input) return;

    QString var_name = getVariableName(input->var_name);
    assembly_code.append(QString("  ; INPUT %1").arg(input->var_name));
    assembly_code.append("  call read_int");
    assembly_code.append(QString("  mov [%1], eax").arg(var_name));
}

void ASTAssemblerGenerator::generateOutput(OutputNode* output) {
    if (!output) return;

    QString expr_code = generateExpression(output->expression);
    assembly_code.append("  ; OUTPUT expression");

    if (expr_code.startsWith("constant:")) {
        QString value = expr_code.mid(9);
        assembly_code.append(QString("  push %1").arg(value));
    } else if (expr_code.startsWith("register:")) {
        QString reg = expr_code.mid(9);
        assembly_code.append(QString("  push %1").arg(reg));
    } else {
        assembly_code.append(QString("  push dword ptr [%1]").arg(expr_code));
    }

    assembly_code.append("  call print_int");
    assembly_code.append("  call print_nl");
}

QString ASTAssemblerGenerator::generateExpression(ASTNode* expr) {
    if (!expr) return "";

    switch (expr->type) {
    case ASTNodeType::IDENTIFIER: {
        IdentifierNode* id = dynamic_cast<IdentifierNode*>(expr);
        return getVariableName(id->name);
    }
    case ASTNodeType::CONSTANT: {
        ConstantNode* cnst = dynamic_cast<ConstantNode*>(expr);
        return QString("constant:%1").arg(cnst->value);
    }
    case ASTNodeType::BINARY_OP: {
        BinaryOpNode* binop = dynamic_cast<BinaryOpNode*>(expr);
        QString left = generateExpression(binop->left);
        QString right = generateExpression(binop->right);
        QString temp = newTemp();

        assembly_code.append(QString("  ; %1 %2 %3").arg(
            left.contains("constant:") ? left.mid(9) : left,
            binop->op,
            right.contains("constant:") ? right.mid(9) : right));

        // Load left operand
        if (left.startsWith("constant:")) {
            assembly_code.append(QString("  mov eax, %1").arg(left.mid(9)));
        } else {
            assembly_code.append(QString("  mov eax, [%1]").arg(left));
        }

        // Perform operation
        if (binop->op == "+") {
            if (right.startsWith("constant:")) {
                assembly_code.append(QString("  add eax, %1").arg(right.mid(9)));
            } else {
                assembly_code.append(QString("  add eax, [%1]").arg(right));
            }
        }
        else if (binop->op == "-") {
            if (right.startsWith("constant:")) {
                assembly_code.append(QString("  sub eax, %1").arg(right.mid(9)));
            } else {
                assembly_code.append(QString("  sub eax, [%1]").arg(right));
            }
        }
        else if (binop->op == "*") {
            if (right.startsWith("constant:")) {
                assembly_code.append(QString("  imul eax, %1").arg(right.mid(9)));
            } else {
                assembly_code.append(QString("  imul eax, [%1]").arg(right));
            }
        }
        else if (binop->op == "/") {
            assembly_code.append("  cdq");
            if (right.startsWith("constant:")) {
                assembly_code.append(QString("  mov ebx, %1").arg(right.mid(9)));
                assembly_code.append("  idiv ebx");
            } else {
                assembly_code.append(QString("  idiv dword ptr [%1]").arg(right));
            }
        }

        // Store result in temp
        assembly_code.append(QString("  mov [%1], eax").arg(temp));
        return temp;
    }
    default:
        return "";
    }
}

QString ASTAssemblerGenerator::newLabel(const QString& prefix) {
    return QString("%1_%2").arg(prefix).arg(label_counter++);
}

QString ASTAssemblerGenerator::newTemp() {
    QString temp = QString("_temp%1").arg(temp_counter++);

    // Add temp variable declaration to data section
    for (int i = 0; i < assembly_code.size(); i++) {
        if (assembly_code[i].contains(".data")) {
            for (int j = i + 1; j < assembly_code.size(); j++) {
                if (assembly_code[j].contains(".code")) {
                    assembly_code.insert(j, QString("  %1 dd 0").arg(temp));
                    break;
                }
            }
            break;
        }
    }

    return temp;
}

QString ASTAssemblerGenerator::getVariableName(const QString& var) {
    if (!variable_map.contains(var)) {
        QString asm_var = QString("_%1").arg(var);
        variable_map[var] = asm_var;

        // Add to data section
        for (int i = 0; i < assembly_code.size(); i++) {
            if (assembly_code[i].contains(".data")) {
                for (int j = i + 1; j < assembly_code.size(); j++) {
                    if (assembly_code[j].contains(".code")) {
                        assembly_code.insert(j, QString("  %1 dd 0").arg(asm_var));
                        break;
                    }
                }
                break;
            }
        }
    }
    return variable_map[var];
}

void ASTAssemblerGenerator::print() const {
    qDebug() << "\n=== GENERATED ASSEMBLY (AST-based) ===";
    for (const auto& line : assembly_code) {
        qDebug() << line.toUtf8().constData();
    }
}

void ASTAssemblerGenerator::save(const QString& filename) const {
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const auto& line : assembly_code) {
            out << line << "\n";
        }
        file.close();
        qDebug() << "Saved AST-based assembly to" << filename;
    }
}


// ... (rest of the code remains the same)
