/*
 * [test] btyacc
 * [test] cc -c -Wall -Werror
 */
%left LO '+' '-'
%left HI '*' '/' '%'
%nonassoc UNARY

%%

expr: expr op1 expr %prec LO
    | expr op2 expr %prec HI
    | unary expr %prec UNARY
    ;

op1 : '+'
    | '-'
    ;

op2 : '*'
    | '/'
    | '%'
    ;

unary : '+' | '-' | '*' | '&' ;

