#ifndef MAP_H
#define MAP_H

#define MAP_NAME_SIZE 30
#define NUM_MAPOBJTYPES 10

typedef int dbref;

typedef enum {
	GTYPE_MECH,
	GTYPE_DEBUG,
	GTYPE_MECHREP,
	GTYPE_MAP,
	GTYPE_AUTO,
	GTYPE_TURRET
} GlueType;

typedef struct {
	GlueType type;
	size_t size;
} XCODE;

typedef struct mapobj_struct {
	short x, y;
	dbref obj;
	char type;
	char datac;
	short datas;
	int datai;
	struct mapobj_struct *next;
} mapobj;

typedef struct {
	XCODE xcode;

	dbref mynum;
	unsigned char **map;
	char mapname[MAP_NAME_SIZE + 1];

	short map_width;
	short map_height;

	char temp;
	unsigned char grav;
	short cloudbase;
	char unused_char;
	char mapvis;
	short maxvis;
	char maplight;
	short winddir, windspeed;

	int flags;

	mapobj *mapobj[NUM_MAPOBJTYPES];
	short cf, cfmax;
	dbref onmap;
	char buildflag;

	unsigned char first_free;
	dbref *mechsOnMap;
	unsigned short **LOSinfo;

	char *mechflags;
	short moves;
	short movemod;
	int sensorflags;
} MAP;

#endif /* !MAP_H */
