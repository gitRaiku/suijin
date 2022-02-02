all: suijin shaders

.PHONY: clean shaders suijin tar test

CC = gcc
SHADER_CC = glslc
DATE := $(shell date "+%Y-%m-%d")

PROJECT_FILES = "main" "fujin"
SHADER_FILES  = "triangle.frag" "triangle.vert"
LIBRARY_FILES = "volk/volk"
OBJECT_FILES = `for f in ${PROJECT_FILES} ; do echo "src/$$f.o" ; done`

clean:
	for f in ${PROJECT_FILES} ; do echo rm -f src/$$f.o ; rm -f src/$$f.o ; done
	for f in ${SHADER_FILES} ; do echo rm -f shaders/$$f.spv ; rm -f shaders/$$f.spv ; done
	rm -f suijin
	
shaders:
	@for f in ${SHADER_FILES} ; do echo $(SHADER_CC) "shaders/$$f" -o "shaders/$$f.spv" ; $(SHADER_CC) "shaders/$$f" -o "shaders/$$f.spv" ; done

DEFINE_FLAGS = -D_DEBUG -D_GNU_SOURCE -DVK_USE_PLATFORM_XLIB_KHR -DGLFW_EXPOSE_NATIVE_X11
LINK_FLAGS = -lvulkan -lglfw

suijin: shaders
	@for f in ${PROJECT_FILES} ; do echo $(CC) -Og -g -ggdb3 -march=native -mtune=native $(DEFINE_FLAGS) -c -o "src/$$f.o" "src/$$f.c" ; $(CC) -Og -g -ggdb3 -march=native -mtune=native $(DEFINE_FLAGS) -c -o "src/$$f.o" "src/$$f.c" ; done

	$(CC) -Og -g -ggdb3 -march=native -mtune=native $(DEFINE_FLAGS) -o suijin $(OBJECT_FILES) $(LINK_FLAGS)
# What the fuck Makefile?
# Yeah gonna do this another time

tar:
	tar -czvf suijin-$(DATE).tar.gz Makefile src/* shaders/* AUTHORS

test:
	echo $(CC)
	echo $(SHADER_CC)
	echo $(CFLAGS)
	echo $(DEFINE_FLAGS)
	echo $(LINK_FLAGS)
	echo $(DATE)
