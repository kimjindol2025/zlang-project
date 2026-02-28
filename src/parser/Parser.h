#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <memory>
#include <string>
#include "../lexer/Token.h"
#include "../ast/ASTNode.h"

namespace zlang {

/**
 * Parser: Token 스트림을 AST(Abstract Syntax Tree)로 변환
 *
 * 입력: Token 벡터 (Lexer 출력)
 * 출력: ProgramNode (AST 트리)
 *
 * 파싱 전략: 재귀적 하강 파서 (Recursive Descent Parser)
 *
 * 문법:
 *   program       → function*
 *   function      → 'fn' IDENTIFIER '(' parameters? ')' '->' type block
 *   parameters    → parameter (',' parameter)*
 *   parameter     → IDENTIFIER ':' type
 *   block         → '{' statement* '}'
 *   statement     → varDecl | ifStmt | whileStmt | returnStmt | exprStmt | blockStmt
 *   varDecl       → 'let' IDENTIFIER ':' type '=' expression ';'
 *   ifStmt        → 'if' expression block ('else' block)?
 *   whileStmt     → 'while' expression block
 *   returnStmt    → 'return' expression? ';'
 *   exprStmt      → expression ';'
 *   blockStmt     → block
 *   expression    → assignment
 *   assignment    → logicalOr ('=' logicalOr)*
 *   logicalOr     → logicalAnd ('||' logicalAnd)*
 *   logicalAnd    → equality ('&&' equality)*
 *   equality      → relational (('==' | '!=') relational)*
 *   relational    → additive (('<' | '>' | '<=' | '>=') additive)*
 *   additive      → multiplicative (('+' | '-') multiplicative)*
 *   multiplicative → unary (('*' | '/' | '%') unary)*
 *   unary         → ('!' | '-' | '&') unary | postfix
 *   postfix       → primary ('[' expression ']' | '(' arguments? ')')*
 *   primary       → literal | IDENTIFIER | '(' expression ')' | '[' typeAndSize ']'
 *   type          → IDENTIFIER | '[' type ';' INTEGER ']' | '&' type
 *   literal       → INTEGER | FLOAT | STRING | 'true' | 'false'
 */
class Parser {
private:
    std::vector<Token> tokens;
    size_t current = 0;
    std::vector<std::string> errors;

public:
    Parser(const std::vector<Token>& tok);

    /**
     * 진입점: 토큰 스트림을 AST로 변환
     */
    std::shared_ptr<ProgramNode> parse();

    /**
     * 에러 메시지
     */
    const std::vector<std::string>& getErrors() const { return errors; }

private:
    // ========================================================================
    // 【 Program & Function 파싱 】
    // ========================================================================

    std::shared_ptr<ProgramNode> parseProgram();
    std::shared_ptr<FunctionNode> parseFunction();
    std::vector<Parameter> parseParameters();
    Type parseType();

    // ========================================================================
    // 【 Statement 파싱 】
    // ========================================================================

    std::shared_ptr<ASTNode> parseStatement();
    std::shared_ptr<VarDeclNode> parseVarDecl();
    std::shared_ptr<BlockNode> parseBlock();
    std::shared_ptr<IfNode> parseIfStatement();
    std::shared_ptr<WhileNode> parseWhileStatement();
    std::shared_ptr<ReturnNode> parseReturnStatement();

    // 【 Step 3: Exception Handling 】
    std::shared_ptr<TryCatchNode> parseTryCatch();

    // ========================================================================
    // 【 Expression 파싱 (우선순위 처리) 】
    // ========================================================================

    std::shared_ptr<ASTNode> parseExpression();
    std::shared_ptr<ASTNode> parseAssignment();
    std::shared_ptr<ASTNode> parseLogicalOr();
    std::shared_ptr<ASTNode> parseLogicalAnd();
    std::shared_ptr<ASTNode> parseEquality();
    std::shared_ptr<ASTNode> parseRelational();
    std::shared_ptr<ASTNode> parseAdditive();
    std::shared_ptr<ASTNode> parseMultiplicative();
    std::shared_ptr<ASTNode> parseUnary();
    std::shared_ptr<ASTNode> parsePostfix();
    std::shared_ptr<ASTNode> parsePrimary();
    std::vector<std::shared_ptr<ASTNode>> parseArguments();

    // ========================================================================
    // 【 헬퍼 메서드 】
    // ========================================================================

    /**
     * 현재 토큰 반환
     */
    Token peek() const;

    /**
     * 다음 토큰 확인
     */
    Token peekNext() const;

    /**
     * 현재 토큰이 특정 타입인지 확인
     */
    bool check(TokenType type) const;

    /**
     * 현재 토큰이 여러 타입 중 하나인지 확인
     */
    bool checkAny(const std::vector<TokenType>& types) const;

    /**
     * 현재 토큰을 반환하고 다음으로 이동
     */
    Token advance();

    /**
     * 현재 토큰이 특정 타입이면 advance하고 true 반환
     */
    bool match(TokenType type);

    /**
     * 현재 토큰이 특정 타입이면 advance하고 토큰 반환, 아니면 에러
     */
    Token consume(TokenType type, const std::string& message);

    /**
     * EOF 도달 여부 확인
     */
    bool isAtEnd() const;

    /**
     * 에러 기록
     */
    void reportError(const std::string& message);

    /**
     * 파싱 오류 복구 (동기화)
     */
    void synchronize();

    /**
     * 토큰 타입을 이항 연산자 타입으로 변환
     */
    BinaryOp tokenTypeToBinaryOp(TokenType type);

    /**
     * 토큰 타입을 단항 연산자 타입으로 변환
     */
    UnaryOp tokenTypeToUnaryOp(TokenType type);

    /**
     * 토큰 타입을 기본 타입으로 변환
     */
    BuiltinType tokenTypeToBuiltinType(TokenType type);
};

} // namespace zlang

#endif // PARSER_H
