/*
Grupo:
Diego Spagnuolo Sugai       | RA: 10417329
Leonardo Moreira dos Santos | RA: 10417555

Para compilar:
gcc -Wall -Wno-unused-result -g -Og compilador.c -o compilador
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

// Enumeração de todos os "átomos" (tokens) da linguagem PasKenzie.
// Cada token recebe um código numérico para ser identificado.
typedef enum {
    ERRO, IDENTIFICADOR, CONSTINT, CONSTCHAR, COMENTARIO, DIV, OR, AND, NOT, IF, THEN, ELSE,
    WHILE, DO, BEGIN, END, READ, WRITE, VAR, PROGRAM, TRUE_TRUE, FALSE_FALSE,
    CHAR, INTEGER, BOOLEAN, PONTO_VIRGULA, VIRGULA, DOIS_PONTOS, ASTERISCO,
    ABRE_PARENTESES, FECHA_PARENTESES, MAIS, MENOS, PONTO, IGUAL, ATRIBUICAO,
    MENOR, MAIOR, MENOR_IGUAL, MAIOR_IGUAL, NEGACAO, EOS
} TAtomo;

// Tabela de strings para imprimir o nome dos átomos e os átomos
const char *TAtomo_str[] = {
    "ERRO", "identifier", "constint", "constchar", "comentario", "div", "or", "and", "not", "if", "then", "else",
    "while", "do", "begin", "end", "read", "write", "var", "program", "true", "false",
    "char", "integer", "boolean", ";", ",", ":", "*",
    "(", ")", "+", "-", ".", "=", ":=",
    "<", ">", "<=", ">=", "<>", "EOS"
};

// Estrutura para guardar as informações de um átomo
// O analisador léxico retorna essa estrutura para o sintático
typedef struct {
    TAtomo atomo;
    int linha;
    union {
        int numero;   // atributo do átomo constint (constante inteira)
        char id[16];  // atributo identifier 
        char ch;      // atributo do átomo constchar (constante carecter) 
    }atributo; 
}TInfoAtomo;

// Variaveis globais usadas pelo compilador.
char *buffer;       // Ponteiro para o código-fonte carregado na memória.
int nLinha;         // Contador global de linhas.
TInfoAtomo lookahead; // Armazena o átomo atual que está sendo analisado.

// Funções do Analisador Léxico.
TInfoAtomo obter_atomo();
void reconhece_numero(TInfoAtomo *infoAtomo);
void reconhece_id(TInfoAtomo *infoAtomo);
void reconhece_constchar(TInfoAtomo *infoAtomo);
void reconhece_simbolos(TInfoAtomo *infoAtomo);   
void reconhece_comentario(TInfoAtomo *infoAtomo);
const char* nome_atomo(TAtomo a);

// Funções do Analisador Sintático.
void consome(TAtomo esperado);
void program();
void block();
void variable_declaration_part();
void variable_declaration();
void type();
void statement_part();
void statement();
void assignment_statement();
void read_statement();
void write_statement();
void if_statement();
void while_statement();
void expression();
void simple_expression();
void term();
void factor();
void relational_operator();
void adding_operator();
void multiplying_operator();

// Função main
int main(int argc, char *argv[]) {
    // Verifica se o nome do arquivo foi passado como argumento na linha de comando
    if (argc != 2) {
        printf("Uso: %s <arquivo_fonte>\n", argv[0]);
        return 1;
    }
    // Abre o arquivo fonte para leitura.
    FILE *f = fopen(argv[1], "r");
    if (f == NULL) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

    // Aloca memória
    fseek(f, 0, SEEK_END);
    long tamanho = ftell(f);
    fseek(f, 0, SEEK_SET);

    buffer = (char*)malloc(tamanho + 1);
    if(buffer == NULL){
        printf("Erro ao alocar memoria.\n");
        fclose(f);
        return 1;
    }
    // Guarda o ponteiro inicial para poder liberar a memória no final
    char *ponteiro_original_para_liberar = buffer;
    // Lê o conteúdo do arquivo para o buffer
    fread(buffer, 1, tamanho, f);
    buffer[tamanho] = '\0';
    fclose(f);
    
    // Inicia a contagem de linhas
    nLinha = 1;
    
    // Pega o primeiro átomo do código-fonte
    lookahead = obter_atomo();

    // Inicia a análise sintática pela regra inicial da gramática (<program>)
    program();
    // Verifica se o final do arquivo foi alcançado corretamente
    consome(EOS);
    
    // Se chegou até aqui sem erros, o programa está sintaticamente correto
    printf("\n%d linhas analisadas, programa sintaticamente correto\n", nLinha);
    
    // Libera a memória alocada para o buffer
    free(ponteiro_original_para_liberar);
    return 0;
}


// -------------------------
// --- ANALISADOR LÉXICO ---
// -------------------------

// Função auxiliar para obter o nome de um átomo como string
const char* nome_atomo(TAtomo a) {
    if (a >= 0 && a <= EOS) return TAtomo_str[a];
    return "TOKEN_DESCONHECIDO";
}

// Função principal do analisador léxico
// Pula espaços em branco e chama a função correta para reconhecer o próximo átomo
TInfoAtomo obter_atomo() {
    TInfoAtomo infoAtomo;
    infoAtomo.atomo = ERRO;

    // Laço pra ignorar todos os caracteres delimitadores (espaço, tab, quebra de linha).
    while (1) {
        while (*buffer == ' ' || *buffer == '\t' || *buffer == '\r') {
            buffer++;
        }

        if (*buffer == '\n') {
            nLinha++;
            buffer++;
            continue;
        }

        // Se encontrou um comentário, processa e continua buscando o próximo átomo
        if (*buffer == '(' && *(buffer + 1) == '*') {
            int linha_inicio_comentario = nLinha;
            buffer += 2;
            while (*buffer != '\0') {
                if (*buffer == '\n') nLinha++;
                if (*buffer == '*' && *(buffer + 1) == ')') {
                    buffer += 2;
                    break;
                }
                buffer++;
            }
            if (*buffer == '\0') {
                printf("# %d: erro lexico, comentario nao fechado.\n", linha_inicio_comentario);
                exit(1);
            }
            printf("# %d:comentario\n", linha_inicio_comentario);
            continue;
        }
        
        break; // Sai do laço se encontrou um caractere que não é espaço nem comentário
    }

    infoAtomo.linha = nLinha;

    // Decide qual tipo de átomo reconhecer com base no caractere atual.
    if (*buffer == '\0') {
        infoAtomo.atomo = EOS; // Fim do arquivo (End Of Source).
    } else if (isdigit(*buffer)) {
        reconhece_numero(&infoAtomo);
    } else if (isalpha(*buffer) || *buffer == '_') {
        reconhece_id(&infoAtomo);
    } else if (*buffer == '\'') {
        reconhece_constchar(&infoAtomo);
    } else {
        reconhece_simbolos(&infoAtomo);
    }
    
    return infoAtomo;
}

// Reconhece um comentário do tipo (* ... *)
void reconhece_comentario(TInfoAtomo *infoAtomo) {
    int linha_inicio_comentario = nLinha;

    buffer += 2; // Para pular o "(*"
    while (*buffer != '\0') {
        if (*buffer == '\n') nLinha++;
        if (*buffer == '*' && *(buffer + 1) == ')') {
            buffer += 2; // Para pular o "*)"
            infoAtomo->linha = linha_inicio_comentario;
            infoAtomo->atomo = COMENTARIO;
            return;
        }
        buffer++;
    }
    // Se o laço terminar sem encontrar "*)", o comentário não foi fechado.
    printf("# %d: erro lexico, comentario nao fechado.\n", linha_inicio_comentario);
    exit(1);
}

// Reconhece constantes inteiras (constint), incluindo notação exponencial com 'd'.
void reconhece_numero(TInfoAtomo *infoAtomo) {
    char *ini_lexema = buffer;
    while(isdigit(*buffer)) buffer++;
    int valor_base = strtoll(ini_lexema, NULL, 10);
    // Verifica se há um 'd' para a parte exponencial.
    if (tolower(*buffer) == 'd') {
        buffer++;
        int sinal_expoente = 1;
        if (*buffer == '-') { sinal_expoente = -1; buffer++; }
        else if (*buffer == '+') { buffer++; }
        if (!isdigit(*buffer)) {
            printf("# %d: erro lexico, 'd' de expoente deve ser seguido por um digito.\n", infoAtomo->linha);
            exit(1);
        }
        char *ini_expoente = buffer;
        while(isdigit(*buffer)) buffer++;
        int expoente = (int)strtol(ini_expoente, NULL, 10);
        // Calcula o valor final com base no expoente.
        if (sinal_expoente > 0) { for (int i = 0; i < expoente; i++) { valor_base *= 10; } }
        else { for (int i = 0; i < expoente; i++) { valor_base /= 10; } }
    }
    // Armazena o valor no atributo e define o tipo do átomo.
    infoAtomo->atributo.numero = (int)valor_base;
    infoAtomo->atomo = CONSTINT;
}

// Reconhece identificadores e verifica se são palavras reservadas.
void reconhece_id(TInfoAtomo *infoAtomo){
    char *ini_lexema = buffer;
    while(isalnum(*buffer) || *buffer == '_') buffer++;
    int tamanho = buffer - ini_lexema;
    // Verifica o limite de 15 caracteres.
    if (tamanho > 15) {
        printf("# %d: erro lexico, identificador com mais de 15 caracteres.\n", infoAtomo->linha);
        exit(1);
    }
    // Copia o lexema para o atributo do átomo.
    strncpy(infoAtomo->atributo.id, ini_lexema, tamanho);
    infoAtomo->atributo.id[tamanho] = '\0';
    // Compara o lexema com a lista de palavras reservadas.
    if (strcmp(infoAtomo->atributo.id, "div") == 0) infoAtomo->atomo = DIV;
    else if (strcmp(infoAtomo->atributo.id, "or") == 0) infoAtomo->atomo = OR;
    else if (strcmp(infoAtomo->atributo.id, "and") == 0) infoAtomo->atomo = AND;
    else if (strcmp(infoAtomo->atributo.id, "not") == 0) infoAtomo->atomo = NOT;
    else if (strcmp(infoAtomo->atributo.id, "if") == 0) infoAtomo->atomo = IF;
    else if (strcmp(infoAtomo->atributo.id, "then") == 0) infoAtomo->atomo = THEN;
    else if (strcmp(infoAtomo->atributo.id, "else") == 0) infoAtomo->atomo = ELSE;
    else if (strcmp(infoAtomo->atributo.id, "while") == 0) infoAtomo->atomo = WHILE;
    else if (strcmp(infoAtomo->atributo.id, "do") == 0) infoAtomo->atomo = DO;
    else if (strcmp(infoAtomo->atributo.id, "begin") == 0) infoAtomo->atomo = BEGIN;
    else if (strcmp(infoAtomo->atributo.id, "end") == 0) infoAtomo->atomo = END;
    else if (strcmp(infoAtomo->atributo.id, "read") == 0) infoAtomo->atomo = READ;
    else if (strcmp(infoAtomo->atributo.id, "write") == 0) infoAtomo->atomo = WRITE;
    else if (strcmp(infoAtomo->atributo.id, "var") == 0) infoAtomo->atomo = VAR;
    else if (strcmp(infoAtomo->atributo.id, "program") == 0) infoAtomo->atomo = PROGRAM;
    else if (strcmp(infoAtomo->atributo.id, "true") == 0) infoAtomo->atomo = TRUE_TRUE;
    else if (strcmp(infoAtomo->atributo.id, "false") == 0) infoAtomo->atomo = FALSE_FALSE;
    else if (strcmp(infoAtomo->atributo.id, "char") == 0) infoAtomo->atomo = CHAR;
    else if (strcmp(infoAtomo->atributo.id, "integer") == 0) infoAtomo->atomo = INTEGER;
    else if (strcmp(infoAtomo->atributo.id, "boolean") == 0) infoAtomo->atomo = BOOLEAN;
    // Se não for nenhuma palavra reservada, é um identificador comum.
    else infoAtomo->atomo = IDENTIFICADOR;
}

// Reconhece constantes de caractere (constchar), como 'a'.
void reconhece_constchar(TInfoAtomo *infoAtomo){
    buffer++; // Pula o primeiro apóstrofo.
    // Verifica se o formato é válido (caractere seguido de apóstrofo).
    if (*buffer != '\0' && *(buffer + 1) == '\'') {
        infoAtomo->atributo.ch = *buffer;
        buffer += 2; // Pula o caractere e o apóstrofo final.
        infoAtomo->atomo = CONSTCHAR;
    } else {
        printf("# %d: erro lexico, constante char mal formada.\n", infoAtomo->linha);
        exit(1);
    }
}

// Reconhece todos os outros símbolos e operadores da linguagem.
void reconhece_simbolos(TInfoAtomo *infoAtomo){
    switch(*buffer) {
        case '+': infoAtomo->atomo = MAIS; buffer++; break;
        case '-': infoAtomo->atomo = MENOS; buffer++; break;
        case '*': infoAtomo->atomo = ASTERISCO; buffer++; break;
        case ';': infoAtomo->atomo = PONTO_VIRGULA; buffer++; break;
        case ',': infoAtomo->atomo = VIRGULA; buffer++; break;
        case '.': infoAtomo->atomo = PONTO; buffer++; break;
        case '(': infoAtomo->atomo = ABRE_PARENTESES; buffer++; break;
        case ')': infoAtomo->atomo = FECHA_PARENTESES; buffer++; break;
        case '=': infoAtomo->atomo = IGUAL; buffer++; break;
        case ':':
            // Verifica se é ':=' (atribuição) ou apenas ':' (dois-pontos).
            if (*(buffer + 1) == '=') { infoAtomo->atomo = ATRIBUICAO; buffer += 2; }
            else { infoAtomo->atomo = DOIS_PONTOS; buffer++; }
            break;
        case '<':
            // Verifica se é '<=', '<>' (diferente) ou apenas '<'.
            if (*(buffer + 1) == '=') { infoAtomo->atomo = MENOR_IGUAL; buffer += 2; }
            else if (*(buffer + 1) == '>') { infoAtomo->atomo = NEGACAO; buffer += 2; }
            else { infoAtomo->atomo = MENOR; buffer++; }
            break;
        case '>':
            // Verifica se é '>=' ou apenas '>'.
            if (*(buffer + 1) == '=') { infoAtomo->atomo = MAIOR_IGUAL; buffer += 2; }
            else { infoAtomo->atomo = MAIOR; buffer++; }
            break;
        default:
            // Se o caractere não for reconhecido, é um erro léxico.
            printf("# %d: erro lexico, simbolo desconhecido: %c\n", infoAtomo->linha, *buffer);
            exit(1);
    }
}


// ----------------------------
// --- ANALISADOR SINTÁTICO ---
// ----------------------------

// Função principal do sintático. Verifica se o token atual é o esperado pela gramática.
// Se for, imprime e avança para o próximo token. Se não, gera um erro sintático.
void consome(TAtomo esperado) {
    if (lookahead.atomo == esperado) {
        // Imprime o átomo reconhecido na tela
        switch (esperado) {
            case COMENTARIO:    printf("# %d:comentario\n", lookahead.linha); break;
            case PROGRAM:       printf("# %d:program\n", lookahead.linha); break;
            case IDENTIFICADOR: printf("# %d:identifier : %s\n", lookahead.linha, lookahead.atributo.id); break;
            case VAR:           printf("# %d:var\n", lookahead.linha); break;
            case INTEGER:       printf("# %d:integer\n", lookahead.linha); break;
            case CHAR:          printf("# %d:char\n", lookahead.linha); break;
            case BOOLEAN:       printf("# %d:boolean\n", lookahead.linha); break;
            case BEGIN:         printf("# %d:begin\n", lookahead.linha); break;
            case END:           printf("# %d:end\n", lookahead.linha); break;
            case READ:          printf("# %d:read\n", lookahead.linha); break;
            case WRITE:         printf("# %d:write\n", lookahead.linha); break;
            case IF:            printf("# %d:if\n", lookahead.linha); break;
            case THEN:          printf("# %d:then\n", lookahead.linha); break;
            case ELSE:          printf("# %d:else\n", lookahead.linha); break;
            case WHILE:         printf("# %d:while\n", lookahead.linha); break;
            case DO:            printf("# %d:do\n", lookahead.linha); break;
            case TRUE_TRUE:   printf("# %d:true\n", lookahead.linha); break;
            case FALSE_FALSE:  printf("# %d:false\n", lookahead.linha); break;
            case PONTO_VIRGULA: printf("# %d:ponto_virgula\n", lookahead.linha); break;
            case VIRGULA:       printf("# %d:virgula\n", lookahead.linha); break;
            case DOIS_PONTOS:   printf("# %d:dois_pontos\n", lookahead.linha); break;
            case PONTO:         printf("# %d:ponto\n", lookahead.linha); break;
            case ABRE_PARENTESES:     printf("# %d:abre_parenteses\n", lookahead.linha); break;
            case FECHA_PARENTESES:    printf("# %d:fecha_parenteses\n", lookahead.linha); break;
            case MAIS:          printf("# %d:mais\n", lookahead.linha); break;
            case MENOS:         printf("# %d:menos\n", lookahead.linha); break;
            case ASTERISCO:     printf("# %d:asterisco\n", lookahead.linha); break;
            case IGUAL:         printf("# %d:igual\n", lookahead.linha); break;
            case ATRIBUICAO:    printf("# %d:atribuicao\n", lookahead.linha); break;
            case MENOR:         printf("# %d:menor\n", lookahead.linha); break;
            case MAIOR:         printf("# %d:maior\n", lookahead.linha); break;
            case MENOR_IGUAL:   printf("# %d:menor_igual\n", lookahead.linha); break;
            case MAIOR_IGUAL:   printf("# %d:maior_igual\n", lookahead.linha); break;
            case NEGACAO:       printf("# %d:negacao\n", lookahead.linha); break;
            case DIV:           printf("# %d:div\n", lookahead.linha); break;
            case OR:            printf("# %d:or\n", lookahead.linha); break;
            case AND:           printf("# %d:and\n", lookahead.linha); break;
            case NOT:           printf("# %d:not\n", lookahead.linha); break;
            case CONSTINT:      printf("# %d:constint : %d\n", lookahead.linha, lookahead.atributo.numero); break;
            case CONSTCHAR:     printf("# %d:constchar : '%c'\n", lookahead.linha, lookahead.atributo.ch); break;
            case EOS:           break;
            default:            break;
        }
        // Avança para o próximo átomo.
        if (lookahead.atomo != EOS) {
            lookahead = obter_atomo();
        }
    } else {
        // Gera um erro sintático se o átomo atual não for o esperado.
        printf("# %d:erro sintatico, esperado [%s] encontrado [%s]\n", lookahead.linha, nome_atomo(esperado), nome_atomo(lookahead.atomo));
        exit(1);
    }
}

// Implementa a regra: <program> ::= program identifier ';' <block> '.'
void program() {
    while(lookahead.atomo==COMENTARIO){
        printf("# %d:comentario\n", lookahead.linha);
        lookahead = obter_atomo();
    }
    consome(PROGRAM);
    consome(IDENTIFICADOR);
    consome(PONTO_VIRGULA);
    block();
    consome(PONTO);
}

// Implementa a regra: <block> ::= <variable_declaration_part> <statement_part>
void block(){
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);

    variable_declaration_part();

    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);

    statement_part();
}

// Implementa a regra: <variable_declaration_part> ::= [ var <variable_declaration> ';' { <variable_declaration> ';' } ]
void variable_declaration_part() {
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    // A parte de declaração de variáveis é opcional.
    if (lookahead.atomo == VAR) {
        consome(VAR);
        variable_declaration();
        consome(PONTO_VIRGULA);
        // Laço para múltiplas declarações de variáveis.
        while (lookahead.atomo == IDENTIFICADOR || lookahead.atomo == COMENTARIO) {
             while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
             if(lookahead.atomo == IDENTIFICADOR){
                variable_declaration();
                consome(PONTO_VIRGULA);
             }
        }
    }
}

// Implementa a regra: <variable_declaration> ::= identifier { ',' identifier } ':' <type>
void variable_declaration() {
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    consome(IDENTIFICADOR);
    // Laço para identificadores separados por vírgula (ex: var a, b: integer).
    while (lookahead.atomo == VIRGULA || lookahead.atomo == COMENTARIO) {
        while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
        if(lookahead.atomo == VIRGULA){
            consome(VIRGULA);
            consome(IDENTIFICADOR);
        }
    }
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    consome(DOIS_PONTOS);
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    type();
}

// Implementa a regra: <type> ::= char | integer | boolean
void type() {
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    if (lookahead.atomo == CHAR) consome(CHAR);
    else if (lookahead.atomo == INTEGER) consome(INTEGER);
    else if (lookahead.atomo == BOOLEAN) consome(BOOLEAN);
    else {
        printf("# %d:erro sintatico, tipo invalido. Esperado [char, integer, boolean] mas encontrado [%s]\n", lookahead.linha, nome_atomo(lookahead.atomo));
        exit(1);
    }
}

// Implementa a regra: <statement_part> ::= begin <statement> { ';' <statement> } end
void statement_part() {
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    consome(BEGIN);
    statement();
    // Laço para múltiplos comandos separados por ponto e vírgula.
    while (lookahead.atomo == PONTO_VIRGULA || lookahead.atomo == COMENTARIO) {
        while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
        if(lookahead.atomo == PONTO_VIRGULA){
            consome(PONTO_VIRGULA);
            statement();
        }
    }
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    consome(END);
}

// Implementa a regra: <statement> ::= <assignment_statement> | <read_statement> | ...
void statement() {
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    // Decide qual tipo de comando analisar com base no token atual.
    if (lookahead.atomo == IDENTIFICADOR) assignment_statement();
    else if (lookahead.atomo == READ) read_statement();
    else if (lookahead.atomo == WRITE) write_statement();
    else if (lookahead.atomo == IF) if_statement();
    else if (lookahead.atomo == WHILE) while_statement();
    else if (lookahead.atomo == BEGIN) statement_part();
    else {consome(READ);} // Permite um comando vazio.
}

// Implementa a regra: <assignment_statement> ::= identifier ':=' <expression>
void assignment_statement(){
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    consome(IDENTIFICADOR);
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    consome(ATRIBUICAO);
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    expression();
}

// Implementa a regra: <read_statement> ::= read '(' identifier { ',' identifier } ')'
void read_statement() {
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    consome(READ);
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    consome(ABRE_PARENTESES);
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    consome(IDENTIFICADOR);
    while (lookahead.atomo == VIRGULA || lookahead.atomo == COMENTARIO) {
        while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
        if(lookahead.atomo == VIRGULA){
            consome(VIRGULA);
            consome(IDENTIFICADOR);
        }
    }
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    consome(FECHA_PARENTESES);
}

// Implementa a regra: <write_statement> ::= write '(' identifier { ',' identifier } ')'
void write_statement() {
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    consome(WRITE);
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    consome(ABRE_PARENTESES);
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    consome(IDENTIFICADOR);
    while (lookahead.atomo == VIRGULA || lookahead.atomo == COMENTARIO) {
        while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
        if(lookahead.atomo == VIRGULA){
            consome(VIRGULA);
            consome(IDENTIFICADOR);
        }
    }
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    consome(FECHA_PARENTESES);
}

// Implementa a regra: <if_statement> ::= if <expression> then <statement> [ else <statement> ]
void if_statement() {
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    consome(IF);
    expression();
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    consome(THEN);
    statement();
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    // A parte 'else' é opcional.
    if (lookahead.atomo == ELSE) {
        consome(ELSE);
        statement();
    }
}

// Implementa a regra: <while_statement> ::= while <expression> do <statement>
void while_statement() {
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    consome(WHILE);
    expression();
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    consome(DO);
    statement();
}

// Implementa a regra: <expression> ::= <simple_expression> [ <relational_operator> <simple_expression> ]
void expression() {
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    simple_expression();
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    // A parte com o operador relacional é opcional.
    if (lookahead.atomo == MENOR || lookahead.atomo == MAIOR || lookahead.atomo == MENOR_IGUAL ||
        lookahead.atomo == MAIOR_IGUAL || lookahead.atomo == NEGACAO || lookahead.atomo == IGUAL ||
        lookahead.atomo == OR || lookahead.atomo == AND ) {
        relational_operator();
        simple_expression();
    }
}

// Implementa a regra: <relational_operator> ::= '<>' | '<' | ...
void relational_operator() {
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    switch (lookahead.atomo) {
        case MENOR:       consome(MENOR); break;
        case MAIOR:       consome(MAIOR); break;
        case MENOR_IGUAL: consome(MENOR_IGUAL); break;
        case MAIOR_IGUAL: consome(MAIOR_IGUAL); break;
        case NEGACAO:     consome(NEGACAO); break;
        case IGUAL:       consome(IGUAL); break;
        case OR:          consome(OR); break;
        case AND:         consome(AND); break;
        default:
            printf("# %d:erro sintatico, operador relacional esperado mas encontrado [%s]\n", lookahead.linha, nome_atomo(lookahead.atomo));
            exit(1);
    }
}

// Implementa a regra: <simple_expression> ::= <term> { <adding_operator> <term> }
void simple_expression() {
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    term();
    while (lookahead.atomo == MAIS || lookahead.atomo == MENOS || lookahead.atomo == COMENTARIO) {
        while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
        if(lookahead.atomo == MAIS || lookahead.atomo == MENOS){
            adding_operator();
            term();
        }
    }
}

// Implementa a regra: <adding_operator> ::= '+' | '-'
void adding_operator() {
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    if (lookahead.atomo == MAIS) consome(MAIS);
    else if (lookahead.atomo == MENOS) consome(MENOS);
    else {
        printf("# %d:erro sintatico, operador de adicao esperado mas encontrado [%s]\n", lookahead.linha, nome_atomo(lookahead.atomo));
        exit(1);
    }
}

// Implementa a regra: <term> ::= <factor> { <multiplying_operator> <factor> }
void term() {
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    factor();
    while (lookahead.atomo == ASTERISCO || lookahead.atomo == DIV || lookahead.atomo == COMENTARIO) {
        while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
        if(lookahead.atomo == ASTERISCO || lookahead.atomo == DIV){
            multiplying_operator();
            factor();
        }
    }
}

// Implementa a regra: <multiplying_operator> ::= '*' | div
void multiplying_operator() {
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    if (lookahead.atomo == ASTERISCO) consome(ASTERISCO);
    else if (lookahead.atomo == DIV) consome(DIV);
    else {
        printf("# %d:erro sintatico, operador de multiplicacao esperado mas encontrado [%s]\n", lookahead.linha, nome_atomo(lookahead.atomo));
        exit(1);
    }
}

// Implementa a regra: <factor> ::= identifier | constint | constchar | ...
void factor() {
    while (lookahead.atomo == COMENTARIO) consome(COMENTARIO);
    if (lookahead.atomo == IDENTIFICADOR) consome(IDENTIFICADOR);
    else if (lookahead.atomo == CONSTINT) consome(CONSTINT);
    else if (lookahead.atomo == CONSTCHAR) consome(CONSTCHAR);
    else if (lookahead.atomo == ABRE_PARENTESES) {
        consome(ABRE_PARENTESES);
        expression();
        consome(FECHA_PARENTESES);
    }
    else if (lookahead.atomo == NOT) {
        consome(NOT);
        factor();
    }
    else if (lookahead.atomo == TRUE_TRUE) consome(TRUE_TRUE);
    else if (lookahead.atomo == FALSE_FALSE) consome(FALSE_FALSE);
    else {
        printf("# %d:erro sintatico, fator invalido. Esperado [identifier, constint, '(', ...], mas encontrado [%s]\n", lookahead.linha, nome_atomo(lookahead.atomo));
        exit(1);
    }
}