################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/adaptive.cpp \
../src/flvreader.cpp \
../src/getinfo.cpp \
../src/getinfo_adaptive.cpp \
../src/helper.cpp \
../src/matroska.cpp \
../src/metrics.cpp \
../src/mpeg.cpp \
../src/mpeg_a.cpp \
../src/youtube-dl.cpp 

CC_SRCS += \
../src/curlops.cc 

OBJS += \
./src/adaptive.o \
./src/curlops.o \
./src/flvreader.o \
./src/getinfo.o \
./src/getinfo_adaptive.o \
./src/helper.o \
./src/matroska.o \
./src/metrics.o \
./src/mpeg.o \
./src/mpeg_a.o \
./src/youtube-dl.o 

CC_DEPS += \
./src/curlops.d 

CPP_DEPS += \
./src/adaptive.d \
./src/flvreader.d \
./src/getinfo.d \
./src/getinfo_adaptive.d \
./src/helper.d \
./src/matroska.d \
./src/metrics.d \
./src/mpeg.d \
./src/mpeg_a.d \
./src/youtube-dl.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -O0 -g3 -DDEBUG -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -O0 -g3 -DDEBUG -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


