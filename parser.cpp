#include "parser.h"
#include <string>

[[nodiscard]] bool Parser::analyze() {
    qsizetype the_largest_rule = rules[0].len();
    foreach(auto& rule, rules) {
        if (the_largest_rule < rule.len())
            the_largest_rule = rule.len();
    }

    QList<Lexema> observe_list;

    foreach(auto i, __lexer->get_tokenized_code())
        __line.push_back(i);

    __line.push_back(Lexema("$", TokenType::Delimeter));

    __stack.push(Lexema("^", TokenType::Delimeter));

    while (!__line.isEmpty()) {

        Lexema stack_lex = __stack.top();
        if(stack_lex.type() == TokenType::Nonterminal)
            stack_lex = __stack.at(__stack.length()-2);

        Lexema line_lex = __line.front();

        if ((int)line_lex.type() & ((int)TokenType::Id | (int)TokenType::Const))
            line_lex = Lexema::A();

        if (!(parser_rules.contains(line_lex.value()) &&
              parser_rules[line_lex.value()].contains(stack_lex.value())))
            throw std::runtime_error(QString("No realtion specified for: (%1, %2)")
                                         .arg(line_lex.value())
                                         .arg(stack_lex.value())
                                         .toStdString());

        int rel = parser_rules[line_lex.value()][stack_lex.value()];

        if (rel <= 0) {
            __stack.push(line_lex);
            __line.pop_front();
        }
        else {
            Rule the_best_rule = Rule();

            foreach (auto& rule, rules) {
                if (__stack.size() - 1 < rule.len())
                    continue;

                if (rule.check_stack(&__stack))
                    if (the_best_rule.len() < rule.len()) the_best_rule = rule;
            }

            if (the_best_rule.empty())
                throw std::runtime_error("NoRuleForSequanceException");

            for (int i = 0; i < the_best_rule.len(); i++)
                __stack.pop();

            __stack.push(the_best_rule());

            rulewasfound:
        }
    }

    return 1;
}
