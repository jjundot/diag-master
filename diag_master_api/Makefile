all:
	gcc -c -I ../diag_master_common/ \
		../diag_master_common/cjson.c \
		../diag_master_common/dm_common.c \
		../diag_master_common/dm_udsc_types.c \
		dm_api.c
	ar -crv libdm_api.a *.o
clean:
	rm libdm_api.a
	rm *.o














