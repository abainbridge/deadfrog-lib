src_dir=../../src
obj_dir=obj
inc_dirs=

cxxflags=-MMD -D_WIN32 -Os -march=native -Wno-unused-result -fno-strict-aliasing -ffunction-sections -fdata-sections -Wall -g

c_files_raw=df_bitmap.cpp df_bmp.cpp df_colour.cpp df_common.cpp \
	df_font.cpp df_message_dialog.cpp df_polygon_aa.cpp df_time.cpp \
	df_window.cpp fonts/df_mono.cpp fonts/df_prop.cpp
c_files=$(addprefix $(src_dir)/,$(c_files_raw))
o_files=$(patsubst $(src_dir)/%.cpp,$(obj_dir)/%.o,$(c_files))
d_files=$(patsubst %.o,%.d,$(o_files))


libdeadfrog.a: $(obj_dir) $(o_files)
	ar cr $@ $(o_files)

# This line tells Make to pay attention to the dependency files.
-include $(d_files)

$(obj_dir)/fonts/%.o: $(src_dir)/fonts/%.cpp Makefile
	g++ $(cxxflags) $(inc_dirs) $< -c -o $@

$(obj_dir)/%.o: $(src_dir)/%.cpp Makefile
	g++ $(cxxflags) $(inc_dirs) $< -c -o $@

$(obj_dir):
	mkdir -p $(obj_dir)

clean:
	rm -f $(o_files) $(d_files) libdeadfrog.a
