add_executable(tst_scanengine tst_scanengine.cpp)
target_link_libraries(tst_scanengine PRIVATE dupfind_core Qt6::Test)
add_test(NAME scanengine COMMAND tst_scanengine)

add_executable(tst_duplicatesmodel
    tst_duplicatesmodel.cpp
    ../src/ui/DuplicatesModel.cpp
    ../src/ui/DuplicatesModel.h
)
target_link_libraries(tst_duplicatesmodel PRIVATE dupfind_core Qt6::Test)
add_test(NAME duplicatesmodel COMMAND tst_duplicatesmodel)

add_executable(tst_scanworker
    tst_scanworker.cpp
    ../src/worker/ScanWorker.cpp
    ../src/worker/ScanWorker.h
)
target_link_libraries(tst_scanworker PRIVATE dupfind_core Qt6::Test)
add_test(NAME scanworker COMMAND tst_scanworker)
