src_dir=../..
obj_dir=obj
inc_dirs=-I ../../../src

cxxflags=-MMD -g -O2 -march=native -Wno-unused-result -fno-strict-aliasing -fdata-sections \
	-ffunction-sections -wshadow

c_files_raw=\
	all.cpp \
	asteroids.cpp \
	colour_whirl.cpp \
	fractal_fern.cpp \
	gas.cpp \
	harmonograph.cpp \
	hello_world.cpp \
	lattice_boltzmann.cpp \
	mandelbrot.cpp \
	sierpinski_3d.cpp \
	stretch_blit.cpp \
	two_windows.cpp
c_files=$(addprefix $(src_dir)/,$(c_files_raw))
o_files=$(patsubst $(src_dir)/%.cpp,$(obj_dir)/%.o,$(c_files))
d_files=$(patsubst %.o,%.d,$(o_files))

all: $(obj_dir) $(o_files)
	g++ $(cxxflags) $(inc_dirs) $(o_files) -o $@ -L ../../../build/linux -l deadfrog

# This magic line makes g++ pay attention to the dependency files
-include $(d_files)

$(obj_dir)/%.o: $(src_dir)/%.cpp Makefile
	g++ $(cxxflags) $(inc_dirs) $< -c -o $@

$(obj_dir):
	mkdir -p $(obj_dir)

clean:
	rm -f $(o_files) $(d_files)
