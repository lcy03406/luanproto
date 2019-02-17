SKYWORK_CXXFLAGS ?= -ggdb3 -O2 -Wall -fPIC -DPIC -std=c++1z 
CXXFLAGS = $(SKYWORK_CXXFLAGS) -I../../../dep/build/include -DLUACAPNP_PARSER
LDFLAGS = $(SKYWORK_LDFLAGS) -shared -L../../../lib -lcapnp-rpc -lcapnpc -lcapnp -lkj

all : luacapnp.so
	
.PHONY : all clean install

luacapnp.so : schema.o dynamic.o encode.o decode.o luacapnp.o luamodule.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

%.o : %.c++
	$(CXX) $(CXXFLAGS) -c $^ -o $@ 

clean :
	rm -f luacapnp.so *o

install : all
	mkdir -p ../../../luaclib
	cp -f luacapnp.so ../../../luaclib/

