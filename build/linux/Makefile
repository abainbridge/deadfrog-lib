src_dir=../../src
obj_dir=obj
fonts_obj_dir=obj/fonts
inc_dirs=

cxxflags=-MMD -g -O2 -march=native -Wno-unused-result -fno-strict-aliasing -fdata-sections \
	-ffunction-sections -wshadow


c_files_raw=\
 fonts/df_mono.cpp \
 fonts/df_prop.cpp \
 df_bitmap.cpp \
 df_bmp.cpp \
 df_clipboard.cpp \
 df_colour.cpp \
 df_common_linux.cpp \
 df_font.cpp \
 df_message_dialog.cpp \
 df_polygon_aa.cpp \
 df_time.cpp \
 df_window.cpp
c_files=$(addprefix $(src_dir)/,$(c_files_raw))
o_files=$(patsubst $(src_dir)/%.cpp,$(obj_dir)/%.o,$(c_files))
d_files=$(patsubst %.o,%.d,$(o_files))


libdeadfrog.a: $(obj_dir) $(fonts_obj_dir) $(o_files)
	ar cr $@ $(o_files)

# This magic line makes g++ pay attention to the dependency files
-include $(d_files)

$(obj_dir)/%.o: $(src_dir)/%.cpp Makefile
	g++ $(cxxflags) $(inc_dirs) $< -c -o $@

$(obj_dir):
	mkdir -p $(obj_dir)

$(fonts_obj_dir):
	mkdir -p $(fonts_obj_dir)

clean:
	rm -rf $(obj_dir) $(o_files) $(d_files)
