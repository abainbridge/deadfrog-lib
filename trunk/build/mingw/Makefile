srcdir=../../src/lib

srcs=$(wildcard $(srcdir)/*.cpp)
hdrs=$(wildcard $(srcdir)/*.h)
objs=$(subst .cpp,.o,$(srcs))

cflags=-DSTATIC_LIB -Wall -I../../src -O2 -s
#cflags=-DDLL_EXPORTS -Wall -I../../src -O2 -s
#lflags=-lgdi32 -shared -Wl,--out-implib,deadfrog_dll.a
lflags=-lgdi32

deadfrog.a: $(objs) $(hdrs) Makefile
	ar rvs $(objs) -o deadfrog.a $(cflags) $(lflags)

clean:
	rm -f $(objs) deadfrog.dll deadfrog_dll.a
