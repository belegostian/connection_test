PCAPPLUSPLUS_HOME := Drive:/your/PcapPlusPlus/folder
MINGW_HOME := Drive:/MinGW/folder
PCAP_SDK_HOME := Drive:/WpdPack/folder
### COMMON ###

# includes
PCAPPP_INCLUDES := -I$(PCAPPLUSPLUS_HOME)/header

# libs dir
PCAPPP_LIBS_DIR := -L$(PCAPPLUSPLUS_HOME)

# libs
PCAPPP_LIBS := -lPcap++ -lPacket++ -lCommon++

# post build
PCAPPP_POST_BUILD :=

# build flags
PCAPPP_BUILD_FLAGS := -fPIC -std=c++11

ifdef PCAPPP_ENABLE_CPP_FEATURE_DETECTION
        PCAPPP_BUILD_FLAGS += -DPCAPPP_CPP_FEATURE_DETECTION
endif

ifndef CXXFLAGS
CXXFLAGS := -O2 -g -Wall
endif

PCAPPP_BUILD_FLAGS += $(CXXFLAGS)
### WIN32 - MINGW-W64 ###

# includes
PCAPPP_INCLUDES += -I$(PCAP_SDK_HOME)/Include

# libs dir
PCAPPP_LIBS_DIR += -L$(PCAP_SDK_HOME)/lib -L$(PCAP_SDK_HOME)/lib/x64 -L$(MINGW_HOME)/lib

# libs
PCAPPP_LIBS += -lwpcap -lPacket -lws2_32 -liphlpapi

# flags
PCAPPP_BUILD_FLAGS += -std=gnu++11 -static -static-libgcc -static-libstdc++
