common_files = common
naming_server_files = handle_client lru

common_files_h = $(foreach C,$(common_files),$C.h)
common_files_c = $(foreach C,$(common_files),$C.c)

naming_server_files_h = $(foreach C,$(naming_server_files),$C.h)
naming_server_files_c = $(foreach C,$(naming_server_files),$C.c)

compile: ${common_files_h} ${common_files_c} ${naming_server_files_h} ${naming_server_files_c} naming.c
	gcc -g ${common_files_c} ${naming_server_files_c} naming.c