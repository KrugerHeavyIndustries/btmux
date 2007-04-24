#!/usr/bin/python

# Copyright (c) 1999-2002 Thomas Wouters
#   All rights reserved

import sys, string

builtin_attrnames = {
    "1": "OSUCC",
    "2": "OFAIL",
    "3": "FAIL",
    "4": "SUCC",
    "5": "PASS",
    "6": "DESC",
    "7": "SEX",
    "8": "ODROP",
    "9": "DROP",
    "10": "OKILL",
    "11": "KILL",
    "12": "ASUCC",
    "13": "AFAIL",
    "14": "ADROP",
    "15": "AKILL",
    "16": "AUSE",
    "17": "CHARGES",
    "18": "RUNOUT",
    "19": "STARTUP",
    "20": "ACLONE",
    "21": "APAY",
    "22": "OPAY",
    "23": "PAY",
    "24": "COST",
    "25": "MONEY",
    "26": "LISTEN",
    "27": "AAHEAR",
    "28": "AMHEAR",
    "29": "AHEAR",
    "30": "LAST",
    "31": "QUEUEMAX",
    "32": "IDESC",
    "33": "ENTER",
    "34": "OXENTER",
    "35": "AENTER",
    "36": "ADESC",
    "37": "ODESC",
    "38": "RQUOTA",
    "39": "ACONNECT",
    "40": "ADISCONNECT",
    "41": "ALLOWANCE",
    "42": "LOCK",
    "43": "NAME",
    "44": "COMMENT",
    "45": "USE",
    "46": "OUSE",
    "47": "SEMAPHORE",
    "48": "TIMEOUT",
    "49": "QUOTA",
    "50": "LEAVE",
    "51": "OLEAVE",
    "52": "ALEAVE",
    "53": "OENTER",
    "54": "OXLEAVE",
    "55": "MOVE",
    "56": "OMOVE",
    "57": "AMOVE",
    "58": "ALIAS",
    "59": "LENTER",
    "60": "LLEAVE",
    "61": "LPAGE",
    "62": "LUSE",
    "63": "LGIVE",
    "64": "EALIAS",
    "65": "LALIAS",
    "66": "EFAIL",
    "67": "OEFAIL",
    "68": "AEFAIL",
    "69": "LFAIL",
    "70": "OLFAIL",
    "71": "ALFAIL",
    "72": "REJECT",
    "73": "AWAY",
    "74": "IDLE",
    "75": "UFAIL",
    "76": "OUFAIL",
    "77": "AUFAIL",
    "78": "PFAIL",
    "79": "TPORT",
    "80": "OTPORT",
    "81": "OXTPORT",
    "82": "ATPORT",
    "83": "PRIVS",
    "84": "LOGINDATA",
    "85": "LTPORT",
    "86": "LDROP",
    "87": "LRECEIVE",
    "88": "LASTSITE",
    "89": "INPREFIX",
    "90": "PREFIX",
    "91": "INFILTER",
    "92": "FILTER",
    "93": "LLINK",
    "94": "LTELOUT",
    "95": "FORWARDLIST",
    "96": "MAILFOLDERS",
    "97": "LUSER",
    "98": "LPARENT",
    "100": "VA",
    "101": "VB",
    "102": "VC",
    "103": "VD",
    "104": "VE",
    "105": "VF",
    "106": "VG",
    "107": "VH",
    "108": "VI",
    "109": "VJ",
    "110": "VK",
    "111": "VL",
    "112": "VM",
    "113": "VN",
    "114": "VO",
    "115": "VP",
    "116": "VQ",
    "117": "VR",
    "118": "VS",
    "119": "VT",
    "120": "VU",
    "121": "VV",
    "122": "VW",
    "123": "VX",
    "124": "VY",
    "125": "VZ",

    "129": "GFAIL",
    "130": "OGFAIL",
    "131": "AGFAIL",
    "132": "RFAIL",
    "133": "ORFAIL",
    "134": "ARFAIL",
    "135": "DFAIL",
    "136": "ODFAIL",
    "137": "ADFAIL",
    "138": "TFAIL",
    "139": "OTFAIL",
    "140": "ATFAIL",
    "141": "TOFAIL",
    "142": "OTOFAIL",
    "143": "ATOFAIL",
    "144": "LASTNAME",
    "145": "HEATCHARS",
    "146": "MECHPREFID",
    "200": "LASTPAGE",
    "201": "MAIL",
    "202": "AMAIL",
    "203": "SIGNATURE",
    "204": "DAILY",
    "205": "MAILTO",
    "206": "MAILMSG",
    "207": "MAILSUB",
    "208": "MAILCURF",
    "209": "LSPEECH",
    "210": "PROGCMD",
    "211": "MAILFLAGS",
    "212": "DESTROYER",
    "213": "LUCK",
    "214": "MECHSKILLS",
    "215": "XTYPE",
    "216": "TACHEIGHT",
    "217": "LRSHEIGHT",
    "218": "CONTACTOPT",
    "219": "MECHNAME",
    "220": "MECHTYPE",
    "221": "MECHDESC",
    "222": "MECHSTATUS",
    "229": "MWTEMPLATE",
    "230": "FACTION",
    "231": "JOB",
    "232": "RANKNUM",
    "233": "HEALTH",
    "234": "ATTRS",
    "235": "BUILDLINKS",
    "236": "BUILDENTRANCE",
    "237": "BUILDCOORD",
    "238": "ADVS",
    "239": "PILOTNUM",
    "240": "MAPVIS",
    "241": "TZ",
    "242": "TECHTIME",
    "243": "ECONPARTS",
    "244": "SKILLS",
    "245": "PCEQUIP",
    "246": "HOURLY",
    "247": "HISTORY",
    "250": "VRML_URL",
    "251": "HTDESC"
}

class MUXDBLoadError(Exception):
    "Something went wrong with loading the DB"
    pass

class MUXFlatfileDB:
    def __init__(self, fp=None):
        self.db = {}
        self.dbtop = 0
        self.player_record = 0
        self.version = 0
        self.attrnames = builtin_attrnames.copy()
        if fp:
            self.readdb(fp)

    def readdb(self, fp):
        self.fp = fp
        version = self.fp.readline()
        if not version or version[:2] <> "+X":
            # Version starts with +X
            raise MUXDBLoadError, \
                  "Not a valid DB file (no first line or it doesn't start with '+X')"
        if version[2:-1] <> "992001":
            sys.stderr.write("flatfiledb: Warning: possibly wrong flatfile version"
                             "(%d insteas of %s)\n"%( version[2:-1], "992001"))
        self.version = int(version[2:-1])
        nextline = self.fp.readline()

        if nextline and nextline[:2] == "+S":
            self.dbtop = int(nextline[2:-1])
            nextline = self.fp.readline()

        if nextline and nextline[:2] == "+N":
            self.nextattr = int(nextline[2:-1])
            nextline = self.fp.readline()

        if nextline and nextline[:2] == "-R":
            self.player_record = int(nextline[2:-1])
            nextline = self.fp.readline()

        while nextline and nextline[:2] == "+A":
            # Attribute defn, next line is the attribute name
            attr = self.fp.readline()
            if attr[0] <> '"' or attr[-2] <> '"':
                sys.stderr.write("Warning: broken +A lines:\n%s%s"%(nextline, attr))
                nextline = self.fp.readline()
                continue
            # The attribute name is really "flags:name", but we don't care about flags yet
            attrname = string.split(attr[1:-2], ":", maxsplit=1)[1]
#            sys.stderr.write("debug: adding attr num %s (%s)\n"%(nextline[2:-1], nextline))
            self.attrnames[nextline[2:-1]] = attrname
            nextline = self.fp.readline()

        while nextline and nextline[0] == "!":
            # Next object.
            nextline = self.readobject(int(nextline[1:-1]))

        if not nextline or nextline <> "***END OF DUMP***\n":
            sys.stderr.write("Didn't find ***END OF DUMP***\n")
            if nextline:
                sys.stderr.write("The line read was: %s"%nextline)
        return

    def _readint(self):
        line = self.fp.readline()[:-1]
        try:
            return int(line)
        except:
            sys.stderr.write("warning: something went wrong with reading int: %s\n"%line)
            return -1

    def _readstr(self):
        return self.fp.readline()[1:-2]

    def _readattr(self):
        # If the line doesn't start with '"', it's broken
        # If the line doesn't *end* with '"', it's continued on the next line
        line = self.fp.readline()[:-1] # Strip the newline
        if line[0] <> '"':
            sys.stderr.write("flatfiledb: Warning: not a valid attr line: %s\n"%line)
        while line[-1] <> '"':
            line = line + self.fp.readline()[:-1]
        return line[1:-1]

    def readobject(self, objnum):
        obj = MUXDBObject(objnum)
        self.db[objnum] = obj

        # if not vlags & V_ATRNAME
        obj.name = self._readstr()
        obj.attrs["NAME"] = obj.name

        obj.location = self._readint()

        # if flags & V_ZONE)
        obj.zone = self._readint()
        
        obj.contents = self._readint()
        obj.exits = self._readint()

        # if flags & V_LINK)
        obj.link = self._readint()

        obj.next = self._readint()

        # if not flags & V_ATRKEY
        # Lock attribute... treat as string, even though it should be specially handled
        obj.lock = self._readstr()
        obj.attrs["LOCK"] = obj.lock
        # Ugly hack to work around silly db restriction
        nextline = self.fp.readline()
        if string.strip(nextline):
            obj.owner = int(nextline[:-1])
        else:
            obj.owner = self._readint()

        # if flags & V_PARENT
        obj.parent = self._readint()
        # if not flags & V_ATRMONEY
        obj.money = self._readint()

        obj.flags = self._readint()

        # if flags & V_XFLAGS
        obj.flags2 = self._readint()

        # if flags & V_3FLAGS
        obj.flags3 = self._readint()

        # if flags & V_POWERS
        obj.powers = self._readint()
        obj.powers2 = self._readint()

        # Read the attributes
        # This is really 'if not flags & V_GDBM' but we don't do GDBM db's (yet?)

        nextline = self.fp.readline()
        while nextline and nextline[0] == ">":
            attrnum = nextline[1:-1]
            obj.attrs[self.attrnames[attrnum]] = self._readattr()
            nextline = self.fp.readline()

        if nextline <> "<\n":
            sys.stderr.write("flatfiledb: Warning: attrlist ended with: %s"%nextline)

        return self.fp.readline()

    def attr_search(self, str):
        res = []
        for obj in self.db.values():
            res.extend(obj.attr_search(str))
        return res

class MUXDBObject:
    def __init__(self, objnum):
        self.objnum = objnum
        self.attrs = {}

    def attr_search(self, str):
        res = []
        for attr in self.attrs.keys():
            if string.find(self.attrs[attr], str) <> -1:
                res.append((self.objnum, attr, self.attrs[attr]))
        return res


if __name__ == "__main__":
    import sys
    if len(sys.argv) <> 2:
        print "Usage: python -i flatfiledb.py <flatfile>"
        sys.exit()
    db = MUXFlatfileDB(open(sys.argv[1]))
        
