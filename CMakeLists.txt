
cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 11)
project(cc_parser LANGUAGES CXX)

find_package(aws-lambda-runtime REQUIRED)
find_package(AWSSDK COMPONENTS s3 transfer)
find_package(Boost REQUIRED COMPONENTS system iostreams)
find_package(ZLIB)

add_executable(${PROJECT_NAME} "src/main.cpp")
target_link_libraries(${PROJECT_NAME} PUBLIC
	AWS::aws-lambda-runtime 
	${AWSSDK_LINK_LIBRARIES} ${Boost_LIBRARIES} ZLIB::ZLIB)

aws_lambda_package_target(${PROJECT_NAME})

