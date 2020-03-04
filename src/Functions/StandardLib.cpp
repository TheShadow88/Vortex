#include "StandardLib.hpp"

StdLib::StdLib(Driver& drv) {

    std::vector<Var*> props = {
        new Var(Type::OBJECT, "vec"),
        new NativeMethod("construct", Type::VOID, [](std::vector<rvalue>&& args, Instance inst, Driver& drv){
            rvalue rv = args[0];
            rvalue vector_rv = inst.getProp("vec").getValue();
            auto vec = inst.getProp("vec").getValue<std::vector<rvalue>>();
            vec = rv.getValue<std::vector<rvalue>>();
            vector_rv.setValue(vec);
            std::cout << "Created vector" << std::endl;
        }),
        new NativeMethod("push", Type::VOID, [](std::vector<rvalue>&& args, Instance& inst, Driver& drv){
            rvalue rv = args[0];
            rvalue vector_rv = inst.getProp("vec").getValue();
            auto vec = inst.getProp("vec").getValue<std::vector<rvalue>>();
            vec.push_back(std::move(rv));
            for(auto it = vec.begin(); it != vec.end(); ++it){
                std::cout << *it << std::endl;
            }
            vector_rv.setValue(vec);
        })
    };
    Class vec = Class("vector",std::move(props), drv);

    classes.push_back(vec);
}