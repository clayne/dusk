#include "expression.hh"

ExpressionAtomic::ExpressionAtomic(long long num) {
    this->number = num;
    this->type = t_number;
}

ExpressionAtomic::ExpressionAtomic(double num) {
    this->floating = num;
    this->type = t_float;
}

ExpressionAtomic::ExpressionAtomic(std::string str, bool is_identifier) {
    this->str = (is_identifier)?std::string(str):std::string(str.substr(1, str.length() - 2));
    this->type = (is_identifier)?t_identifier:t_string;
}

ExpressionAtomic::ExpressionAtomic(bool b) {
    this->boolean = b;
    this->type = t_bool;
}

ExpressionAtomic::ExpressionAtomic() {
    this->type = t_null;
}

ExpressionAtomic::ExpressionAtomic(std::string str, std::vector<Expression*> args) {
    this->str = std::string(str);
    this->args = args;
    this->type = t_function_call;
}

void ExpressionAtomic::debug(size_t depth) {
    switch (this->type) {
        case t_number: std::cout << std::string(depth * 4, ' ') << this->number << " : int"<< std::endl; break;
        case t_float: std::cout << std::string(depth * 4, ' ') << this->floating << " : float" << std::endl; break;
        case t_string: std::cout << std::string(depth * 4, ' ') << this->str << " - len = " << this->str.length() << " : str" << std::endl; break;
        case t_identifier: std::cout << std::string(depth * 4, ' ') << this->str << " - len = " << this->str.length() << " : identifier" << std::endl; break;
        case t_null: std::cout << std::string(depth * 4, ' ') << "null : null" << std::endl; break;
        case t_bool: std::cout << std::string(depth * 4, ' ') << ((this->boolean)?"True : bool":"False : bool") << std::endl; break;
        case t_function_call: std::cout << std::string(depth * 4, ' ') << "call " << this->str << " : " << this->args.size() << " args" << std::endl; for (size_t i = 0; i < this->args.size(); i++) this->args[i]->debug(depth + 1); break;
    }
}

BinaryExpression::BinaryExpression(Expression* l, Expression* r, std::string t) {
    this->lhs = l;
    this->rhs = r;
    this->type = t;
}

void BinaryExpression::debug(size_t depth) {
    std::cout << std::string(depth * 4, ' ') << "binary " << this->type << std::endl;
    this->lhs->debug(depth + 1);
    this->rhs->debug(depth + 1);
    
}

UnaryExpression::UnaryExpression(Expression* expr, std::string op) {
    this->operand = expr;
    this->op = op;
}

void UnaryExpression::debug(size_t depth) {
    std::cout << std::string(depth * 4, ' ') << "unary " << this->op << std::endl;
    this->operand->debug(depth + 1);
    
}
AssignmentExpression::AssignmentExpression(Expression* identifier, Expression* value) {
    this->identifier = identifier;
    this->value = value;
}

void AssignmentExpression::debug(size_t depth) {
    std::cout << std::string(depth * 4, ' ') << ((ExpressionAtomic*)this->identifier)->str << " = " << std::endl;
    this->value->debug(depth + 1);
}
