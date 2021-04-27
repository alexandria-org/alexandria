
cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
project(cc_parser LANGUAGES CXX)

find_package(aws-lambda-runtime REQUIRED)
find_package(AWSSDK COMPONENTS s3 transfer)
find_package(Boost REQUIRED COMPONENTS system iostreams)
find_package(ZLIB)

include_directories(src/)

add_executable(${PROJECT_NAME} "src/main.cpp" "src/entities.cpp" "src/HtmlParser.cpp" "src/Link.cpp")
add_executable(cc_indexer "src/indexer.cpp" "src/CCIndex.cpp" "src/TsvFile.cpp")
add_executable(run_tests "tests/test.cpp" "tests/test1.cpp" "tests/test2.cpp" "src/entities.cpp" "src/HtmlParser.cpp"
	"src/Link.cpp" "src/CCIndex.cpp" "src/TsvFile.cpp")

target_link_libraries(${PROJECT_NAME} PUBLIC
	AWS::aws-lambda-runtime 
	${AWSSDK_LINK_LIBRARIES} ${Boost_LIBRARIES} ZLIB::ZLIB)
target_link_libraries(cc_indexer PUBLIC
	AWS::aws-lambda-runtime 
	${AWSSDK_LINK_LIBRARIES} ${Boost_LIBRARIES} ZLIB::ZLIB)
target_link_libraries(run_tests PUBLIC
	AWS::aws-lambda-runtime 
	${AWSSDK_LINK_LIBRARIES} ${Boost_LIBRARIES} ZLIB::ZLIB)

aws_lambda_package_target(${PROJECT_NAME})
aws_lambda_package_target(cc_indexer)

