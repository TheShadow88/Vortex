%require "3.4"
%language "c++"

%define api.value.type variant
%define api.token.constructor
%locations
%defines
%define parse.trace
%define parse.error verbose
%define api.token.prefix {TOK_}

%{
#include "../src/Driver/driver.hpp"
%}


%code requires {
  #include <string>
  #include <any>
  #include <vector>
  #include "../src/Variables/vars.hpp"
  #include "../src/Functions/Function.hpp"
  #include "../src/Classes/Class.hpp"

  class Driver;
}

// The parsing context.
%param { Driver& drv }

%token END_OF_FILE 0
%token <Type> TYPE;
%token <std::string> SYMBOL STRING BODY FILEPATH;
%token <int> INTEGER;
%token <float> FLOAT;
%token <char> CHAR;
%token <bool> BOOL;
%token
  COMPARE "=="
  GREATER ">"
  LESS "<"
  DIFFERENT "!="
  GREATEREQUAL ">="
  SMALLEREQUAL "<="
  EQUALS "="
  MINUS   "-"
  PLUS    "+"
  STAR    "*"
  SLASH   "/"
  COMMA   ","
  LPAREN  "("
  RPAREN  ")"
  LSQUARE "["
  RSQUARE "]"
  LCURLY  "{"
  RCURLY  "}"
  SEMICOL ";"
  DOT     "."
  FUNC "fn"
  RETSIGN "->"
  RET "ret"
  IF "if"
  ELSE "else"
  IMPORT "import"
  LOOP "loop"
  BREAK "break"
  CLASS "class"
  NEW "new"
  EXTENDS "extends"
  ;


%code {
  yy::parser::symbol_type yylex(Driver& drv);

}


%%
%start result;
result:
  %empty {}
  | result clause  { 
    if(!$2.isStatement()) {
      drv.setLastValue(std::move($2)); if(drv.isPrintingLastValue()) std::cout << "~> " << drv.getLastValue() << '\n';
    }
  }
  | result "ret" exp ";" {drv.setLastValue(std::move($3)); return 2;}
  | result "ret" ";" {drv.setLastValue(std::move(rvalue()));return 2;}
  | result "break" ";" {drv.setLastValue(std::move(rvalue()));return 2;}

;

%type <rvalue> clause;
clause: exp
| statement {$$=rvalue();}
;


statement:
  "fn" SYMBOL "(" args_d ")" "->" TYPE "{" body "}" {drv.declareFunction(new Function($2, $7, std::move($args_d), $body));}
| "import" FILEPATH {drv.executeFile($2);}
| "loop" "{" body "}" { while(true) { int res = drv.loop($body); if(res == 2){ break; } } }
| if_stmnt {}
| "class" SYMBOL extend.opt bases "{" declarations.opt "}" {Class sel_class($SYMBOL, std::move($[declarations.opt]), drv);
  sel_class.extendWithClasses(std::move($bases));drv.declareClass(std::move(sel_class));}
;

extend.opt:
  %empty
| "extends";

%type <std::vector<Class>> bases;
bases: %empty {}
| SYMBOL {$$.push_back(drv.getScope().getClass($SYMBOL));}
| bases "," SYMBOL {$$=$1;$$.push_back(drv.getScope().getClass($SYMBOL));}
;

%type <Var*> declaration;
declaration: 
 TYPE SYMBOL ";" {$$=new Var($TYPE, $SYMBOL);}
| "fn" SYMBOL "(" args_d ")" "->" TYPE "{" body "}" {$$=new Function($SYMBOL, $TYPE, std::move($args_d), $body);}
;

%type <std::vector<Var*>> declarations;
declarations: 
  declaration {$$.push_back($declaration);}
| declarations declaration {$1.push_back($declaration); $$=$1;}
;


%type <std::vector<Var*>> declarations.opt;
declarations.opt: 
  %empty {}
| declarations {$$=$1;}
;


%type <std::string> body;
body: 
%empty {$$="";}
| BODY
;

%type <std::vector<Var>> args_d;
args_d:
%empty {}
|  arg_d {$$.push_back($arg_d);}
| args_d "," arg_d {$$=$1; $$.push_back($arg_d); }
;

%type <Var> arg_d;
arg_d: TYPE SYMBOL {$$=Var($1, $2);}
;

%type <std::vector<rvalue>> args;
args:
%empty {}
| arg {$$.push_back($arg);}
| args "," arg {$$=$1; $$.push_back($arg);}
;

%type <rvalue> arg;
arg: exp
;

%left "==" ">" "<" ">=" "<=" "!=";
%left "+" "-";
%left "*" "/" ".";
%precedence SYMBOL;
%right "=" "[";
%precedence NEG;


%type <rvalue> exp;
exp: INTEGER {$$=rvalue(Type::INT, $1);}
| STRING {$$=rvalue(Type::STRING, $1);}
| FLOAT {$$=rvalue(Type::FLOAT, $1);}
| CHAR {$$=rvalue(Type::CHAR, $1);}
| lside {if($lside.getType() == Type::UNKNOWN) { 
  $$=drv.getVariable($lside.getName()).getValue();} 
  else {$$=$lside.getValue();}}
| lside "=" exp ";" {$lside.setValue(rvalue($3));if($lside.getType() == Type::UNKNOWN) {$lside.setType($3.getType());}
  drv.setVariable(std::move($lside)); $$=std::move($3);}
| exp "." SYMBOL "=" exp ";" {Instance inst = $1.getValue<Instance>(); inst.getProp($SYMBOL).setValue(rvalue($5)); $$=std::move($5);}
| exp[base] "*" "*" exp[power] {$$=$base.pow($power);}
| exp "+" exp {$$= $1 + $3;}
| exp "-" exp {$$= $1 - $3;} 
| exp "*" exp {$$= $1 * $3;} 
| exp "/" exp {$$= $1 / $3;}
| "-" exp %prec NEG {$$= -$2;}
| SYMBOL "(" args ")" {drv.callFunction($1, std::move($args)); $$=drv.getLastValue();}
| exp "[" exp "]" {
      if($1.getType() != Type::OBJECT){
          if($1.getType() == Type::STRING){
              $$=rvalue(Type::CHAR, $1.getValue<std::string>()[$3.getValue<int>()]);
          }
          else{
            throw new IncorrectTypesException("No index operator for", Type::STRING, $1.getType());
          }
      }
      else{
        Instance inst = $1.getValue<Instance>();
        inst.callMethod("atIndex", {std::move($3)}, drv);  $$=drv.getLastValue();}}
| "[" args "]" {$$=drv.makeVector(std::move($args));}
| exp "[" exp "]" "=" exp ";"{Instance inst = $1.getValue<Instance>(); inst.callMethod("remove", {$3}, drv);
  inst.callMethod("insert", {std::move($3), $6}, drv); $$=$6;}
  
| exp "."  SYMBOL "(" args ")"  {Instance inst = $1.getValue<Instance>(); drv.setLastValue(std::move($1));
      inst.callMethod($SYMBOL, std::move($args), drv); $$=drv.getLastValue();}
      
| exp "." "." SYMBOL "(" args ")"  {Instance inst = $1.getValue<Instance>().clone();drv.setLastValue(rvalue(Type::OBJECT,inst));
      inst.callMethod($SYMBOL, std::move($args), drv); $$=drv.getLastValue();}
| "new" SYMBOL "(" args ")" {$$=drv.makeInstance($SYMBOL, std::move($args));}
| bool_exp 
;

%type <Var> lside;
lside: TYPE SYMBOL {$$=Var($1, $2);}
| SYMBOL {if (drv.getScope().variables.contains($SYMBOL)) {$$=drv.getVariable($SYMBOL);} else {$$=Var($SYMBOL);}}
| exp "." SYMBOL {Instance inst = $1.getValue<Instance>(); $$=inst.getProp($SYMBOL);}
;

%type <bool> bool;
bool:
 bool_exp {$$=$1.getValue<bool>();} 
;

%type <rvalue> bool_exp;
bool_exp: BOOL {$$=rvalue(Type::BOOL, $1);}
| exp "==" exp {$$=$1 == $3;}
| exp ">" exp {$$=$1 > $3;}
| exp "<" exp {$$=$1 < $3;}
| exp ">=" exp {$$=$1 >= $3;}
| exp "<=" exp {$$=$1 <= $3;}
| exp "!=" exp {$$=$1 != $3;}
;

%type <bool> if_stmnt;
if_stmnt:
"if" "(" bool ")" "{" body "}" { $$= $bool; 
    if($bool){ 
      if(drv.runConditional($body) == 2) {
        return 2;
      }  
    } 
  }
| if_stmnt "else" "{" body "}" {  $$=$1;
    if(!$1) {
      if(drv.runConditional($body) == 2) {
        return 2;
      }  
    }
  }
;


%%
namespace yy
{
  // Report an error to the user. a = 2; loop { a = a +2; if(a == 10) {ret;} }
  auto parser::error (const location_type& l, const std::string& msg) -> void
  {
    std::string* arr = drv.getLastLines();
    for(int i=0; i < 3; i++){
      if(*(arr+i) != ""){
        std::cerr << i + (l.begin.line - 1) << " | " << *(arr + i) << std::endl;
      }
    }
    int offset = std::to_string(l.end.line).size() + 3 + (l.end.column) - 1;
    // divide columns by 2 to find the real region
    // pretty errors
    for(int i=0; i < offset; i++){
      std::cerr << "~";
    }
    std::cerr << "^" << '\n';
    std::cerr << l << ":" << msg << '\n';

  }
}

// int main ()
// {
//   std::string s = "-";
//   loc.initialize(&s);
//   yy::parser parse;
//   return parse ();
// }
