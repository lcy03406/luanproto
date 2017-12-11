CXXFLAGS = -std=c++11 -g -O2 -Wall -fPIC -I../build/include -DLUANP_PARSER
LDFLAGS = -shared -L../../lib -lcapnp-rpc -lcapnpc -lcapnp -lkj

all : luanproto.so
	
.PHONY : all clean install

luanproto.so : luanproto.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

%.o : %.c++
	$(CXX) $(CXXFLAGS) -c $^ -o $@ 

clean :
	rm -f luanproto.so *o

install : all
	mkdir -p ../../luaclib
	cp -f luanproto.so ../../luaclib/

