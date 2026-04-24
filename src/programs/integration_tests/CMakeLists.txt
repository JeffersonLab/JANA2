

set(JANA2_INTEGRATION_TEST_SOURCES 
    SimpleOffloading.cc
    BatchedArrow.cc
)

add_jana_test(jana-integration-tests SOURCES ${JANA2_INTEGRATION_TEST_SOURCES})


