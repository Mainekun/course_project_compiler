#include "parser.h"

[[nodiscard]] bool Parser::analyze() {
    __line.clear();
    __stack.clear();
    __conv_sequance.clear();
    __original_lexemas.clear();
    //__conv_seq.clear();

    if (__syntax_tree != nullptr) {
        delete __syntax_tree;
        __syntax_tree = nullptr;
    }

    qsizetype the_largest_rule = rules[0].len();
    foreach(auto& rule, rules) {
        if (the_largest_rule < rule.len())
            the_largest_rule = rule.len();
    }

    QList<Lexema> observe_list;

    int pos = 0;
    foreach(auto i, __lexer->get_tokenized_code()) {
        __line.push_back(i);
        __original_lexemas[pos++] = i;
    }

    __line.push_back(Lexema("$", TokenType::Delimeter));

    __stack.push(Lexema("^", TokenType::Delimeter));

    QStack<BlockNode*> block_stack;
    ProgramNode* current_program = nullptr;
    BlockNode* current_block = new BlockNode();

    while (!__line.isEmpty()) {

        Lexema stack_lex = __stack.top();
        if(stack_lex.type() == TokenType::Nonterminal)
            stack_lex = __stack.at(__stack.length()-2);

        Lexema line_lex = __line.front();

        QString line_lex_value = [&]()->QString{
            if ((int)line_lex.type() & ((int)TokenType::Id | (int)TokenType::Const))
                return "a";
            else
                return line_lex.value();
        }();

        QString stack_lex_value = [&]()->QString{
            if ((int)stack_lex.type() & ((int)TokenType::Id | (int)TokenType::Const))
                return "a";
            else
                return stack_lex.value();
        }();



        if (!(parser_rules.contains(line_lex_value) &&
              parser_rules[line_lex_value].contains(stack_lex_value)))
            throw std::runtime_error(QString("No relation specified for: (%1, %2)")
                                         .arg(line_lex_value)
                                         .arg(stack_lex.value())
                                         .toStdString());

        int rel = parser_rules[line_lex_value][stack_lex_value];

        if (rel <= 0) { //push
            __stack.push(line_lex);
            __line.pop_front();

        }
        else { //conv
            Rule the_best_rule = Rule();

            foreach (auto& rule, rules) {
                if (__stack.size() - 1 < rule.len())
                    continue;

                if (rule.check_stack(&__stack))
                    if (the_best_rule.len() < rule.len()) the_best_rule = rule;
            }

            if (the_best_rule.empty())
                throw std::runtime_error("NoRuleForSequanceException");

            QList<Lexema> buffer;
            for (int i = 0; i < the_best_rule.len(); i++)
                buffer.push_front(__stack.pop());

            __stack.push(the_best_rule());

            ASTNode* ast_node = buildASTFromSequence(the_best_rule.name(), buffer);

            // Handle the AST node based on rule type
            if (ast_node) {
                switch (ast_node->type) {
                case ASTNodeType::PROGRAM: {
                    current_program = dynamic_cast<ProgramNode*>(ast_node);
                    current_block = current_program->body;
                    break;
                }
                case ASTNodeType::VAR_DECL: {
                    VarDeclNode* var_decl = dynamic_cast<VarDeclNode*>(ast_node);
                    if (current_program) {
                        for (const auto& var : var_decl->variable_names) {
                            current_program->addVariable(var);
                        }
                    }
                    delete var_decl; // Variables stored in program
                    break;
                }
                case ASTNodeType::BLOCK: {
                    BlockNode* new_block = dynamic_cast<BlockNode*>(ast_node);
                    if (!block_stack.isEmpty()) {
                        // Add block to parent
                        block_stack.top()->addStatement(new_block);
                    }
                    block_stack.push(new_block);
                    current_block = new_block;
                    break;
                }
                case ASTNodeType::WHILE_LOOP:
                case ASTNodeType::FOR_LOOP:
                case ASTNodeType::IF_STATEMENT:
                case ASTNodeType::IF_ELSE: {
                    // These need their bodies to be filled later
                    __ast_stack.push(ast_node);
                    block_stack.push(new BlockNode());
                    current_block = block_stack.top();
                    break;
                }
                default: {
                    // Regular statement
                    if (current_block) {
                        current_block->addStatement(ast_node);
                    } else if (current_program) {
                        // If no current block (top-level statements), add directly to program
                        current_program->addStatement(ast_node);
                    }
                    break;
                }
                }
            }

            __conv_sequance.push_back({the_best_rule.name(), buffer});
            //__conv_seq.push_back(the_best_rule.name());

            rulewasfound:


            // Check for end of blocks
            if (the_best_rule.name() == "end" || the_best_rule.name() == "block_op") {
                if (!block_stack.isEmpty()) {
                    BlockNode* finished_block = block_stack.pop();

                    // If we have a control structure waiting for its body
                    if (!__ast_stack.isEmpty()) {
                        ASTNode* control_node = __ast_stack.pop();

                        switch (control_node->type) {
                        case ASTNodeType::WHILE_LOOP: {
                            WhileLoopNode* while_node = dynamic_cast<WhileLoopNode*>(control_node);
                            while_node->setBody(finished_block);
                            break;
                        }
                        case ASTNodeType::FOR_LOOP: {
                            ForLoopNode* for_node = dynamic_cast<ForLoopNode*>(control_node);
                            for_node->setBody(finished_block);
                            break;
                        }
                        case ASTNodeType::IF_STATEMENT: {
                            IfStatementNode* if_node = dynamic_cast<IfStatementNode*>(control_node);
                            if_node->then_block = finished_block;
                            break;
                        }
                        case ASTNodeType::IF_ELSE: {
                            // For if-else, we need to handle differently
                            // The else block should be on the stack
                            break;
                        }
                        default:
                            break;
                        }

                        // Add the completed control structure to current block
                        if (!block_stack.isEmpty()) {
                            block_stack.top()->addStatement(control_node);
                            current_block = block_stack.top();
                        } else if (current_program) {
                            current_program->body->addStatement(control_node);
                        }
                    } else {
                        // Regular block end
                        if (!block_stack.isEmpty()) {
                            block_stack.top()->addStatement(finished_block);
                            current_block = block_stack.top();
                        }
                    }
                }
            }
        }
    }

    __syntax_tree = current_program;

    if (!__conv_sequance.isEmpty()) {
        __semantic_analyzer.analyze(__conv_sequance);
    }


    return 1;
}

ASTNode* Parser::buildASTFromSequence(const QString& rule_name, const QList<Lexema>& sequence) {
    // qDebug() << "Building AST from rule:" << rule_name << "with sequence size:" << sequence.size();

    if (sequence.isEmpty()) return nullptr;

    // Handle different rule types
    if (rule_name == "program") {
        // program name var declarations begin statements end .
        if (sequence.size() >= 2) {
            QString prog_name = sequence[1].value();
            return new ProgramNode(prog_name);
        }
    }
    else if (rule_name == "description") {
        // VAR identifier_list : INTEGER
        QList<QString> variables;
        for (const auto& lex : sequence) {
            if (lex.type() == TokenType::Id) {
                variables.append(lex.value());
            }
        }
        return new VarDeclNode(variables);
    }
    else if (rule_name == "definition_op") {
        // LET identifier = expression
        if (sequence.size() >= 4) {
            QString var_name = sequence[1].value();

            // Debug: Print what we're parsing
            qDebug() << "Creating assignment for" << var_name << "with sequence:";
            for (const auto& lex : sequence) {
                qDebug() << "  -" << lex.value() << "type:" << (int)lex.type();
            }

            // Create expression from remaining tokens (starting from position 3)
            QList<Lexema> expr_seq;
            for (int i = 3; i < sequence.size(); i++) {
                expr_seq.append(sequence[i]);
            }

            ASTNode* expr_node = createExpressionNode(expr_seq);
            if (expr_node) {
                qDebug() << "Created expression node type:" << expr_node->typeName();
                return new AssignmentNode(var_name, expr_node);
            } else {
                qDebug() << "Failed to create expression node";
            }
        }
    }
    else if (rule_name == "input_op") {
        // INPUT ( identifier )
        if (sequence.size() >= 4) {
            QString var_name = sequence[2].value();
            return new InputNode(var_name);
        }
    }
    else if (rule_name == "output_op") {
        // OUTPUT ( expression )
        if (sequence.size() >= 4) {
            QList<Lexema> expr_seq = sequence.mid(2, sequence.size() - 3);
            ASTNode* expr_node = createExpressionNode(expr_seq);
            return new OutputNode(expr_node);
        }
    }
    else if (rule_name == "while_op") {
        // WHILE ( expression ) statement
        if (sequence.size() >= 5) {
            QList<Lexema> cond_seq = sequence.mid(2, 1);
            ASTNode* cond_node = createExpressionNode(cond_seq);
            return new WhileLoopNode(cond_node);
        }
    }
    else if (rule_name == "for_op") {
        // FOR ( initialization ; condition ; increment ) statement
        if (sequence.size() >= 9) {
            // Parse: FOR ( init ; cond ; inc ) body
            QList<Lexema> init_seq = sequence.mid(2, 1);  // initialization
            QList<Lexema> cond_seq = sequence.mid(4, 1);  // condition
            QList<Lexema> inc_seq = sequence.mid(6, 1);   // increment

            ASTNode* init_node = createExpressionNode(init_seq);
            ASTNode* cond_node = createExpressionNode(cond_seq);
            ASTNode* inc_node = createExpressionNode(inc_seq);

            return new ForLoopNode(init_node, cond_node, inc_node);
        }
    }
    else if (rule_name == "if_op") {
        // IF ( expression ) THEN statement
        if (sequence.size() >= 6) {
            QList<Lexema> cond_seq = sequence.mid(2, 1);
            ASTNode* cond_node = createExpressionNode(cond_seq);
            return new IfStatementNode(cond_node, nullptr);
        }
    }
    else if (rule_name == "if-else_op") {
        // IF ( expression ) THEN statement ELSE statement
        if (sequence.size() >= 8) {
            QList<Lexema> cond_seq = sequence.mid(2, 1);
            ASTNode* cond_node = createExpressionNode(cond_seq);
            return new IfElseNode(cond_node, nullptr, nullptr);
        }
    }
    else if (rule_name == "block_op") {
        // BEGIN statements END
        return new BlockNode();
    }
    else if (rule_name == "ops") {
        // Multiple statements - handled by block structure
        return nullptr;
    }
    else if (rule_name == "term_sum" || rule_name == "term_dif" ||
             rule_name == "factor_mul" || rule_name == "factor_div") {
        // Binary operations - create expression nodes
        if (sequence.size() == 3) {
            ASTNode* left = createExpressionNode(QList<Lexema>{sequence[0]});
            ASTNode* right = createExpressionNode(QList<Lexema>{sequence[2]});
            return new BinaryOpNode(sequence[1].value(), left, right);
        }
    }
    else if (rule_name == "atom") {
        // Single identifier or constant
        if (sequence.size() == 1) {
            return createExpressionNode(sequence);
        }
    }
    else if (rule_name == "neg_num") {
        // Negative number
        if (sequence.size() == 2) {
            // - constant
            if (sequence[1].type() == TokenType::Const) {
                bool ok;
                int value = sequence[1].value().toInt(&ok);
                if (ok) {
                    return new ConstantNode(-value);
                }
            }
        }
    }

    return nullptr;
}

ASTNode* Parser::createExpressionNode(const QList<Lexema>& operands) {


    if (operands.isEmpty()) return nullptr;

    if (operands.size() == 1) {
        const Lexema& lex = operands[0];

        // Check if this is an identifier
        for (int pos = 0; pos < __original_lexemas.size(); pos++) {
            const Lexema& original = __original_lexemas.value(pos);
            if (original.value() == lex.value() &&
                original.type() == TokenType::Id) {
                return new IdentifierNode(lex.value());
            }
        }

        // Check if this is a constant
        for (int pos = 0; pos < __original_lexemas.size(); pos++) {
            const Lexema& original = __original_lexemas.value(pos);
            if (original.value() == lex.value() &&
                original.type() == TokenType::Const) {
                bool ok;
                int value = lex.value().toInt(&ok);
                if (ok) {
                    return new ConstantNode(value);
                }
            }
        }

        // Default to identifier
        return new IdentifierNode(lex.value());
    }

    // Handle binary operations
    for (int i = 1; i < operands.size() - 1; i++) {
        if (operands[i].value() == "+" || operands[i].value() == "-" ||
            operands[i].value() == "*" || operands[i].value() == "/") {
            QList<Lexema> left_seq = operands.mid(0, i);
            QList<Lexema> right_seq = operands.mid(i + 1);
            ASTNode* left = createExpressionNode(left_seq);
            ASTNode* right = createExpressionNode(right_seq);
            return new BinaryOpNode(operands[i].value(), left, right);
        }
    }

    // Handle parentheses
    if (operands.size() >= 3 && operands[0].value() == "(" && operands.last().value() == ")") {
        return createExpressionNode(operands.mid(1, operands.size() - 2));
    }

    return nullptr;
}
