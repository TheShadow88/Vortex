#ifndef PARSERTEST_HH
#define PARSERTEST_HH

#include <catch2/catch.hpp>
#include "../src/Driver/driver.hpp"
#include "../precompiled/parser.hh"

// #include "HelperTests.hpp"
#include <iostream>
#include <string>
#include <tuple>
#include <functional>
using namespace std;
using namespace Catch::Matchers;

// function<void(string)> generateExecutor(Driver& drv){
//     drv.disableReturnPrint();
//     return [&drv](string s)->void{
//         drv.evaluate(s.c_str());
//     };
// }
//function<void(Type)>, function<typename T, void(T)>, function<typename T, void(Type, T)>
auto generateExecutor(Driver& drv){
    drv.disableReturnPrint();
    return [&drv](string s)->void{
        drv.evaluate(s.c_str());
    };
}

auto generateValueCheckers(Driver& drv){
    auto f1= [&drv](Type expected)->void{
        CHECK(drv.getLastValue().getType() == expected);
    };

    auto f2= [&drv]<typename T>(T expected)->void{
        CHECK(drv.getLastValue().getValue<T>() == expected);
    };

    auto f3= [&drv]<typename T>(Type expected, T val)->void{
        CHECK(drv.getLastValue().getType() == expected);
        CHECK(drv.getLastValue().getValue<T>() == val);
    };
    
    return make_tuple(f1, f2, f3);

}

auto generateVectorChecks(Driver& drv){
    auto f1= [&drv]()->std::vector<rvalue>{
        return drv.getLastValue().getValue<Instance>().getProp("vec").getValue<std::vector<rvalue>>();
    };
    return f1;
}


TEST_CASE("Test variable creation"){
    Driver drv;
    drv.disableReturnPrint();
    yy::parser parser = drv.createParser();
    auto exec = generateExecutor(drv);
    const auto& [type_checker, value_checker, t_and_v_check] = generateValueCheckers(drv);


    SECTION("Set and get variables"){
        exec("int a = 3;");

        CHECK(drv.getLastValue().getValue<int>() == 3);

        exec("any v1 = 10;");

        CHECK(drv.getLastValue().getValue<int>() == 10);
        CHECK(drv.getVariable("v1").getType() == Type::ANY);

        exec("v1 = \"testing\";");

        CHECK(drv.getLastValue().getValue<string>() == "testing");
        CHECK(drv.getVariable("v1").getType() == Type::ANY);


        exec("b = 6;");

        CHECK(drv.getLastValue().getValue<int>() == 6);

        exec("a + b");

        CHECK(drv.getLastValue().getValue<int>() == 9);

    }
    

    SECTION("Full arithmetics"){
        exec("float f1 = 3.5;");
        exec("f1 + 1.3");
    
        t_and_v_check(Type::FLOAT, 4.8f);

        exec("2 + 3*2 - 4/2");

        t_and_v_check(Type::INT, 6);

        exec("2**2 - 5 * -1 + 1");
        t_and_v_check(Type::INT, 10);

        exec("5.5 - 4**0.5");
        t_and_v_check(Type::FLOAT, 3.5f);

         exec("2 * 0.5");
        t_and_v_check(Type::FLOAT, 1.0f);

         exec("3 + 3");
        t_and_v_check(Type::INT, 6);

         exec("1.2 - 0.2");
        t_and_v_check(Type::FLOAT, 1.0f);

         exec("\"hello \" + \"world\"");
        t_and_v_check(Type::STRING, std::string("hello world"));

        exec("\" strin\" + 'g'");
        t_and_v_check(Type::STRING, std::string(" string"));

        exec("\"testing\"[0]");
        t_and_v_check(Type::CHAR, 't');

        exec("\"testing\"[1]");
        t_and_v_check(Type::CHAR, 'e');

        exec("\"testing\"[2]");
        t_and_v_check(Type::CHAR, 's');

    }

}

TEST_CASE("Conditionals"){
    Driver drv;
    auto exec = generateExecutor(drv);
    const auto& [type_checker, value_checker, t_and_v_check] = generateValueCheckers(drv);
    SECTION("Basic tests"){
        exec("int res = 5;");
        exec("if(2 > 1) { res = 10; }");


        exec("res");
        t_and_v_check(Type::INT, 10);

        exec("15 if (2**2 - 3 > 5) { res = 20; } else { res = 35; }");

        exec("res");
        t_and_v_check(Type::INT, 35);
    }
}

TEST_CASE("Functions"){
    Driver drv;
    auto exec = generateExecutor(drv);
    const auto& [type_checker, value_checker, t_and_v_check] = generateValueCheckers(drv);
    SECTION("Declare and run"){
        CHECK_NOTHROW(drv.executeFile("tests/TestFiles/function_declare.vx"));

        exec("f1(3, 2)");

        t_and_v_check(Type::INT, 7);

        exec("f2()");

        t_and_v_check(Type::CHAR, 'c');

        exec("f3(69)");

        type_checker(Type::VOID);
        

    }

    SECTION("Recursion and ifs"){
        CHECK_NOTHROW(drv.executeFile("tests/TestFiles/function_declare.vx"));

        exec("max(3, 5)");

        t_and_v_check(Type::INT, 5);


        exec("fact(1)");

        t_and_v_check(Type::INT, 1);

        exec("fact(2)");

        t_and_v_check(Type::INT, 2);

        exec("fact(4)");

        t_and_v_check(Type::INT, 24);
    }
}

TEST_CASE("Vectors"){
    Driver drv;
    auto exec = generateExecutor(drv);
    const auto& [type_checker, value_checker, t_and_v_check] = generateValueCheckers(drv);
    const auto& vectorGet = generateVectorChecks(drv);

    SECTION("Creation"){
        exec("[]");
        type_checker(Type::OBJECT);

        exec("a = [\"str\", true, 3.4, 100, 'c'];");
        exec("a[0]");
        t_and_v_check(Type::STRING, std::string("str"));
        exec("a[1]");
        t_and_v_check(Type::BOOL, true);
        exec("a[2]");
        t_and_v_check(Type::FLOAT, 3.4f);
        exec("a[3]");
        t_and_v_check(Type::INT, 100);
        exec("a[4]");
        t_and_v_check(Type::CHAR, 'c');
        exec("a[4] = 200;");
        exec("a[4]");
        t_and_v_check(Type::INT, 200);
        exec("a[1] = 'j';");
        exec("a[1]");
        t_and_v_check(Type::CHAR, 'j');
    }

    SECTION("Methods"){
        exec("[].len()");
        t_and_v_check(Type::INT, 0);

         exec("a = [\"str\", true, 3.4, 100, 'c'];");

         exec("a.len()");
         t_and_v_check(Type::INT, 5);

         exec("a.remove(0)");
         exec("a.len()");
         t_and_v_check(Type::INT, 4);

         exec("a[0]");
         t_and_v_check(Type::BOOL, true);

         exec("a.push('c')");

         exec("a.len()");
         t_and_v_check(Type::INT, 5);

         exec("a[4]");
         t_and_v_check(Type::CHAR, 'c');
    }
}

TEST_CASE("Classes"){
    Driver drv;
    auto exec = generateExecutor(drv);
    const auto& [type_checker, value_checker, t_and_v_check] = generateValueCheckers(drv);
    SECTION("Declaration with extending"){
        CHECK_NOTHROW(drv.executeFile("tests/TestFiles/classes.vx"));

    }

    SECTION("Instances test"){
        CHECK_NOTHROW(drv.executeFile("tests/TestFiles/classes.vx"));

        exec("god1 = new God(20, \"god1\", [\"Programming\", \"Reading documentation\"]);");
        type_checker(Type::OBJECT);
        exec("god1.getName()");
        t_and_v_check(Type::STRING, std::string("god1"));
        exec("god1.getPowers()[0]");
        t_and_v_check(Type::STRING, std::string("Programming"));

        exec("stefan = new Person(18, \"Stefan\");");
        type_checker(Type::OBJECT);
        exec("stefan.getAge()");
        t_and_v_check(Type::INT, 18);

    }
}



#endif