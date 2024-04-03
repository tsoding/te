#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "lexer.h"

typedef struct {
    Token_Kind kind;
    const char *text;
} Literal_Token;

Literal_Token literal_tokens[] = {
    {.text = "(", .kind = TOKEN_OPEN_PAREN},
    {.text = ")", .kind = TOKEN_CLOSE_PAREN},
    {.text = "{", .kind = TOKEN_OPEN_CURLY},
    {.text = "}", .kind = TOKEN_CLOSE_CURLY},
    {.text = ";", .kind = TOKEN_SEMICOLON},
};
#define literal_tokens_count (sizeof(literal_tokens)/sizeof(literal_tokens[0]))

const char *jKeywords[] = {
    "abstract", "assert", "boolean", "break", "byte", "case", "catch", "char", "class",
    "const", "continue", "default", "do", "double", "else", "enum", "extends", "final",
    "finally", "float", "for", "goto", "if", "implements", "import", "instanceof", "int",
    "interface", "long", "native", "new", "package", "private", "protected", "public",
    "return", "short", "static", "super", "switch", "synchronized", "this", "throw",
    "throws", "transient", "try", "void", "volatile", "while", "non-sealed", "open",
    "opens", "permits", "provides", "record", "sealed", "to", "transitive", "uses", "var",
    "with", "yield", "true", "false", "null", "const", "goto", "strictfp",
};
#define jKeywords_count (sizeof(jKeywords)/sizeof(jKeywords[0]))

const char *ktKeywords[] = {
    "abstract", "break", "catch", "class", "const", "continue", "else", "enum", "is", "as",
    "when", "val", "var", "for", "if", "import", "interface", "data", "external", "inner",
    "package", "private", "protected", "return", "super", "when",  "this", "throw",
    "try", "while", "sealed", "open", "true", "false", "null", "fun", "typealias", "in",
};
#define ktKeywords_count (sizeof(ktKeywords)/sizeof(ktKeywords[0]))

const char *cKeywords[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do", "double",
    "else", "enum", "extern", "float", "for", "goto", "if", "int", "long", "register",
    "return", "short", "signed", "sizeof", "static", "struct", "switch", "typedef",
    "union", "unsigned", "void", "volatile", "while", "alignas", "alignof", "and",
    "and_eq", "asm", "atomic_cancel", "atomic_commit", "atomic_noexcept", "bitand",
    "bitor", "bool", "catch", "char16_t", "char32_t", "char8_t", "class", "co_await",
    "co_return", "co_yield", "compl", "concept", "const_cast", "consteval", "constexpr",
    "constinit", "decltype", "delete", "dynamic_cast", "explicit", "export", "false",
    "friend", "inline", "mutable", "namespace", "new", "noexcept", "not", "not_eq",
    "nullptr", "operator", "or", "or_eq", "private", "protected", "public", "reflexpr",
    "reinterpret_cast", "requires", "static_assert", "static_cast", "synchronized",
    "template", "this", "thread_local", "throw", "true", "try", "typeid", "typename",
    "using", "virtual", "wchar_t", "xor", "xor_eq",
};
#define cKeywords_count (sizeof(cKeywords)/sizeof(cKeywords[0]))

const char *pyKeywords[] = {
    "False", "None", "True", "and", "as", "assert", "async", "await", "break", "class",
    "continue", "def", "del", "elif", "else", "except", "finally", "for", "from", "global",
    "if", "import", "in", "is", "lambda", "nonlocal", "not", "or", "pass", "raise",
    "return", "try", "while", "with", "yield",
};
#define pyKeywords_count (sizeof(pyKeywords)/sizeof(pyKeywords[0]))

const char *token_kind_name(Token_Kind kind)
{
    switch (kind) {
    case TOKEN_END:
        return "end of content";
    case TOKEN_INVALID:
        return "invalid token";
    case TOKEN_PREPROC:
        return "preprocessor directive";
    case TOKEN_SYMBOL:
        return "symbol";
    case TOKEN_OPEN_PAREN:
        return "open paren";
    case TOKEN_CLOSE_PAREN:
        return "close paren";
    case TOKEN_OPEN_CURLY:
        return "open curly";
    case TOKEN_CLOSE_CURLY:
        return "close curly";
    case TOKEN_SEMICOLON:
        return "semicolon";
    case TOKEN_KEYWORD:
        return "keyword";
    default:
        UNREACHABLE("token_kind_name");
    }
    return NULL;
}

Lexer lexer_new(Free_Glyph_Atlas *atlas, const char *content, size_t content_len, String_Builder file_path)
{
    Lexer l = {0};
    l.atlas = atlas;
    l.content = content;
    l.content_len = content_len;
    l.file_ext = FEXT_CPP;
    if (file_path.items != NULL) {
        l.file_path.items = (char*) malloc(sizeof(char*) * (strlen(file_path.items) + 1));
        strcpy(l.file_path.items, file_path.items);

        const char *filename = l.file_path.items;
        const char *dot = strrchr(filename, '.');
        if (dot && dot != filename) {
            const char *file_ext_str = dot + 1;

            if (strcmp(file_ext_str, "kt") == 0 || strcmp(file_ext_str, "kts") == 0) {
                l.file_ext = FEXT_KOTLIN;
            } else if (strcmp(file_ext_str, "py") == 0) {
                l.file_ext = FEXT_PYTHON;
            } else if (strcmp(file_ext_str, "java") == 0) {
                l.file_ext = FEXT_JAVA;
            }
        }
    }

    return l;
}

const char *file_ext_str(File_Extension ext)
{
    switch (ext) {
        case FEXT_KOTLIN:
            return "Kotlin";
        case FEXT_JAVA:
            return "Java";
        case FEXT_CPP:
            return "C++";
        case FEXT_PYTHON:
            return "Python";
        default:
            return "?";
    }
}

bool lexer_starts_with(Lexer *l, const char *prefix)
{
    size_t prefix_len = strlen(prefix);
    if (prefix_len == 0) {
        return true;
    }
    if (l->cursor + prefix_len - 1 >= l->content_len) {
        return false;
    }
    for (size_t i = 0; i < prefix_len; ++i) {
        if (prefix[i] != l->content[l->cursor + i]) {
            return false;
        }
    }
    return true;
}

void lexer_chop_char(Lexer *l, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        // TODO: get rid of this assert by checking the length of the choped prefix upfront
        assert(l->cursor < l->content_len);
        char x = l->content[l->cursor];
        l->cursor += 1;
        if (x == '\n') {
            l->line += 1;
            l->bol = l->cursor;
            l->x = 0;
        } else {
            if (l->atlas) {
                size_t glyph_index = x;
                // TODO: support for glyphs outside of ASCII range
                if (glyph_index >= GLYPH_METRICS_CAPACITY) {
                    glyph_index = '?';
                }
                Glyph_Metric metric = l->atlas->metrics[glyph_index];
                l->x += metric.ax;
            }
        }
    }
}

void lexer_trim_left(Lexer *l)
{
    while (l->cursor < l->content_len && isspace(l->content[l->cursor])) {
        lexer_chop_char(l, 1);
    }
}

bool is_symbol_start(char x)
{
    return isalpha(x) || x == '_';
}

bool is_symbol(char x)
{
    return isalnum(x) || x == '_';
}

Token lexer_next(Lexer *l)
{
    lexer_trim_left(l);

    Token token = {
        .text = &l->content[l->cursor],
    };

    token.position.x = l->x;
    token.position.y = -(float)l->line * FREE_GLYPH_FONT_SIZE;

    if (l->cursor >= l->content_len) return token;

    if (l->content[l->cursor] == '"') {
        // TODO: TOKEN_STRING should also handle escape sequences
        token.kind = TOKEN_STRING;
        lexer_chop_char(l, 1);
        while (l->cursor < l->content_len && l->content[l->cursor] != '"' && l->content[l->cursor] != '\n') {
            lexer_chop_char(l, 1);
        }
        if (l->cursor < l->content_len) {
            lexer_chop_char(l, 1);
        }
        token.text_len = &l->content[l->cursor] - token.text;
        return token;
    }

    if (l->content[l->cursor] == '#') {
        // TODO: preproc should also handle newlines
        token.kind = TOKEN_PREPROC;
        while (l->cursor < l->content_len && l->content[l->cursor] != '\n') {
            lexer_chop_char(l, 1);
        }
        if (l->cursor < l->content_len) {
            lexer_chop_char(l, 1);
        }
        token.text_len = &l->content[l->cursor] - token.text;
        return token;
    }

    if (lexer_starts_with(l, "//")) {
        token.kind = TOKEN_COMMENT;
        while (l->cursor < l->content_len && l->content[l->cursor] != '\n') {
            lexer_chop_char(l, 1);
        }
        if (l->cursor < l->content_len) {
            lexer_chop_char(l, 1);
        }
        token.text_len = &l->content[l->cursor] - token.text;
        return token;
    }
    
    for (size_t i = 0; i < literal_tokens_count; ++i) {
        if (lexer_starts_with(l, literal_tokens[i].text)) {
            // NOTE: this code assumes that there is no newlines in literal_tokens[i].text
            size_t text_len = strlen(literal_tokens[i].text);
            token.kind = literal_tokens[i].kind;
            token.text_len = text_len;
            lexer_chop_char(l, text_len);
            return token;
        }
    }

    if (is_symbol_start(l->content[l->cursor])) {
        token.kind = TOKEN_SYMBOL;
        while (l->cursor < l->content_len && is_symbol(l->content[l->cursor])) {
            lexer_chop_char(l, 1);
            token.text_len += 1;
        }
        
        const char **keywords;
        size_t keywords_count;
        switch (l->file_ext) {
            case FEXT_JAVA:
                keywords = jKeywords;
                keywords_count = jKeywords_count;
            break;

            case FEXT_KOTLIN:
                keywords = ktKeywords;
                keywords_count = ktKeywords_count;
            break;

            case FEXT_PYTHON:
                keywords = pyKeywords;
                keywords_count = pyKeywords_count;
            break;

            default:
                keywords = cKeywords;
                keywords_count = cKeywords_count;
        }
        

        for (size_t i = 0; i < keywords_count; ++i) {
            size_t keyword_len = strlen(keywords[i]);
            if (keyword_len == token.text_len && memcmp(keywords[i], token.text, keyword_len) == 0) {
                token.kind = TOKEN_KEYWORD;
                break;
            }
        }
        
        return token;
    }

    lexer_chop_char(l, 1);
    token.kind = TOKEN_INVALID;
    token.text_len = 1;
    return token;
}
