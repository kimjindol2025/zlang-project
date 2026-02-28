#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "Token.h"

namespace zlang {

/**
 * Lexer: 소스 코드를 토큰 스트림으로 변환
 *
 * 입력: Z-Lang 소스 코드 (문자열)
 * 출력: Token 벡터
 *
 * 예:
 *   source: "let x: i64 = 42;"
 *   tokens: [KW_LET, IDENTIFIER("x"), COLON, KW_I64, ASSIGN, INTEGER("42"), SEMICOLON, EOF]
 */
class Lexer {
private:
    std::string source;
    size_t position = 0;        // 현재 위치
    int line = 1;
    int column = 1;
    char current_char = '\0';   // 현재 문자

    std::vector<Token> tokens;
    std::vector<std::string> errors;

    // 키워드 맵 (문자열 → TokenType)
    static const std::unordered_map<std::string, TokenType> keywords;

public:
    Lexer(const std::string& src);

    /**
     * 진입점: 소스 코드를 토큰화
     */
    std::vector<Token> tokenize();

    /**
     * 에러 메시지
     */
    const std::vector<std::string>& getErrors() const { return errors; }

private:
    // ========================================================================
    // 【 내부 헬퍼 메서드 】
    // ========================================================================

    /**
     * 다음 문자로 이동
     */
    void advance();

    /**
     * 현재 문자 확인 (이동 X)
     */
    char peek() const;

    /**
     * 현재 문자가 조건을 만족하는지 확인
     */
    bool match(char c);

    /**
     * 공백 및 주석 건너뛰기
     */
    void skipWhitespaceAndComments();

    /**
     * 식별자 또는 키워드 읽기
     */
    Token readIdentifierOrKeyword();

    /**
     * 정수 또는 실수 읽기
     */
    Token readNumber();

    /**
     * 문자열 읽기 ("..." 형식)
     */
    Token readString();

    /**
     * 2글자 연산자 확인 (==, !=, <=, >=, &&, ||, ->, #[)
     */
    Token readOperatorOrPunctuation();

    /**
     * 에러 기록
     */
    void reportError(const std::string& message);

    /**
     * 문자가 식별자 시작 가능한지 확인
     */
    static bool isIdentifierStart(char c);

    /**
     * 문자가 식별자 계속 가능한지 확인
     */
    static bool isIdentifierContinue(char c);

    /**
     * 문자가 숫자인지 확인
     */
    static bool isDigit(char c);

    /**
     * 문자가 16진수 숫자인지 확인
     */
    static bool isHexDigit(char c);

    /**
     * 문자가 공백인지 확인
     */
    static bool isWhitespace(char c);
};

} // namespace zlang

#endif // LEXER_H
