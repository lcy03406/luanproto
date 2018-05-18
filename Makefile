CXXFLAGS = -std=c++11 -g -O2 -Wall -fPIC -I../../../dep/build/include -DLUACAPNP_PARSER -g
LDFLAGS = -shared -L../../../lib -lcapnp-rpc -lcapnpc -lcapnp -lkj

all : luacapnp.so
	
.PHONY : all clean install

luacapnp.so : luacapnp.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

%.o : %.c++
	$(CXX) $(CXXFLAGS) -c $^ -o $@ 

clean :
	rm -f luacapnp.so *o

install : all
	mkdir -p ../../../luaclib
	cp -f luacapnp.so ../../../luaclib/

