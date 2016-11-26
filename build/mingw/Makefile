src_dir=../../src
obj_dir=obj
inc_dirs=

cxxflags=-MMD -O2 -march=native

c_files=$(wildcard $(src_dir)/*.cpp)
o_files=$(patsubst $(src_dir)/%.cpp,$(obj_dir)/%.o,$(c_files))
d_files=$(patsubst %.o,%.d,$(o_files))

deadfrog.a: $(obj_dir) $(o_files)
	ar cr $@ $(o_files)

# This magic line makes g++ pay attention to the dependency files
-include $(d_files)

$(obj_dir)/%.o: $(src_dir)/%.cpp
	g++ $(cxxflags) $(inc_dirs) $< -c -o $@

$(obj_dir):
	mkdir -p $(obj_dir)

clean:
	rm -f $(o_files) $(d_files)
