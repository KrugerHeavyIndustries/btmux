#!/usr/bin/python

# Copyright (c) 1999-2002 Thomas Wouters
#   All rights reserved

import sys,struct,string

class _Typelist:
    def __init__(self, d):
        self.map = d
        for k,v in d.items():
            setattr(self, v, k)

_gnames = {
    0: "MECH",
    1: "DEBUG",
    2: "MECHREP",
    3: "MAP",
    4: "CHARGEN",
    5: "AUTO",
    6: "TURRET",
    7: "CUSTOM",
    8: "SCEN",
    9: "SSIDE",
    10: "SSOBJ",
    11: "SSINS",
    12: "SSEXT",
    13: "FMAP",
    14: "FMAPBLOCK",
    15: "FLOC",
    16: "FCHAR",
    17: "FOBJ",
    18: "FMOD",
# Disabled MT_LIST support
#    127: "MT_LIST"
}

gtypes = _Typelist(_gnames)

NUM_MAPOBJ = 10
DYNAMIC_MAGIC = 42
MAPFLAG_MAPO = 1
MAPOBJSTART_MAGICNUM = 27
MAPOBJEND_MAGICNUM = 39
TYPE_BITS = 8

class HCodeDB:
    def __init__(self, fp=None):
        self.data = []
        self.db = {}
        if fp:
            self.fp = fp
            self.readdb()

    def hcodeobj(self, key, type, size, data):
        if type == gtypes.MECH:
            return MECHObject(key, type, size, data)
        elif type == gtypes.MAP:
            return MAPObject(key, type, size, data)
        else:
            return HCodeObject(key, type, size, data)

    def readdb(self):
        # version, [key, type, size]+
        # version == char
        # key == int
        # type == unsigned char
        # size == unsigned short
        self.version = ord(self.fp.read(1))
        keydata = self.fp.read(4)
        while keydata and len(keydata) == 4:
            key = struct.unpack("=i", keydata)[0]
            if key < 0:
                break
            header = self.fp.read(3)
            type, size = struct.unpack("=bH", header)
            data = self.fp.read(size)
# Disabled MT_LIST support, MUX doesn't use it
#            data = [self.fp.read(size),]
#            if type == gtypes.MT_LIST:
#                sys.stderr.write("Found MT_LIST_TYPE.\n")
#                self.recursize_readlist(data)
            obj = self.hcodeobj(key, type, size, data)
            self.data.append(obj)
            self.db[key] = obj
            keydata = self.fp.read(4)

        sys.stderr.write("Done loading xcode tree: %d\n"%self.fp.tell())
        # Read postdata, objtypes and all
        # (God, this db format is f*cked)
        for meth in ("load_update1", "load_update2",
                     "load_update3", "load_update4"):
            for obj in self.data:
                if hasattr(obj, meth):
                    getattr(obj,meth)(self.fp)
            sys.stderr.write("Done pass " + meth + ": %d\n"%self.fp.tell())

            


class HCodeObject:
    def __init__(self, key, type, size, data):
        self.key = key
        self.type = type
        self.size = size
        self.data = data
    def __repr__(self):
        return "<HCodeObject key %s type %s>"%(self.key, gtypes.map[self.type])

# MECH struct is:
# dbref mynum (4 bytes)
# int mapnumber (4 bytes)
# dbref mapindex (4 bytes)
# char ID[2] (2 bytes (2-char string))
# char brief (1 byte)
# unsigned long tic[4][3] (48 bytes, 4-list of 3-list of int ?)
# char chantitle[16][16] (256 bytes, 16-list of lenght-15 strings (w/ nul byte))
# int freq[16] (64 bytes, 16-list of ints)
# int freqmodes[16] (64 bytes, 16-list of ints)
# --> mech_ud follows, which is:
#  char mech_name[31] (31 bytes, maybe no nul byte)
#  char mech_type[15] (15 bytes, maybe no nul byte)
#  char type (1 byte)
#  char move (1 byte)
#  int tons (4 bytes)
#  short radio_range (2 bytes)
#  char tac_range (1 byte)
#  char lrs_range (1 byte)
#  char scan_range (1 byte)
#  --> 8 times session_struct, which is:
#   unsigned char armor (1 byte)
#   unsigned char internal (1 byte)
#   unsigned char rear (1 byte)
#   unsigned char armor_orig (1 byte)
#   unsigned char internal_orig (1 byte)
#   unsigned char rear_orig (1 byte)
#   char basetohit (1 byte)
#   char config (1 byte)
#   char recycle (1 byte)
#   --> 12 times critical_slot, which is:
#    unsigned short type (2 bytes)
#    unsigned char data (1 byte)
#    unsigned short mode (2 bytes)
#    unsigned char brand (1 byte)
#   <-- end of critslot (6 bytes times 12 is 72 bytes)
#  <-- end of session_struct (81 bytes times 8 is 648 bytes)
#  char si (1 byte)
#  char si_orig (1 byte)
#  int fuel (4 bytes)
#  int fuel_orig (4 bytes)
#  float maxspeed (4 bytes)
#  char computer (1 byte)
#  char radio (1 byte)
#  char radioinfo (1 byte)
#  int mechbv (4 bytes)
#  int cargospace (4 bytes)
#  int unused[8] (32 bytes)
# <-- end of mech_ud (772 bytes ?)
#
# --> mech_pd, which is:
#  dbref pilot (4 bytes)
#  char pilotstatus (1 byte)
#  short hexes_walked (2 byte)
#  char terrian (1 byte)
#  char elev (1 byte)
#  short facing (2 bytes)
#  dbref master_c3_node (4 bytes)
#  char team (1 byte)
#  short x (2 bytes)
#  short y (2 bytes)
#  short z (2 bytes)
#  short last_x (2 bytes)
#  short last_y (2 bytes)
#  float fx (4 bytes)
#  float fy (4 bytes)
#  float fz (4 bytes)
#  int unusable_arcs (4 bytes)
#  int stall (4 bytes)
#  dbref bay[4] (16 bytes, 4-list of ints)
#  dbref turret[3] (12 bytes, 3-list of ints)
# <-- mech_pd (74 bytes)
#
# --> mech_rd, which is:
#  float startfx (4 bytes)
#  float startfy (4 bytes)
#  float startfz (4 bytes)
#  float endfz (4 bytes)

#  short jumplength (2 bytes)
#  char jumptop (1 byte)
#  short goingx (2 bytes)
#  short goingy (2 bytes)
#  float verticalspeed (4 bytes)
#  short desiredfacing (2 bytes)
#  short angle ( 2 bytes)
#  float speed ( 4 bytes)
#  float desired_speed ( 4 bytes)
#  float jumpspeed ( 4 bytes)
#  short jumpheading (4 bytes)
#  short targx (4 bytes)
#  short targy (4 bytes)
#  short targz (4 bytes)
#  short turretfacing (4 bytes)
#
#  char aim ( 1 byte)
#  char pilotskillbase (1 byte)
#  short turndamage (1 byte)
#  char basetohit (1 byte)
#
#  dbref chgtarget (4 byte)
#  dbref dfatarget (4 bytes)
#  dbref target (4 bytes)
#  dbref swarming (4 bytes)
#  dbref carrying (4 bytes)
#  dbref spotter (4 bytes)
#
#  char engineheat (1 byte)
#  float heat (4 bytes)
#  float weapheat (4 bytes)
#  float plus_heat (4 bytes)
#  float minus_heat (4 bytes)
#  int critstatus (4 bytes)
#  int status (4 bytes)
#  int specials (4 bytes)
#
#  char masc_value (1 byte)
#  time_t last_weapon_recycle (4 bytes)
#  char sensor[2] (2 bytes, 2-char string)
#  byte fire_adjustment (1 byte)
#  int cargo_weight (4 bytes)
#  short lateral (2 bytes)
#  short num_seen (2 bytes)
#  int lastrndu (4 bytes)
#  int rnd (4 bytes) (30 4bytes)
#  int last_ds_msg (4 bytes)
#  int boom_start (4 bytes)
#  int maxfuel (4 bytes)
#  int lastused (4 bytes)

#  int cocoon (4 bytes)
#  int commconv (4 bytes)
#  int commconv_last (4 bytes)
#  int onumsinks (4 bytes)
#  int disabled_hs (4 bytes)
#  int autopilot_num (4 bytes)
#  char aim_type (4 bytes)
#  int heatboom_last (4 bytes)
#  char vis_mod (4 bytes)
#  int sspin (4 bytes)
#  int can_see (4 bytes)
#  short lx (2 bytes)
#  short ly (2 bytes)
#  int row (4 bytes)
#  int rcw (4 bytes)
#  float rspd (4 bytes)
#  int erat (4 bytes)
#  int per (4 bytes)
#  int wxf (4 bytes)
#  char chargetimer (1 byte)
#  char chargedist (1 byte)
#  short mech_prefs (2 bytes)
#  int last_startup (4 bytes)
#  int specialsstatus (4 bytes)
#  int tankcritstatus (4 bytes)
#  int specials2 (4 bytes)
#  int unused[7] (28 bytes, 7-list of ints)
# <-- end of mech_rd (280 bytes)
# 
# Is a grand total of 1548 real bytes,
# 1752 with byte alignment, and 1768 with sub-struct alignment


class MECHObject(HCodeObject):
    def __init__(self, key, type, size, data):
        self.key = key
        self.type = type

        header_format = "iii2sb" + 4*3*"L" + 16*"16s" + "16i" + "16i"
        critslot_format = "HbHb0H"
        section_format = "BBBBBBbbb" + critslot_format*12
        ud_format = ("31s 15s bbiHbbb" + section_format*8 +
                    "bbiifbbbii8i0H" )
        pd_format = "ibHbbHibHHHHHfffii4i3i0H"
        rd_format = ("ffff HbHHfHHfffHHHHH bbHb iiiiii " "cffffiii"
                    "ci2bbiHHiiiiii" 
                    "iiiiiibibiiHHiifiiibbHiiii7i0H")

        self.format = ("@" + header_format + ud_format +
                       pd_format + rd_format)

        self.data = data
        self.parsedata(data, size)

    def parsedata(self, data, size):        

        class _Dummy:
            pass

        def _cull(s):
            return s[:string.find(s, "\000")]

        pdata = struct.unpack(self.format, data)
        (self.mynum, self.mapnumber, self.mapindex, self.ID,
         self.brief), pdata = pdata[:5], pdata[5:]
        self.tic = []
        for i in range(4):
            self.tic.append(pdata[:3])
            pdata = pdata[3:]

        self.chantitles, pdata = list(pdata[:16]), pdata[16:]
        for i in range(16):
            self.chantitles[i] = _cull(self.chantitles[i])
        self.freqs, pdata = pdata[:16], pdata[16:]
        self.freqmodes, pdata = pdata[:16], pdata[16:]

        ud = _Dummy()
        (ud.mech_name, ud.mech_type, type, move, tons, radio_range,
         tac_range, lrs_range, scan_range), pdata = pdata[:9], pdata[9:]
        ud.mech_name = _cull(ud.mech_name)
        ud.mech_type = _cull(ud.mech_type)
        ud.sections = []
        for i in range(8):
            section = _Dummy()
            (section.armor, section.internal, section.rear,
             section.armor_orig, section.internal_orig,
             section.rear_orig, section.basetohit, section.config,
             section.recycle), pdata = pdata[:9], pdata[9:]
            section.crits = []
            for i in range(12):
                crit = _Dummy()
                (crit.type, crit.data, crit.mode, crit.brand
                 ), pdata = pdata[:4], pdata[4:]
                section.crits.append(crit)
            ud.sections.append(section)
        (ud.si, ud.si_orig, ud.fuel, ud.fuel_orig, ud.maxspeed,
         ud.computer, ud.radio, ud.radioinfo, ud.mechbv, ud.cargospace,
         ), pdata = pdata[:10], pdata[10:]
        ud.unused, pdata = pdata[:8], pdata[8:]
        self.ud = ud

        pd = _Dummy()
        (pd.pilot, pd.pilotstatus, pd.hexes_walked, pd.terrain,
         pd.elev, pd.facing, pd.master_c3_node, pd.team, pd.x,
         pd.y, pd.z, pd.last_x, pd.last_y, pd.fx, pd.fy, pd.fz,
         pd.unusable_arcs, pd.stall), pdata = pdata[:18], pdata[18:]
        pd.bays, pdata = pdata[:4], pdata[4:]
        pd.turrets, pdata = pdata[:4], pdata[4:]
        self.pd = pd

        rd = _Dummy()
        (rd.startfx, rd.startfy, rd.startfz, rd.endfz, rd.jumplength,
         rd.jumptop, rd.goingx, rd.goingy, rd.verticalspeed,
         rd.desiredfacing, rd.angle, rd.speed, rd.desired_speed,
         rd.jumpspeed, rd.jumpheading, rd.targx, rd.targy, rd.targz,
         rd.turretfacing, rd.aim, rd.pilotskillbase, rd.turndamage,
         rd.basetohit, rd.chgtarget, rd.dfatarget, rd.target,
         rd.swarming, rd.carrying, rd.spotter, rd.engineheat, rd.heat,
         rd.weapheat, rd.plus_heat, rd.minus_heat, rd.critstatus,
         rd.status, rd.specials, rd.masc_value, rd.last_weapon_recycle,
         rd.sensor, rd.fire_adjustment, rd.cargo_weight, rd.lateral,
         rd.num_seen, rd.lastrndu, rd.rnd, rd.last_ds_msg, rd.boom_start,
         rd.maxfuel, rd.lastused, rd.cocoon, rd.commconv,
         rd.commconv_last, rd.onumsinks, rd.disabled_hs,
         rd.autopilot_num, rd.aim_type, rd.heatboom_last, rd.vis_mod,
         rd.sspin, rd.can_see, rd.lx, rd.ly, rd.row, rd.rcw, rd.rspd,
         rd.erat, rd.per, rd.wxf, rd.chargetimer, rd.chargedist,
         rd.mech_prefs, rd.last_startup, rd.specialstatus,
         rd.tankcritstatus, rd.specials2), pdata = pdata[:76],pdata[76:]

        rd.unused, pdata = pdata[:7], pdata[7:]
        self.rd = rd

        if pdata:
            sys.stderr.write("pdata left! %s\n"%(pdata,))
            sys.stderr.write("length of data: %s\n"%size)


# MAP struct is:
# dbref mynum (4 bytes)
# unsigned char * map[] (4 bytes)
# char mapname[31] (31 bytes)
# short map_width (2 bytes)
# short map_height (2 bytes)
# char temp ( 1 byte)
# unsigned char grav (1 byte)
# short cloudbase (2 bytes)
# char unused_char (1 byte)
# char mapvis (1 byte)
# short maxvis (2 bytes)
# char maplight (1 byte)
# short winddir (2 bytes)
# short windspeed (2 bytes)
# unsigned char flags (1 byte)
# struct mapobj * mapobj (4 bytes)
# short cf (2 bytes)
# short cfmax (2 bytes)
# dbref onmap (4 bytes)
# char buildflag (1 byte)
# unsigned char first_free (1 byte)
# dbref * mechsOnMap (4 bytes)
# unsigned short * LOSInfo[] (4 bytes)
# char * mechflags[] (4 bytes)
# short moves (1 byte)
# short movemod (1 byte)
#
# Resulting in:
# @ii31sHHbBHbbHbHHbiHHibBiiiHH

class MAPObject(HCodeObject):
    def __init__(self, key, type, size, data):
        self.key = key
        self.type = type

        self.format = "@ii31sHHbBHbbHbHHb10iHHibBiiiHH"
        self.parsedata(data, size)

    def parsedata(self, data, size):
        csize = struct.calcsize(self.format)
        if (size <> csize):
            sys.stderr.write("Wrong size: %d vs %d"%(size, csize))
            if size < csize:
                data += "\0"*(csize - size)
            else:
                data = data[:csize]

        pdata = list(struct.unpack(self.format, data))
        (self.mynum, x, self.name, self.width, self.height,
         self.temp, self.grav, self.cloudbase, self.unused, self.vis,
         self.maxvis, self.light, self.winddir, self.windspeed,
         self.flags), pdata = pdata[:15], pdata[15:]

        # Skip mapobjs
        pdata = pdata[10:]

        (self.cf, self.maxcf, self.onmap, self.buildflag,
         self.first_free, x, x, x,
         self.moves, self.movemod) = pdata

    def load_update1(self, fp):
        self.map = []
        self.losinfo = []
        num = self.first_free

        if num:
            fmt = "@" + "i" * num
            self.mechsonmap = _unpack(fmt, fp)
            fmt = "@" + "b" * num
            self.mechflags = _unpack(fmt, fp)
            fmt = "@" + "H" * num
            for x in range(num):
                self.losinfo.append(_unpack(fmt, fp))
        else:
            self.mechsonmap = []
            self.mapobj = []

        magic = ord(fp.read(1))
        if magic <> DYNAMIC_MAGIC:
            sys.stderr.write("Did not find DYNAMIC_MAGIC for #%d\n"%self.mynum)
            sys.stderr.write("Wanted %d, got %d\n"%(DYNAMIC_MAGIC, magic))
            sys.exit(1)
        
        if self.flags & MAPFLAG_MAPO:
            self.load_mapobj(fp)

    def load_mapobj(self, fp):
        magic = ord(fp.read(1))
        if magic <> MAPOBJSTART_MAGICNUM:
            sys.stderr.write("Did not find mapobjstartmagic for #%d\n"%self.mynum)
            sys.stderr.write("Wanted %d, got %d\n"%(MAPOBJSTART_MAGICNUM, magic))

        self.mapobj = []
        for i in range(NUM_MAPOBJ):
            self.mapobj.append([])

        nextbyte = ord(fp.read(1))
        while nextbyte:
            if nextbyte - 1 == TYPE_BITS:
                self.load_bits(fp)
            else:
                self.mapobj[nextbyte - 1].append(MapobjObject(fp))
            nextbyte = ord(fp.read(1))
        magic = ord(fp.read(1))
        if magic <> MAPOBJEND_MAGICNUM:
            sys.stderr.write("no mapobjend found for #%d!\n")
            sys.stderr.write("Wanted %d, got %d\n"%(MAPOBJEND_MAGICNUM, magic))

    def load_bits(self, fp):

        self.mapbits = []
        fmt = "@" + "i"*self.height
        foo = _unpack(fmt, fp)
        fmt = "@" + "B"*(self.width / 4 + ((self.width % 4) and 1 or 0))
        for x in foo:
            if x:
                self.mapbits.append(_unpack(fmt, fp))
            else:
                self.mapbits.append([])
                

class MapobjObject:
    def __init__(self, fp):
        fmt = "@HHiccHii"
        (self.x, self.y, self.obj, self.type, self.datac,
         self.datas, self.datai, x) = _unpack(fmt, fp)


def _unpack(fmt, fp):
    return list(struct.unpack(fmt, fp.read(struct.calcsize(fmt))))


if __name__ == "__main__":
    if len(sys.argv) <> 2:
        print "Usage: python -i hcodedb.py <hcodedbfile>"
        sys.exit()

    db = HCodeDB(open(sys.argv[1]))


