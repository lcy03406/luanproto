CXXFLAGS = -std=c++11 -g -O2 -Wall -fPIC -I../../libs/include
LDFLAGS = -shared -L../../libs/lib -lcapnp-rpc -lcapnpc -lcapnp -lkj


all : luanproto.so
	
.PHONY : all clean install

luanproto.so : src/luanproto.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

src/luanproto.o : src/luanproto.c++
	$(CXX) $(CXXFLAGS) -c $^ -o $@ 

clean :
	rm -f luanproto.so src/*o

install :
	cp -f luanproto.so ../../luaclib/

