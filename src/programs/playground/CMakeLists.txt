
add_executable(playground
        playground.cc
        factory_gens_iter_1.cc
        Factory.cc
        Factory.h
        JContext.cc
        JContext.h
        FactoryGenerator.h
        Factory.cc
        JEvent2.h
        JAbstractFactory.h
        JLazyCollection.h
        )

target_link_libraries(playground jana2)
target_link_libraries(playground Threads::Threads)

