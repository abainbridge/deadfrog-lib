src_dir=../../src
obj_dir=obj
inc_dirs=

cxxflags=-MMD -g -march=native -Wno-unused-result -fno-strict-aliasing

c_files_raw=df_bitmap.cpp df_bmp.cpp df_colour.cpp df_common_linux.cpp \
	df_font.cpp df_message_dialog_linux.cpp df_polygon_aa.cpp df_time.cpp \
	df_window.cpp
c_files=$(addprefix $(src_dir)/,$(c_files_raw))
o_files=$(patsubst $(src_dir)/%.cpp,$(obj_dir)/%.o,$(c_files))
d_files=$(patsubst %.o,%.d,$(o_files))


libdeadfrog.a: $(obj_dir) $(o_files)
	ar cr $@ $(o_files)

# This magic line makes g++ pay attention to the dependency files
-include $(d_files)

$(obj_dir)/%.o: $(src_dir)/%.cpp Makefile
	g++ $(cxxflags) $(inc_dirs) $< -c -o $@

$(obj_dir):
	mkdir -p $(obj_dir)

clean:
	rm -f $(o_files) $(d_files)
