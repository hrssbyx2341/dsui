#inputShow:
#	aarch64-linux-gnu-gcc input_show.c ../../inputs/input_reader.c -o inputShow  -I ../../../third/libs/include/libdrm/ -L ../../../third/libs/lib -ldrm -lpthread
inputShow:
	aarch64-linux-gnu-g++ m_buffer_input_show_test.cpp ../../inputs/input_reader.c ../../display/m_buffer.cpp -o inputShow  -I ../../../third/libs/include/libdrm/ -L ../../../third/libs/lib -ldrm -lpthread
clean:
	rm inputShow
