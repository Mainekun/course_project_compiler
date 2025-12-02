// asmgenerator.h
#ifndef ASMGENERATOR_H
#define ASMGENERATOR_H

#include "lexer.h"
#include <QMap>
#include <QFile>
#include <QTextStream>

class AsmGenerator {
private:
    Lexer* __lexer;
    QMap<QString, int> __variable_sizes; // variable name -> size in bytes
    QMap<QString, QString> __variable_types; // variable name -> type
    QList<QString> __generated_code;
    int __label_counter = 0;
    int __temp_counter = 0;

public:
    AsmGenerator(Lexer* lex) : __lexer(lex) {}

    bool generate(const QString& output_filename = "output.asm") {
        __generated_code.clear();
        __label_counter = 0;
        __temp_counter = 0;

        // Generate data section
        generateDataSection();

        // Generate code section
        generateCodeSection();

        // Write to file
        return writeToFile(output_filename);
    }

    QString getNextLabel() {
        return QString("L%1").arg(__label_counter++);
    }

    QString getNextTemp() {
        return QString("T%1").arg(__temp_counter++);
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

        // Process identifiers (variables)
        auto ids = __lexer->get_ids();
        for (const auto& lex : ids) {
            QString var_name = lex.value();

            // Check if this is declared as a variable (should be in var section in actual code)
            // For now, assume all ids are integer variables
            __variable_sizes[var_name] = 4; // 4 bytes for integer
            __variable_types[var_name] = "dd";

            __generated_code.append(QString("    %1 %2 0").arg(var_name, __variable_types[var_name]));
        }

        __generated_code.append("");
    }

    void generateCodeSection() {
        __generated_code.append("section .text");
        __generated_code.append("global _start");
        __generated_code.append("");
        __generated_code.append("_start:");
        __generated_code.append("");

        // Process tokenized code to generate instructions
        auto tokens = __lexer->get_tokenized_code();

        for (int i = 0; i < tokens.size(); i++) {
            auto& token = tokens[i];

            switch (token.type()) {
            case TokenType::Word:
                handleKeyword(token, i, tokens);
                break;
            case TokenType::Delimeter:
                handleDelimeter(token, i, tokens);
                break;
            case TokenType::Id:
                handleIdentifier(token, i, tokens);
                break;
            case TokenType::Const:
                handleConstant(token, i, tokens);
                break;
            default:
                break;
            }
        }

        // Add program exit
        __generated_code.append("");
        __generated_code.append("    ; Exit program");
        __generated_code.append("    mov eax, 1      ; sys_exit");
        __generated_code.append("    xor ebx, ebx    ; exit code 0");
        __generated_code.append("    int 0x80");
    }

    void handleKeyword(const Lexema& token, int index, const QList<Lexema>& tokens) {
        QString keyword = token.value();

        if (keyword == "program") {
            __generated_code.append("    ; Program: " + (index + 1 < tokens.size() ? tokens[index + 1].value() : ""));
        }
        else if (keyword == "var") {
            __generated_code.append("    ; Variable declarations");
        }
        else if (keyword == "begin") {
            __generated_code.append("    ; Begin main block");
        }
        else if (keyword == "end") {
            __generated_code.append("    ; End main block");
        }
        else if (keyword == "input") {
            // Generate input code
            if (index + 3 < tokens.size() && tokens[index + 1].value() == "(") {
                QString var_name = tokens[index + 2].value();
                generateInputCode(var_name);
            }
        }
        else if (keyword == "output") {
            // Generate output code
            if (index + 3 < tokens.size() && tokens[index + 1].value() == "(") {
                QString expr = tokens[index + 2].value();
                generateOutputCode(expr);
            }
        }
        else if (keyword == "let") {
            // Generate assignment code
            if (index + 3 < tokens.size() && tokens[index + 2].value() == "=") {
                QString var_name = tokens[index + 1].value();
                QString value = tokens[index + 3].value();
                generateAssignmentCode(var_name, value);
            }
        }
    }

    void handleDelimeter(const Lexema& token, int index, const QList<Lexema>& tokens) {
        QString delim = token.value();

        if (delim == ";") {
            __generated_code.append("    ; Statement end");
        }
        else if (delim == "=") {
            // Assignment operator - handled in keyword section
        }
        else if (Lexema::is_arithm(const_cast<Lexema&>(token))) {
            // Arithmetic operation
            if (index > 0 && index + 1 < tokens.size()) {
                QString left = tokens[index - 1].value();
                QString right = tokens[index + 1].value();
                generateArithmeticCode(left, delim, right);
            }
        }
    }

    void handleIdentifier(const Lexema& token, int index, const QList<Lexema>& tokens) {
        // Mostly handled in other contexts
    }

    void handleConstant(const Lexema& token, int index, const QList<Lexema>& tokens) {
        // Mostly handled in other contexts
    }

    void generateInputCode(const QString& var_name) {
        QString temp_label = getNextLabel();

        __generated_code.append("");
        __generated_code.append("    ; Input to " + var_name);
        __generated_code.append("    push dword " + var_name + "  ; push variable address");
        __generated_code.append("    push dword 10             ; buffer size");
        __generated_code.append("    call read_input");
        __generated_code.append("    add esp, 8               ; clean stack");
        __generated_code.append("    mov [" + var_name + "], eax  ; store result");
    }

    void generateOutputCode(const QString& expr) {
        __generated_code.append("");
        __generated_code.append("    ; Output " + expr);

        // Check if expr is a constant or variable
        bool is_number;
        expr.toInt(&is_number);

        if (is_number) {
            // Constant value
            __generated_code.append("    push dword " + expr);
        } else {
            // Variable
            __generated_code.append("    push dword [" + expr + "]");
        }

        __generated_code.append("    call print_number");
        __generated_code.append("    add esp, 4               ; clean stack");
    }

    void generateAssignmentCode(const QString& var_name, const QString& value) {
        __generated_code.append("");
        __generated_code.append("    ; " + var_name + " = " + value);

        bool is_number;
        int num_value = value.toInt(&is_number);

        if (is_number) {
            // Direct assignment of constant
            __generated_code.append("    mov dword [" + var_name + "], " + value);
        } else {
            // Assignment from another variable
            __generated_code.append("    mov eax, [" + value + "]");
            __generated_code.append("    mov [" + var_name + "], eax");
        }
    }

    void generateArithmeticCode(const QString& left, const QString& op, const QString& right) {
        QString temp_result = getNextTemp();

        __generated_code.append("");
        __generated_code.append("    ; " + left + " " + op + " " + right);

        // Load left operand
        bool left_is_number;
        left.toInt(&left_is_number);

        if (left_is_number) {
            __generated_code.append("    mov eax, " + left);
        } else {
            __generated_code.append("    mov eax, [" + left + "]");
        }

        // Perform operation with right operand
        bool right_is_number;
        right.toInt(&right_is_number);

        if (op == "+") {
            if (right_is_number) {
                __generated_code.append("    add eax, " + right);
            } else {
                __generated_code.append("    add eax, [" + right + "]");
            }
        }
        else if (op == "-") {
            if (right_is_number) {
                __generated_code.append("    sub eax, " + right);
            } else {
                __generated_code.append("    sub eax, [" + right + "]");
            }
        }
        else if (op == "*") {
            if (right_is_number) {
                __generated_code.append("    imul eax, " + right);
            } else {
                __generated_code.append("    imul eax, [" + right + "]");
            }
        }
        else if (op == "/") {
            __generated_code.append("    cdq");  // Sign extend EAX into EDX:EAX
            if (right_is_number) {
                __generated_code.append("    mov ebx, " + right);
            } else {
                __generated_code.append("    mov ebx, [" + right + "]");
            }
            __generated_code.append("    idiv ebx");
        }

        // Store result in temporary
        __generated_code.append("    mov [" + temp_result + "], eax");
    }

    bool writeToFile(const QString& filename) {
        QFile file(filename);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }

        QTextStream out(&file);

        // Add helper functions for I/O
        out << "; Helper functions for I/O operations\n";
        out << "read_input:\n";
        out << "    ; Read integer from stdin\n";
        out << "    push ebp\n";
        out << "    mov ebp, esp\n";
        out << "    sub esp, 12\n";
        out << "    \n";
        out << "    mov eax, 3          ; sys_read\n";
        out << "    mov ebx, 0          ; stdin\n";
        out << "    mov ecx, [ebp+12]   ; buffer\n";
        out << "    mov edx, [ebp+8]    ; length\n";
        out << "    int 0x80\n";
        out << "    \n";
        out << "    ; Convert string to integer\n";
        out << "    mov esi, [ebp+12]\n";
        out << "    xor eax, eax\n";
        out << "    xor ebx, ebx\n";
        out << "    xor ecx, ecx\n";
        out << "    mov edi, 10\n";
        out << "convert_loop:\n";
        out << "    mov bl, [esi]\n";
        out << "    cmp bl, 0\n";
        out << "    je convert_done\n";
        out << "    sub bl, '0'\n";
        out << "    imul eax, edi\n";
        out << "    add eax, ebx\n";
        out << "    inc esi\n";
        out << "    jmp convert_loop\n";
        out << "convert_done:\n";
        out << "    \n";
        out << "    mov esp, ebp\n";
        out << "    pop ebp\n";
        out << "    ret\n";
        out << "\n";

        out << "print_number:\n";
        out << "    ; Print integer to stdout\n";
        out << "    push ebp\n";
        out << "    mov ebp, esp\n";
        out << "    sub esp, 12\n";
        out << "    \n";
        out << "    mov eax, [ebp+8]    ; number to print\n";
        out << "    lea ecx, [ebp-12]   ; buffer\n";
        out << "    mov ebx, 10\n";
        out << "    mov edi, ecx\n";
        out << "    add edi, 11\n";
        out << "    mov byte [edi], 0\n";
        out << "    dec edi\n";
        out << "    mov byte [edi], 10  ; newline\n";
        out << "    \n";
        out << "    cmp eax, 0\n";
        out << "    jge convert_pos\n";
        out << "    neg eax\n";
        out << "    mov byte [ecx], '-'\n";
        out << "    inc ecx\n";
        out << "convert_pos:\n";
        out << "    xor edx, edx\n";
        out << "    div ebx\n";
        out << "    add dl, '0'\n";
        out << "    mov [edi], dl\n";
        out << "    dec edi\n";
        out << "    test eax, eax\n";
        out << "    jnz convert_pos\n";
        out << "    \n";
        out << "    inc edi\n";
        out << "    mov eax, 4          ; sys_write\n";
        out << "    mov ebx, 1          ; stdout\n";
        out << "    mov ecx, edi\n";
        out << "    mov edx, 12\n";
        out << "    sub edx, edi\n";
        out << "    add edx, [ebp-12]\n";
        out << "    int 0x80\n";
        out << "    \n";
        out << "    mov esp, ebp\n";
        out << "    pop ebp\n";
        out << "    ret\n";
        out << "\n";

        // Write generated code
        for (const auto& line : __generated_code) {
            out << line << "\n";
        }

        file.close();
        return true;
    }
};

#endif // ASMGENERATOR_H
