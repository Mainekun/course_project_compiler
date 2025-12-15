// asmgenerator.h
#ifndef ASMGENERATOR_H
#define ASMGENERATOR_H

#include "lexer.h"
#include <QMap>
#include <QFile>
#include <QTextStream>
#include <QStack>
#include <QSet>

class AsmGenerator {
private:
    Lexer* __lexer;
    QMap<QString, int> __variable_sizes;
    QMap<QString, QString> __variable_types;
    QList<QString> __generated_code;
    QStack<QString> __loop_labels;
    QStack<QString> __if_labels;
    int __label_counter = 0;
    int __temp_counter = 0;
    int __loop_depth = 0;
    int __if_depth = 0;
    int __current_token_index = 0;
    QString __current_program_name;
    QSet<QString> __declared_variables;
    bool __in_program = false;

    struct LoopContext {
        QString start_label;
        QString end_label;
        QString condition_label;
        bool is_for_loop = false;
        QString increment_code;
    };

    QStack<LoopContext> __loop_contexts;

public:
    AsmGenerator(Lexer* lex) : __lexer(lex) {}

    bool generate(const QString& output_filename = "output.asm") {
        __generated_code.clear();
        __label_counter = 0;
        __temp_counter = 0;
        __loop_depth = 0;
        __if_depth = 0;
        __current_token_index = 0;
        __current_program_name.clear();
        __declared_variables.clear();
        __in_program = false;
        __loop_contexts.clear();
        __loop_labels.clear();
        __if_labels.clear();

        generateDataSection();
        generateCodeSection();

        return writeToFile(output_filename);
    }

    QString getNextLabel(const QString& prefix = "L") {
        return QString("%1%2").arg(prefix).arg(__label_counter++);
    }

private:
    void generateDataSection() {
        __generated_code.append("section .data");
        __generated_code.append("");

        // Process constants from lexer
        auto consts = __lexer->get_consts();
        for (const auto& lex : consts) {
            QString const_name = lex.const_name().isEmpty() ?
                                     QString("const_%1").arg(lex.value()) :
                                     lex.const_name();

            __generated_code.append(QString("    %1 dd %2").arg(const_name, lex.value()));
        }

        __generated_code.append("");
    }

    void generateCodeSection() {
        __generated_code.append("section .text");
        __generated_code.append("global _start");
        __generated_code.append("");
        __generated_code.append("_start:");
        __generated_code.append("");

        auto tokens = __lexer->get_tokenized_code();
        __current_token_index = 0;

        // Reset state for new program
        resetProgramState();

        // Process all tokens
        while (__current_token_index < tokens.size()) {
            processToken(tokens);
        }

        // Add program exit
        __generated_code.append("");
        __generated_code.append("    ; Exit program");
        __generated_code.append("    mov eax, 1      ; sys_exit");
        __generated_code.append("    xor ebx, ebx    ; exit code 0");
        __generated_code.append("    int 0x80");
        __generated_code.append("");

        generateHelperFunctions();
    }

    void resetProgramState() {
        __loop_depth = 0;
        __if_depth = 0;
        __loop_contexts.clear();
        __loop_labels.clear();
        __if_labels.clear();
        __declared_variables.clear();
        __in_program = false;
        __current_program_name.clear();
    }

    void processToken(const QList<Lexema>& tokens) {
        auto& token = tokens[__current_token_index];

        if (token.value() == "program") {
            // Start of a new program
            if (__in_program) {
                // Close previous program if any
                closeAllBlocks();
            }

            resetProgramState();
            __in_program = true;

            if (__current_token_index + 1 < tokens.size()) {
                __current_program_name = tokens[__current_token_index + 1].value();
                __generated_code.append("");
                __generated_code.append(QString("    ; Program: %1").arg(__current_program_name));
            }

            __current_token_index += 2; // Skip "program" and program name
        }
        else if (token.value() == "var") {
            // Variable declaration section
            __current_token_index++; // Skip "var"

            // Collect variable names until "int" or "integer"
            while (__current_token_index < tokens.size()) {
                QString token_value = tokens[__current_token_index].value();

                if (token_value == "int" || token_value == "integer") {
                    __current_token_index++; // Skip "int" or "integer"
                    break;
                }

                if (token.type() == TokenType::Id || token.type() == TokenType::Word) {
                    if (token_value != ",") {
                        __declared_variables.insert(token_value);
                        // Declare variable in .bss section
                        if (!__variable_sizes.contains(token_value)) {
                            __variable_sizes[token_value] = 4;
                            __variable_types[token_value] = "dd";
                        }
                    }
                }

                __current_token_index++;
            }
        }
        else if (token.value() == "begin") {
            __current_token_index++;
            __generated_code.append("    ; Begin main block");
        }
        else if (token.value() == "end") {
            // Check if it's "end." (end of program)
            if (__current_token_index + 1 < tokens.size() &&
                tokens[__current_token_index + 1].value() == ".") {
                closeAllBlocks();
                __current_token_index += 2; // Skip "end" and "."
                __generated_code.append("    ; End program");
                __in_program = false;
                return;
            }

            // Close current block
            closeCurrentBlock();
            __current_token_index++;
        }
        else if (token.value() == "if") {
            processIfStatement(tokens);
        }
        else if (token.value() == "else") {
            processElseStatement(tokens);
        }
        else if (token.value() == "while") {
            processWhileLoop(tokens);
        }
        else if (token.value() == "for") {
            processForLoop(tokens);
        }
        else if (token.value() == "let") {
            processAssignment(tokens);
        }
        else if (token.value() == "input") {
            processInput(tokens);
        }
        else if (token.value() == "output") {
            processOutput(tokens);
        }
        else if (token.value() == "then") {
            // Skip "then" keyword
            __current_token_index++;
        }
        else if (token.value() == ";") {
            __generated_code.append("    ; Statement end");
            __current_token_index++;
        }
        else if (token.type() == TokenType::Id && __declared_variables.contains(token.value())) {
            // Variable reference - handled in other contexts
            __current_token_index++;
        }
        else {
            __current_token_index++; // Skip other tokens
        }
    }

    void processIfStatement(const QList<Lexema>& tokens) {
        QString else_label = getNextLabel("ELSE_");
        QString end_if_label = getNextLabel("END_IF_");

        __if_labels.push(else_label);
        __if_labels.push(end_if_label);

        __generated_code.append("");
        __generated_code.append("    ; If statement");

        __current_token_index++; // Skip "if"

        if (__current_token_index < tokens.size() && tokens[__current_token_index].value() == "(") {
            __current_token_index++; // Skip "("

            QString condition = extractCondition(tokens);
            generateConditionCode(condition, else_label);

            __current_token_index++; // Skip ")"
        }
    }

    void processElseStatement(const QList<Lexema>& tokens) {
        if (!__if_labels.isEmpty()) {
            QString else_label = __if_labels.pop();
            QString end_if_label = __if_labels.pop();

            __generated_code.append("    jmp " + end_if_label);
            __generated_code.append(else_label + ":");
            __generated_code.append("    ; Else block");

            __if_labels.push(end_if_label);
        }
        __current_token_index++;
    }

    void processWhileLoop(const QList<Lexema>& tokens) {
        __loop_depth++;

        LoopContext context;
        context.start_label = getNextLabel("WHILE_START_");
        context.end_label = getNextLabel("WHILE_END_");
        context.condition_label = getNextLabel("WHILE_COND_");
        context.is_for_loop = false;

        __loop_contexts.push(context);

        __generated_code.append("");
        __generated_code.append("    ; While loop");
        __generated_code.append(context.condition_label + ":");

        __current_token_index++; // Skip "while"

        if (__current_token_index < tokens.size() && tokens[__current_token_index].value() == "(") {
            __current_token_index++; // Skip "("

            QString condition = extractCondition(tokens);
            generateConditionCode(condition, context.end_label);

            __current_token_index++; // Skip ")"
        }

        __generated_code.append(context.start_label + ":");

        // Check for "begin" after condition
        if (__current_token_index < tokens.size() && tokens[__current_token_index].value() == "begin") {
            __current_token_index++; // Skip "begin"
        }
    }

    void processForLoop(const QList<Lexema>& tokens) {
        __loop_depth++;

        LoopContext context;
        context.start_label = getNextLabel("FOR_START_");
        context.end_label = getNextLabel("FOR_END_");
        context.condition_label = getNextLabel("FOR_COND_");
        context.is_for_loop = true;

        __loop_contexts.push(context);

        __generated_code.append("");
        __generated_code.append("    ; For loop");

        __current_token_index++; // Skip "for"

        if (__current_token_index < tokens.size() && tokens[__current_token_index].value() == "(") {
            __current_token_index++; // Skip "("

            // Parse initialization
            QString initialization = "";
            while (__current_token_index < tokens.size() &&
                   tokens[__current_token_index].value() != ";") {
                initialization += tokens[__current_token_index].value() + " ";
                __current_token_index++;
            }
            initialization = initialization.trimmed();

            if (!initialization.isEmpty()) {
                // Handle initialization (could be "1" or "let i = 0")
                if (initialization != "1") { // Skip constant "1"
                    processAssignmentFromString(initialization);
                }
            }

            if (__current_token_index < tokens.size() && tokens[__current_token_index].value() == ";") {
                __current_token_index++;
            }

            __generated_code.append(context.condition_label + ":");

            // Parse condition
            QString condition = "";
            while (__current_token_index < tokens.size() &&
                   tokens[__current_token_index].value() != ";") {
                condition += tokens[__current_token_index].value() + " ";
                __current_token_index++;
            }
            condition = condition.trimmed();

            if (!condition.isEmpty() && condition != "1") { // Skip constant "1"
                generateConditionCode(condition, context.end_label);
            } else if (condition == "1") {
                // Always true condition
                __generated_code.append("    ; Always true condition");
            }

            if (__current_token_index < tokens.size() && tokens[__current_token_index].value() == ";") {
                __current_token_index++;
            }

            // Parse increment
            QString increment = "";
            while (__current_token_index < tokens.size() &&
                   tokens[__current_token_index].value() != ")") {
                increment += tokens[__current_token_index].value() + " ";
                __current_token_index++;
            }
            increment = increment.trimmed();
            context.increment_code = increment;

            if (__current_token_index < tokens.size() && tokens[__current_token_index].value() == ")") {
                __current_token_index++;
            }

            __generated_code.append(context.start_label + ":");

            // Check for "begin" after for loop
            if (__current_token_index < tokens.size() && tokens[__current_token_index].value() == "begin") {
                __current_token_index++; // Skip "begin"
            }
        }
    }

    void processAssignment(const QList<Lexema>& tokens) {
        if (__current_token_index + 3 < tokens.size() &&
            tokens[__current_token_index + 2].value() == "=") {

            QString var_name = tokens[__current_token_index + 1].value();
            QString expr = extractExpression(__current_token_index + 3, tokens);

            generateAssignmentCode(var_name, expr);

            __current_token_index += 4; // Skip "let", var, "=", and expr
        } else {
            __current_token_index++;
        }
    }

    void processAssignmentFromString(const QString& assignment) {
        QStringList parts = assignment.split(" ", Qt::SkipEmptyParts);

        if (parts.size() >= 3 && parts[1] == "=") {
            QString var_name = parts[0];
            if (parts[0] == "let" && parts.size() >= 4) {
                var_name = parts[1];
            }

            QString expr = "";
            for (int i = 2; i < parts.size(); i++) {
                expr += parts[i] + " ";
            }
            expr = expr.trimmed();

            generateAssignmentCode(var_name, expr);
        }
    }

    void processInput(const QList<Lexema>& tokens) {
        if (__current_token_index + 3 < tokens.size() &&
            tokens[__current_token_index + 1].value() == "(") {

            QString var_name = tokens[__current_token_index + 2].value();
            generateInputCode(var_name);

            __current_token_index += 4; // Skip "input", "(", var, ")"
        } else {
            __current_token_index++;
        }
    }

    void processOutput(const QList<Lexema>& tokens) {
        if (__current_token_index + 3 < tokens.size() &&
            tokens[__current_token_index + 1].value() == "(") {

            QString expr = extractExpression(__current_token_index + 2, tokens);
            generateOutputCode(expr);

            __current_token_index += 4; // Skip "output", "(", expr, ")"
        } else {
            __current_token_index++;
        }
    }

    QString extractCondition(const QList<Lexema>& tokens) {
        QString condition = "";
        int paren_count = 1;

        while (__current_token_index < tokens.size() && paren_count > 0) {
            if (tokens[__current_token_index].value() == "(") {
                paren_count++;
            } else if (tokens[__current_token_index].value() == ")") {
                paren_count--;
                if (paren_count == 0) {
                    break;
                }
            }

            if (paren_count == 1) {
                condition += tokens[__current_token_index].value() + " ";
            }

            __current_token_index++;
        }

        return condition.trimmed();
    }

    QString extractExpression(int start_index, const QList<Lexema>& tokens) {
        QString expr = "";
        int i = start_index;
        int paren_count = 0;

        while (i < tokens.size()) {
            QString val = tokens[i].value();

            if (val == "(") {
                paren_count++;
            } else if (val == ")") {
                if (paren_count == 0) {
                    break;
                }
                paren_count--;
            } else if (val == ";" && paren_count == 0) {
                break;
            }

            expr += val + " ";
            i++;
        }

        return expr.trimmed();
    }

    void closeCurrentBlock() {
        // Close loops first
        if (!__loop_contexts.isEmpty()) {
            LoopContext context = __loop_contexts.top();

            if (context.is_for_loop && !context.increment_code.isEmpty() && context.increment_code != "1") {
                // Generate increment code for for-loop
                __generated_code.append("    ; For loop increment");
                processAssignmentFromString(context.increment_code);
            }

            __generated_code.append("    jmp " + context.condition_label);
            __generated_code.append(context.end_label + ":");
            __generated_code.append("    ; Loop end");

            __loop_contexts.pop();
            __loop_depth--;
        }

        // Close if statements
        if (!__if_labels.isEmpty()) {
            QString end_if_label = __if_labels.pop();
            __generated_code.append(end_if_label + ":");
            __generated_code.append("    ; End if/else");
            __if_depth--;
        }
    }

    void closeAllBlocks() {
        while (!__loop_contexts.isEmpty()) {
            closeCurrentBlock();
        }

        while (!__if_labels.isEmpty()) {
            QString end_if_label = __if_labels.pop();
            __generated_code.append(end_if_label + ":");
            __generated_code.append("    ; End if/else");
        }
    }

    void generateInputCode(const QString& var_name) {
        __generated_code.append("");
        __generated_code.append(QString("    ; Input to %1").arg(var_name));

        // Simple inline input (without function call for simplicity)
        __generated_code.append(QString("    mov eax, 3          ; sys_read"));
        __generated_code.append(QString("    mov ebx, 0          ; stdin"));
        __generated_code.append(QString("    mov ecx, input_buffer"));
        __generated_code.append(QString("    mov edx, 12         ; buffer size"));
        __generated_code.append(QString("    int 0x80"));
        __generated_code.append(QString("    "));
        __generated_code.append(QString("    ; Convert string to integer"));
        __generated_code.append(QString("    mov esi, input_buffer"));
        __generated_code.append(QString("    xor eax, eax"));
        __generated_code.append(QString("    xor ebx, ebx"));
        __generated_code.append(QString("    mov ecx, 10"));
        __generated_code.append(QString("convert_input:"));
        __generated_code.append(QString("    mov bl, [esi]"));
        __generated_code.append(QString("    cmp bl, 0"));
        __generated_code.append(QString("    je convert_input_done"));
        __generated_code.append(QString("    cmp bl, 10         ; newline"));
        __generated_code.append(QString("    je convert_input_done"));
        __generated_code.append(QString("    sub bl, '0'"));
        __generated_code.append(QString("    imul eax, ecx"));
        __generated_code.append(QString("    add eax, ebx"));
        __generated_code.append(QString("    inc esi"));
        __generated_code.append(QString("    jmp convert_input"));
        __generated_code.append(QString("convert_input_done:"));
        __generated_code.append(QString("    mov [%1], eax").arg(var_name));
    }

    void generateOutputCode(const QString& expr) {
        __generated_code.append("");
        __generated_code.append(QString("    ; Output %1").arg(expr));

        // Evaluate expression
        generateExpressionCode(expr, "eax");

        // Convert to string and output
        __generated_code.append(QString("    ; Convert to string"));
        __generated_code.append(QString("    mov ebx, 10"));
        __generated_code.append(QString("    mov ecx, output_buffer"));
        __generated_code.append(QString("    add ecx, 11"));
        __generated_code.append(QString("    mov byte [ecx], 0"));
        __generated_code.append(QString("    dec ecx"));
        __generated_code.append(QString("    mov byte [ecx], 10  ; newline"));
        __generated_code.append(QString("    "));
        __generated_code.append(QString("    cmp eax, 0"));
        __generated_code.append(QString("    jge output_positive"));
        __generated_code.append(QString("    neg eax"));
        __generated_code.append(QString("    mov byte [output_buffer], '-'"));
        __generated_code.append(QString("output_positive:"));
        __generated_code.append(QString("output_convert:"));
        __generated_code.append(QString("    xor edx, edx"));
        __generated_code.append(QString("    div ebx"));
        __generated_code.append(QString("    add dl, '0'"));
        __generated_code.append(QString("    mov [ecx], dl"));
        __generated_code.append(QString("    dec ecx"));
        __generated_code.append(QString("    test eax, eax"));
        __generated_code.append(QString("    jnz output_convert"));
        __generated_code.append(QString("    "));
        __generated_code.append(QString("    inc ecx"));
        __generated_code.append(QString("    mov eax, 4          ; sys_write"));
        __generated_code.append(QString("    mov ebx, 1          ; stdout"));
        __generated_code.append(QString("    mov edx, 12"));
        __generated_code.append(QString("    sub edx, ecx"));
        __generated_code.append(QString("    add edx, output_buffer"));
        __generated_code.append(QString("    int 0x80"));
    }

    void generateAssignmentCode(const QString& var_name, const QString& expr) {
        __generated_code.append("");
        __generated_code.append(QString("    ; %1 = %2").arg(var_name, expr));

        generateExpressionCode(expr, "eax");
        __generated_code.append(QString("    mov [%1], eax").arg(var_name));
    }

    void generateConditionCode(const QString& condition, const QString& false_label) {
        __generated_code.append(QString("    ; Condition: %1").arg(condition));

        // Evaluate the condition expression
        generateExpressionCode(condition, "eax");

        // Check if result is zero (false)
        __generated_code.append(QString("    cmp eax, 0"));
        __generated_code.append(QString("    je %1").arg(false_label));
    }

    void generateExpressionCode(const QString& expr, const QString& dest_reg) {
        QStringList parts = expr.split(" ", Qt::SkipEmptyParts);

        if (parts.size() == 1) {
            // Single value
            bool is_number;
            parts[0].toInt(&is_number);

            if (is_number) {
                __generated_code.append(QString("    mov %1, %2").arg(dest_reg, parts[0]));
            } else {
                __generated_code.append(QString("    mov %1, [%2]").arg(dest_reg, parts[0]));
            }
        } else {
            // Complex expression - handle common patterns
            // For simplicity, handle basic arithmetic
            QString simplified = expr;
            simplified = simplified.replace("(", "").replace(")", "");
            QStringList tokens = simplified.split(" ", Qt::SkipEmptyParts);

            if (tokens.size() >= 3) {
                QString left = tokens[0];
                QString op = tokens[1];
                QString right = tokens[2];

                // Load left operand
                bool left_is_number;
                left.toInt(&left_is_number);

                if (left_is_number) {
                    __generated_code.append(QString("    mov %1, %2").arg(dest_reg, left));
                } else {
                    __generated_code.append(QString("    mov %1, [%2]").arg(dest_reg, left));
                }

                // Handle operation
                bool right_is_number;
                right.toInt(&right_is_number);

                if (op == "+") {
                    if (right_is_number) {
                        __generated_code.append(QString("    add %1, %2").arg(dest_reg, right));
                    } else {
                        __generated_code.append(QString("    add %1, [%2]").arg(dest_reg, right));
                    }
                }
                else if (op == "-") {
                    if (right_is_number) {
                        __generated_code.append(QString("    sub %1, %2").arg(dest_reg, right));
                    } else {
                        __generated_code.append(QString("    sub %1, [%2]").arg(dest_reg, right));
                    }
                }
                else if (op == "*") {
                    if (right_is_number) {
                        __generated_code.append(QString("    imul %1, %2").arg(dest_reg, right));
                    } else {
                        __generated_code.append(QString("    imul %1, [%2]").arg(dest_reg, right));
                    }
                }
                else if (op == "/") {
                    // For a/b, we need to handle division properly
                    __generated_code.append(QString("    cdq"));
                    if (right_is_number) {
                        __generated_code.append(QString("    mov ebx, %1").arg(right));
                    } else {
                        __generated_code.append(QString("    mov ebx, [%1]").arg(right));
                    }
                    __generated_code.append(QString("    idiv ebx"));
                    __generated_code.append(QString("    mov %1, eax").arg(dest_reg));
                }
            } else {
                // Simple expression - try to evaluate
                __generated_code.append(QString("    ; Expression: %1").arg(expr));
                __generated_code.append(QString("    mov %1, 0  ; placeholder").arg(dest_reg));
            }
        }
    }

    void generateHelperFunctions() {
        // Add .bss section for buffers
        __generated_code.append("");
        __generated_code.append("section .bss");
        __generated_code.append("    input_buffer resb 12");
        __generated_code.append("    output_buffer resb 12");

        // Declare all variables
        for (const QString& var : __declared_variables) {
            __generated_code.append(QString("    %1 resd 1").arg(var));
        }
    }

    bool writeToFile(const QString& filename) {
        QFile file(filename);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }

        QTextStream out(&file);
        for (const auto& line : __generated_code) {
            out << line << "\n";
        }

        file.close();
        return true;
    }
};

#endif // ASMGENERATOR_H
