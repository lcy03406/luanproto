CXXFLAGS = -std=c++11 -g -O2 -Wall -fPIC -shared -I../../libs/include -L../../libs/lib -lcapnp -lcapnpc -lkj -llua 

all : luanproto.so
	
.PHONY : all clean install

luanproto.so : src/luanproto.c++
	$(CXX) $(CXXFLAGS) $^ -o $@ 

clean :
	rm -f luanproto.so

install :
	cp -f luanproto.so ../../luaclib/

