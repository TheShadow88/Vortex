#ifndef FUNCTIONCALL_HH
#define FUNCTIONCALL_HH
#include "Function.hpp"
#include "../Utility/RefMap.hpp"
#include "../Variables/vars.hpp"
#include "../Classes/Class.hpp"
#include <vector>
#include <string>
class Class;
struct Scope{
    RefMap<std::string, Var> variables;
    RefMap<std::string, Function*> functions;
    RefMap<std::string, Class> classes;

    Class& getClass(std::string name){
        if(!classes.contains(name)){
            throw NoSuchClassException(name);
        }
        return classes.get(name);
    }
    Var& getVariable(std::string name){
        if(!variables.contains(name)){
            throw ParserException("No variable with name '" + name + "'");
        }
        return variables.get(name);
    }
    Function* getFunction(std::string name){
        if(!functions.contains(name)){
            throw FunctionNotDefined("Function '" + name + "' does not exist.");
        }
        return functions.get(name);
    }
    Scope() {}
};

class FunctionCall {
    Function& fun;
    Scope MainScope;

    public:
    FunctionCall(Function& f): fun(f) {}

    FunctionCall(Function& f, std::vector<rvalue>&& args): FunctionCall(f) {

        std::vector<Var>& f_args = fun.getArgs();
        if(f_args.size() != args.size()){
            throw ParserException("Function '" + f.getName() + "' expects " + std::to_string(f_args.size()) + 
            " arguments, but " + std::to_string(args.size()) + " provided.");
        }

        for(int i=0; i < f_args.size(); i++){
            Var f_arg = f_args[i];
            rvalue val = args[i];
            if(f_arg.getType() != val.getType()){
                throw IncorrectTypesException("Value passed to argument '" + f_arg.getName() + "' is incorrect", 
                val.getType(), f_arg.getType());
            }
            
            f_arg.setValue(std::move(val));
            MainScope.variables[f_arg.getName()] = std::move(f_arg);
        }
    }

    void setFunctionRef(Scope& r_scope){
        MainScope.functions.setMap(&r_scope.functions);
        MainScope.classes.setMap(&r_scope.classes);
        // MainScope.variables.setMap(&r_scope.variables);

    }
    
    void setFullRef(Scope& r_scope){
        MainScope.functions.setMap(&r_scope.functions);
        MainScope.variables.setMap(&r_scope.variables);
        MainScope.classes.setMap(&r_scope.classes);

    }

    Scope& getScope() {
        return MainScope;
    }

    const Function& getFunc(){
        return fun;
    }

};

#endif