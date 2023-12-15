all:
	gcc -I ../lib_ev/ -I ../diag_master_common/ \
		../diag_master_common/cjson.c \
		../diag_master_common/dm_common.c \
		../diag_master_common/dm_udsc_types.c \
		dm_uds_client.c \
		diag_master.c \
		dm_doip_client.c \
		main.c \
		-lpthread -L../lib_ev -lev -lm -o diag_master
clean:
	rm diag_master














