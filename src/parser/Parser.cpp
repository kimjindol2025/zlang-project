#include "Parser.h"
#include <iostream>
#include <cassert>

namespace zlang {

// ============================================================================
// 생성자
// ============================================================================

Parser::Parser(const std::vector<Token>& tok)
    : tokens(tok), current(0) {
}

// ============================================================================
// 【 진입점: parse() 】
// ============================================================================

std::shared_ptr<ProgramNode> Parser::parse() {
    return parseProgram();
}

// ============================================================================
// 【 Program & Function 파싱 】
// ============================================================================

std::shared_ptr<ProgramNode> Parser::parseProgram() {
    auto program = std::make_shared<ProgramNode>();

    while (!isAtEnd()) {
        if (check(TokenType::KW_FN)) {
            auto func = parseFunction();
            if (func) {
                program->functions.push_back(func);
            }
        } else {
            reportError("Expected 'fn' at top level");
            synchronize();
            advance();
        }
    }

    return program;
}

std::shared_ptr<FunctionNode> Parser::parseFunction() {
    if (!match(TokenType::KW_FN)) {
        reportError("Expected 'fn'");
        return nullptr;
    }

    Token name_token = consume(TokenType::IDENTIFIER, "Expected function name");
    std::string func_name = name_token.lexeme;

    consume(TokenType::LPAREN, "Expected '(' after function name");
    auto params = parseParameters();
    consume(TokenType::RPAREN, "Expected ')' after parameters");

    consume(TokenType::ARROW, "Expected '->' before return type");
    Type return_type = parseType();

    auto block = parseBlock();
    if (!block) {
        return nullptr;
    }

    return std::make_shared<FunctionNode>(func_name, params, return_type, block);
}

std::vector<Parameter> Parser::parseParameters() {
    std::vector<Parameter> params;

    if (check(TokenType::RPAREN)) {
        return params;  // 파라미터 없음
    }

    do {
        Token param_name = consume(TokenType::IDENTIFIER, "Expected parameter name");
        consume(TokenType::COLON, "Expected ':' after parameter name");
        Type param_type = parseType();

        params.push_back({param_name.lexeme, param_type});

        if (!match(TokenType::COMMA)) {
            break;
        }
    } while (!check(TokenType::RPAREN));

    return params;
}

Type Parser::parseType() {
    Type type;

    // 포인터 타입: &T
    if (match(TokenType::AMPERSAND)) {
        type.is_pointer = true;
        auto pointee = std::make_unique<Type>(parseType());
        type.pointee_type = pointee.release();
        return type;
    }

    // 배열 타입: [T; N]
    if (match(TokenType::LBRACKET)) {
        type.is_array = true;
        auto elem_type = std::make_unique<Type>(parseType());
        type.element_type = elem_type.release();

        consume(TokenType::SEMICOLON, "Expected ';' in array type");

        Token size_token = consume(TokenType::INTEGER, "Expected array size");
        type.array_size = std::stoi(size_token.lexeme);

        consume(TokenType::RBRACKET, "Expected ']' after array type");
        return type;
    }

    // 기본 타입
    type.base = tokenTypeToBuiltinType(peek().type);

    if (type.base == BuiltinType::Unknown) {
        // 커스텀 타입일 수 있음 (현재는 지원하지 않음)
        reportError("Unknown type: " + peek().lexeme);
    }

    advance();
    return type;
}

// ============================================================================
// 【 Statement 파싱 】
// ============================================================================

std::shared_ptr<ASTNode> Parser::parseStatement() {
    if (check(TokenType::KW_LET)) {
        return parseVarDecl();
    }
    if (check(TokenType::KW_IF)) {
        return parseIfStatement();
    }
    if (check(TokenType::KW_WHILE)) {
        return parseWhileStatement();
    }
    if (check(TokenType::KW_RETURN)) {
        return parseReturnStatement();
    }
    // 【 Step 3: try-catch-finally 파싱 】
    if (check(TokenType::KW_TRY)) {
        return parseTryCatch();
    }
    if (check(TokenType::LBRACE)) {
        return parseBlock();
    }

    // Expression statement
    auto expr = parseExpression();
    match(TokenType::SEMICOLON);
    return expr;
}

std::shared_ptr<VarDeclNode> Parser::parseVarDecl() {
    if (!match(TokenType::KW_LET)) {
        reportError("Expected 'let'");
        return nullptr;
    }

    Token var_name = consume(TokenType::IDENTIFIER, "Expected variable name");
    consume(TokenType::COLON, "Expected ':' after variable name");

    Type var_type = parseType();

    consume(TokenType::ASSIGN, "Expected '=' after type");
    auto init_expr = parseExpression();

    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration");

    auto var_decl = std::make_shared<VarDeclNode>(var_name.lexeme, var_type, init_expr);

    return var_decl;
}

std::shared_ptr<BlockNode> Parser::parseBlock() {
    if (!match(TokenType::LBRACE)) {
        reportError("Expected '{'");
        return nullptr;
    }

    auto block = std::make_shared<BlockNode>();

    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        auto stmt = parseStatement();
        if (stmt) {
            block->statements.push_back(stmt);
        }
    }

    consume(TokenType::RBRACE, "Expected '}'");
    return block;
}

std::shared_ptr<IfNode> Parser::parseIfStatement() {
    if (!match(TokenType::KW_IF)) {
        reportError("Expected 'if'");
        return nullptr;
    }

    auto cond = parseExpression();
    auto then_block = parseBlock();

    if (!then_block) {
        return nullptr;
    }

    std::shared_ptr<BlockNode> else_block = nullptr;
    if (match(TokenType::KW_ELSE)) {
        else_block = parseBlock();
    }

    return std::make_shared<IfNode>(cond, then_block, else_block);
}

std::shared_ptr<WhileNode> Parser::parseWhileStatement() {
    if (!match(TokenType::KW_WHILE)) {
        reportError("Expected 'while'");
        return nullptr;
    }

    auto cond = parseExpression();
    auto body = parseBlock();

    if (!body) {
        return nullptr;
    }

    return std::make_shared<WhileNode>(cond, body);
}

std::shared_ptr<ReturnNode> Parser::parseReturnStatement() {
    if (!match(TokenType::KW_RETURN)) {
        reportError("Expected 'return'");
        return nullptr;
    }

    std::shared_ptr<ASTNode> return_expr = nullptr;

    // return 뒤에 식이 없을 수도 있음
    if (!check(TokenType::SEMICOLON)) {
        return_expr = parseExpression();
    }

    consume(TokenType::SEMICOLON, "Expected ';' after return statement");

    auto ret = std::make_shared<ReturnNode>();
    ret->value = return_expr;

    return ret;
}

// ============================================================================
// 【 Expression 파싱 (우선순위 처리) 】
// ============================================================================

std::shared_ptr<ASTNode> Parser::parseExpression() {
    return parseAssignment();
}

std::shared_ptr<ASTNode> Parser::parseAssignment() {
    auto expr = parseLogicalOr();

    if (match(TokenType::ASSIGN)) {
        auto right = parseAssignment();
        return std::make_shared<BinaryOpNode>(BinaryOp::Assign, expr, right);
    }

    return expr;
}

std::shared_ptr<ASTNode> Parser::parseLogicalOr() {
    auto left = parseLogicalAnd();

    while (match(TokenType::OR)) {
        auto right = parseLogicalAnd();
        left = std::make_shared<BinaryOpNode>(BinaryOp::Or, left, right);
    }

    return left;
}

std::shared_ptr<ASTNode> Parser::parseLogicalAnd() {
    auto left = parseEquality();

    while (match(TokenType::AND)) {
        auto right = parseEquality();
        left = std::make_shared<BinaryOpNode>(BinaryOp::And, left, right);
    }

    return left;
}

std::shared_ptr<ASTNode> Parser::parseEquality() {
    auto left = parseRelational();

    while (check(TokenType::EQ) || check(TokenType::NE)) {
        TokenType op_type = advance().type;
        auto right = parseRelational();
        left = std::make_shared<BinaryOpNode>(tokenTypeToBinaryOp(op_type), left, right);
    }

    return left;
}

std::shared_ptr<ASTNode> Parser::parseRelational() {
    auto left = parseAdditive();

    while (check(TokenType::LT) || check(TokenType::GT) ||
           check(TokenType::LE) || check(TokenType::GE)) {
        TokenType op_type = advance().type;
        auto right = parseAdditive();
        left = std::make_shared<BinaryOpNode>(tokenTypeToBinaryOp(op_type), left, right);
    }

    return left;
}

std::shared_ptr<ASTNode> Parser::parseAdditive() {
    auto left = parseMultiplicative();

    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        TokenType op_type = advance().type;
        auto right = parseMultiplicative();
        left = std::make_shared<BinaryOpNode>(tokenTypeToBinaryOp(op_type), left, right);
    }

    return left;
}

std::shared_ptr<ASTNode> Parser::parseMultiplicative() {
    auto left = parseUnary();

    while (check(TokenType::STAR) || check(TokenType::SLASH) ||
           check(TokenType::PERCENT)) {
        TokenType op_type = advance().type;
        auto right = parseUnary();
        left = std::make_shared<BinaryOpNode>(tokenTypeToBinaryOp(op_type), left, right);
    }

    return left;
}

std::shared_ptr<ASTNode> Parser::parseUnary() {
    if (check(TokenType::NOT) || check(TokenType::MINUS) ||
        check(TokenType::AMPERSAND)) {
        TokenType op_type = advance().type;
        auto operand = parseUnary();
        return std::make_shared<UnaryOpNode>(tokenTypeToUnaryOp(op_type), operand);
    }

    return parsePostfix();
}

std::shared_ptr<ASTNode> Parser::parsePostfix() {
    auto expr = parsePrimary();

    while (true) {
        if (match(TokenType::LBRACKET)) {
            // 배열 인덱싱 (현재는 미지원)
            auto index = parseExpression();
            consume(TokenType::RBRACKET, "Expected ']'");
            reportError("Array indexing not yet supported");
            return expr;
        } else if (match(TokenType::LPAREN)) {
            // 함수 호출
            auto args = parseArguments();
            consume(TokenType::RPAREN, "Expected ')'");

            if (auto ident = std::dynamic_pointer_cast<IdentifierNode>(expr)) {
                expr = std::make_shared<CallNode>(ident->name, args);
            } else {
                reportError("Can only call identifiers");
            }
        } else {
            break;
        }
    }

    return expr;
}

std::shared_ptr<ASTNode> Parser::parsePrimary() {
    // 리터럴
    if (check(TokenType::INTEGER)) {
        Token token = advance();
        auto lit = std::make_shared<IntLiteralNode>(std::stoll(token.lexeme));
        return lit;
    }

    if (check(TokenType::FLOAT)) {
        Token token = advance();
        auto lit = std::make_shared<FloatLiteralNode>(std::stod(token.lexeme));
        return lit;
    }

    if (check(TokenType::STRING)) {
        Token token = advance();
        auto lit = std::make_shared<StringLiteralNode>(token.lexeme);
        return lit;
    }

    if (match(TokenType::KW_TRUE)) {
        auto lit = std::make_shared<BoolLiteralNode>(true);
        return lit;
    }

    if (match(TokenType::KW_FALSE)) {
        auto lit = std::make_shared<BoolLiteralNode>(false);
        return lit;
    }

    // 식별자
    if (check(TokenType::IDENTIFIER)) {
        Token token = advance();
        return std::make_shared<IdentifierNode>(token.lexeme);
    }

    // 괄호 식
    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')'");
        return expr;
    }

    reportError("Unexpected token: " + peek().lexeme);
    return nullptr;
}

std::vector<std::shared_ptr<ASTNode>> Parser::parseArguments() {
    std::vector<std::shared_ptr<ASTNode>> args;

    if (check(TokenType::RPAREN)) {
        return args;
    }

    do {
        args.push_back(parseExpression());
    } while (match(TokenType::COMMA));

    return args;
}

// ============================================================================
// 【 헬퍼 메서드 】
// ============================================================================

Token Parser::peek() const {
    if (current < tokens.size()) {
        return tokens[current];
    }
    return tokens.back();  // EOF
}

Token Parser::peekNext() const {
    if (current + 1 < tokens.size()) {
        return tokens[current + 1];
    }
    return tokens.back();  // EOF
}

bool Parser::check(TokenType type) const {
    return peek().type == type;
}

bool Parser::checkAny(const std::vector<TokenType>& types) const {
    for (const auto& t : types) {
        if (check(t)) return true;
    }
    return false;
}

Token Parser::advance() {
    Token token = peek();
    if (!isAtEnd()) {
        current++;
    }
    return token;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) {
        return advance();
    }
    reportError(message + " at line " + std::to_string(peek().line));
    return Token();
}

bool Parser::isAtEnd() const {
    return check(TokenType::END_OF_FILE);
}

void Parser::reportError(const std::string& message) {
    errors.push_back("[Line " + std::to_string(peek().line) + "] " + message);
}

void Parser::synchronize() {
    advance();

    while (!isAtEnd()) {
        if (check(TokenType::KW_FN) || check(TokenType::KW_LET) ||
            check(TokenType::KW_RETURN) || check(TokenType::KW_IF) ||
            check(TokenType::KW_WHILE)) {
            return;
        }
        advance();
    }
}

BinaryOp Parser::tokenTypeToBinaryOp(TokenType type) {
    switch (type) {
        case TokenType::PLUS:  return BinaryOp::Add;
        case TokenType::MINUS: return BinaryOp::Sub;
        case TokenType::STAR:  return BinaryOp::Mul;
        case TokenType::SLASH: return BinaryOp::Div;
        case TokenType::PERCENT: return BinaryOp::Mod;
        case TokenType::EQ:    return BinaryOp::Equal;
        case TokenType::NE:    return BinaryOp::NotEq;
        case TokenType::LT:    return BinaryOp::Less;
        case TokenType::GT:    return BinaryOp::Greater;
        case TokenType::LE:    return BinaryOp::LessEq;
        case TokenType::GE:    return BinaryOp::GreaterEq;
        case TokenType::AND:   return BinaryOp::And;
        case TokenType::OR:    return BinaryOp::Or;
        case TokenType::ASSIGN: return BinaryOp::Assign;
        default:               return BinaryOp::Add;  // 기본값
    }
}

UnaryOp Parser::tokenTypeToUnaryOp(TokenType type) {
    switch (type) {
        case TokenType::NOT:       return UnaryOp::Not;
        case TokenType::MINUS:     return UnaryOp::Neg;  // Use Neg instead
        case TokenType::AMPERSAND: return UnaryOp::Address;
        default:                   return UnaryOp::Not;  // 기본값
    }
}

BuiltinType Parser::tokenTypeToBuiltinType(TokenType type) {
    switch (type) {
        case TokenType::KW_I32:    return BuiltinType::I32;
        case TokenType::KW_I64:    return BuiltinType::I64;
        case TokenType::KW_F32:    return BuiltinType::F32;
        case TokenType::KW_F64:    return BuiltinType::F64;
        case TokenType::KW_BOOL:   return BuiltinType::Bool;
        case TokenType::KW_VOID:   return BuiltinType::Void;
        case TokenType::KW_STRING: return BuiltinType::String;
        default:                   return BuiltinType::Unknown;
    }
}

// ============================================================================
// 【 Step 3: try-catch-finally 파싱 】
// ============================================================================

std::shared_ptr<TryCatchNode> Parser::parseTryCatch() {
    if (!match(TokenType::KW_TRY)) {
        reportError("Expected 'try'");
        return nullptr;
    }

    // try 블록 파싱
    auto try_block = parseBlock();
    if (!try_block) {
        reportError("parseTryCatch: Failed to parse try block");
        return nullptr;
    }

    // catch 블록들 파싱
    std::vector<CatchClause> catch_clauses;
    while (check(TokenType::KW_CATCH)) {
        if (!match(TokenType::KW_CATCH)) break;

        // catch (e: ExceptionType) 파싱
        if (!match(TokenType::LPAREN)) {
            reportError("Expected '(' in catch clause");
            break;
        }

        Token var_token = consume(TokenType::IDENTIFIER, "Expected exception variable");
        consume(TokenType::COLON, "Expected ':' after variable");
        Token type_token = consume(TokenType::IDENTIFIER, "Expected exception type");

        if (!match(TokenType::RPAREN)) {
            reportError("Expected ')' in catch clause");
            break;
        }

        // catch 블록 파싱
        auto catch_block = parseBlock();
        if (!catch_block) {
            reportError("parseTryCatch: Failed to parse catch block");
            break;
        }

        catch_clauses.push_back({
            var_token.lexeme,
            type_token.lexeme,
            catch_block
        });
    }

    // finally 블록 파싱 (선택사항)
    std::shared_ptr<BlockNode> finally_block = nullptr;
    if (check(TokenType::KW_FINALLY)) {
        if (!match(TokenType::KW_FINALLY)) {
            reportError("Expected 'finally'");
        }
        finally_block = parseBlock();
        if (!finally_block) {
            reportError("parseTryCatch: Failed to parse finally block");
        }
    }

    return std::make_shared<TryCatchNode>(try_block, catch_clauses, finally_block);
}

} // namespace zlang
