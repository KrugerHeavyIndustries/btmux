
/*
   p.event.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Sat Jul 18 04:14:31 EEST 1998 from event.c */

#ifndef _P_EVENT_H
#define _P_EVENT_H

/* event.c */
void muxevent_add(int time, int flags, int type, void (*func) (MUXEVENT *),
    void *data, void *data2);
void muxevent_run(void);
int muxevent_run_by_type(int type);
int muxevent_last_type(void);
void muxevent_initialize(void);
void muxevent_remove_data(void *data);
void muxevent_remote_type_data(int type, void *data);
void muxevent_remote_type_data2(int type, void *data);
void muxevent_remote_type_data_data(int type, void *data, void *data2);
int muxevent_type_data(int type, void *data);
void muxevent_get_type_data(int type, void *data, long *data2);
int muxevent_count_type(int type);
int muxevent_count_type_data(int type, void *data);
int muxevent_count_type_data2(int type, void *data);
int muxevent_count_type_data_data(int type, void *data, void *data2);
int muxevent_count_data(int type, void *data);
int muxevent_count_data_data(int type, void *data, void *data2);
void muxevent_gothru_type_data(int type, void *data, void (*func) (MUXEVENT *));
void muxevent_gothru_type(int type, void (*func) (MUXEVENT *));
int muxevent_last_type_data(int type, void *data);
int muxevent_first_type_data(int type, void *data);
long muxevent_count_type_data_firstev(int type, void *data);

#endif				/* _P_EVENT_H */
