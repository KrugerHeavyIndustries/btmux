
                               CONTENTS
                               ========

I. Introduction
===============
a. The HUDINFO Protocol
b. About this document
c. The HUDINFO command
d. The HUDINFO Response Format
e. HUDINFO Errors

II. The special-case HUDINFO commands
=====================================
a. The argument-less 'hudinfo' command
b. The session-key setting 'hudinfo key' command.


III. Internal Data
==================
a. General Status
b. Weapon Status
c. Weapon List
d. Limb Status
e. Ammo Status
f. Static General Info
g. Original Armor Status
h. Armor Status

IV. External Data
=================
a. Contacts
b. Armor Scan
c. Weapon Scan
d. Tactical
e. Conditions
f. Building Contacts

V. Data Type Definitions
========================
a. Sizes
b. Type list
  advtech
  ammo type
  arc
  armor scan status
  building status string
  coordinate
  damage type
  degree
  fuel
  heatmeasure
  map condition flags
  mechid
  offset
  range
  section
  speed
  tactical string
  unit status string
  unit type character
  weapon fire mode
  weapon quality
  weapon scan status
  weapon status
  weapon type




                            I. Introduction
                            ===============


a.  The HUDINFO Protocol
------------------------

 The HUDINFO protocol is designed to allow easy communication between the
MUX server and any automated or semi-automated Battletech client. It
provides all info players can get through the conventional commands, but
drops issues such as readability and hysterical raisins in favour of
flexible and efficient communication.


b.  About this document
-----------------------

 This document describes version 0.7 of the HUDINFO specification. It is not
finished, should be considered a work in progress and should only be used as
a rough guideline in the development of a HUD client. Discussion about this
document, the standard described herein or other aspects of HUD clients can
be held at the mailinglist btech-discussions@lists.sourceforge.net, or the
HUD channel on 3030MUX.

 File: $Id: hudinfo.spec,v 1.1 2005/06/13 20:50:45 murrayma Exp $
 Last-Modified: $Date: 2005/06/13 20:50:45 $
 Last-Modified-By: $Author: murrayma $

 History: $Log: hudinfo.spec,v $
 History: Revision 1.1  2005/06/13 20:50:45  murrayma
 History: Fix a bad initial import.
 History:
 History: Revision 1.1.1.1  2005/06/13 20:18:06  murrayma
 History: Initial import from stable.
 History:
 History: Revision 1.1.1.1  2005/01/11 21:17:31  kstevens
 History: Initial import.
 History:
 History: Revision 1.9  2002/12/07 22:05:25  twouters
 History: Explicitly outlaw empty strings from all responses, and amend type
 History: specifications to do the same. This had been the intent all along, just lost
 History: track of it.
 History:
 History: Revision 1.8  2002/12/07 21:56:25  twouters
 History: Add jumptarget X/Y to the 'gs' command (sneak it into version 0.7)
 History:
 History: Revision 1.7  2002/12/06 23:03:28  twouters
 History: Added building contacts, map identifiers, names and version numbers, and
 History: weapon heat, all based on input on the discussion list.
 History:
 History: Revision 1.6  2002/02/26 12:48:46  twouters
 History: Fix example of 'T' and correct calculation in description.
 History:
 History: Revision 1.5  2002/02/06 22:32:16  twouters
 History: More reality seeping in: get rid of seperate 'side' and 'arm'
 History: arcs, and clarify 'sensors' string.
 History:
 History: Revision 1.4  2002/01/28 19:52:18  twouters
 History: HUDINFO spec meets reality, film at 11. Rework the 'weapon status' to be
 History: more consistent, some minor other changes.
 History:
 History: Revision 1.3  2002/01/22 22:41:57  twouters
 History: Re-work of multi-line responses, include a 'response type', clarification
 History: of error responses, some general typo fixes and clarifications.
 History:
 History: Revision 1.2  2002/01/22 17:15:04  twouters
 History: Lots of whitespace cleanup, some restructuring. Clarify 'light type'.
 History: Provide examples for all commands.
 History:
 History: Revision 1.1  2002/01/22 15:24:23  twouters
 History: The first public draft of the HUDINFO specification.


c.  The HUDINFO command
-----------------------

 The crux of the HUDINFO protcol is the 'hudinfo' command. Almost all of the
data is retrieved using the hudinfo command. All hudinfo responses meant to
be parsed by the software at the client-side are prefixed by (at least)
"#HUD". For everything but the argument-less 'hudinfo' command, the complete
prefix is built up as follows:

   #HUD:<key>:<class>:<type># 

where '<key>' is a session key, '<class>' is the identifier for the
subcommand the response comes from, and '<type>' is one of 'R' (single-line
response), 'L' (multi-line response list), 'S' (start of response list), 'D'
(end of response list) or 'E' (error response). Each 'class' of message has
a distinct response format for each 'type' of response, and all responses
come in that format. For future compatibility, extra fields at the end of
the format should be accepted but ignored.

 The session key is included for spoof protection. It has to be set before
issueing any hudinfo command that returns data (any hudinfo command except
the argument-less form or the 'hudinfo key' command), and for simplicity
must consist of alphanumerical characters alone, of no more than 20
characters. A simple 3-character key is deemed sufficient for common use.


d.  HUDINFO response format
---------------------------

 The actual message that follows the HUDINFO prefix in each message, is
presented as comma-seperated values, where the values can be of various
types. Values will never contain a comma, but extra trailing fields are
possible and should be ignored; they are a result of a new minor version or
experimental features.

 The actual values can be of different types types, where the type is
defined in this specification (and not, like XML, given in the response.)
The values are all the ASCII representations of the values, not their actual
byte value; so, the value 0 is represented as the string '0', not as the NUL
character, and the value 65 is '65', not 'A'. The complete list of types as
well as their definition and meaning can be found in section V, Data Type
Definitions.

 It should be noted that the intent of the specification is that there never
be empty items in responses; in other words, the sequence ',,' should never
occur in raw HUDINFO output. All strings should be at least one character
long, possibly supplying '-' as its only character. This is done to
facilitate easy parsing.


e.  HUDINFO Errors
------------------

 When the 'hudinfo' command needs to report an error, be it because an
operation cannot be performed, an unknown or illegal subcommand is issued or
because of internal problems, a response is generated of type 'E'. In the
case of an unknown or illegal subcommand, the 'class' of such a response is
set to '???'.

 Some error messages are common to all or almost all hudinfo commands; these
are listed here. Some error messages apply only to specific commands, and
are listed in those commands.

   "<subcommand>: subcommand not found":
	The subcommand given does not exist.

   "Not in a BattleTech unit":
	The command was issued outside of a BattleTech unit.

   "You are destroyed!":
	The command was issued in a destroyed BattleTech unit.

   "Reactor not online":
	The command was issued from a shutdown BattleTech unit, but
	requirs an operating one.

   "Unexpected internal error!":
	An internal error occured, report +bug.

   "No such target":
	When referencing a (currently) unknown mechid.

   "Out of range":
	The target or location is out of range.


                 II.  The special-case HUDINFO commands
                 ======================================

 The following special-cased hudinfo commands are special for two reasons:
they can (and should) be issued before a session key is set, and they can be
issued outside of a BattleTech unit. In addition, the response generated by
'hudinfo' is of a different format.

a. The argument-less 'hudinfo' command

   command:
	hudinfo
   response:
   Exactly one:
	#HUD hudinfo version 1.0 [options: <option flags>]
   Or exactly one:
        Huh?  (Type "help" for help.)

   Purpose: Identification

 The argument-less 'hudinfo' command is a special case of the 'hudinfo'
command, the only form that does not returns a response with a session key.
Instead, it returns a version identifier and a list of supported uptions, so
clients can autodetect whether a MUX supports hudinfo and if so, which
version.

 The version number is built up from 'major' and 'minor' version, both
integers, separated by a dot. The version number is not a floating point
number; 1.9 is smaller than 1.10, and not the same as 1.90. The major
version differs only between versions of the protocol that are wholly
incompatible; a HUD client that discovers a major mode it does not know
should not try to use hudinfo. The minor version is increased when new
standard features are added in a backwards-compatible manner; clients should
not break or refuse to work if the minor version is higher than expected,
though they may do so if the minor version is lower than required.

 The optional 'options' section, if present, should contain a list of option
flags for non-standard new features, which are used to add non-standard or
experimental features without creating conflicts on the version number. Such
options and their option flags should preferably be registered by mailing to
the contacts in section Ib, "About this Document", both for inclusion in the
next version of the protocol, and to avoid collisions with other options.

   Example:
    > hudinfo
    < #HUD hudinfo version 1.0


b. The session-key setting 'hudinfo key' command.

   command:
	hudinfo key=<key>
   response:
	#HUD:<key>:KEY:R# Key set

  This command is used to set the sesion key. It should be issued before any
hudinfo command other than the argument-less hudinfo, in order to set the
session key. The session key is a short (1-20 character) preferably
non-deterministic string of alphanumeric characters that serves as spoof
protection. The string is case-sensitive.

   Example:
    > hudinfo key=C58x2
    < #HUD:C58x2:KEY:R# Key set

   Error messages:

    "Invalid key":
	Invalid characters used in key, or key too long.


                            I. Internal Data
                            ================

a. General Status

   command:
	hudinfo gs

   response:
   Exactly once:
	#HUD:<key>:GS:R# ID,X,Y,Z,CH,DH,CS,DS,CH,HD,CF,CV,DV,RC,BC,TU,FL,JX,JY

   ID: mechid, own mech ID
   X, Y, Z : coordinates, current location
   CH: degree, current heading
   DH: degree, desired heading
   CS: speed, current speed
   DS: speed, desired speed
   CH: heatmeasure, current heat
   HD: heatmeasure, current heat dissipation
   CF: fuel or '-', current fuel or '-' if not applicable)
   CV: speed, current vertical speed
   DV: speed, desired vertical speed
   RC: range, range to center of current hex
   BC: degree, bearing of center of current hex
   TU: offset or '-', torso/turret facing offset (or '-' if not applicable)
   FL: Unit status flags
   JX, JY: jump X/Y targets (or - if not jumping)

   Example:
    > hudinfo gs
    < #HUD:C58x2:GS:R# QQ,5,5,0,0,0,32.25,43.0,10,120,-,0,0,0.2,179,0,L,-,-


b. Weapon Status

   command:
	hudinfo we

   response:
   Zero or more:
	#HUD:<key>:WE:L# WN,WT,WQ,LC,ST,FM,AT
   Exactly once:
	#HUD:<key>:WE:D# Done

   WN: integer, weapon number
   WT: weapon type number
   WQ: weapon quality
   LC: section, location of weapon
   ST: weapon status
   FM: weapon fire mode
   AT: ammo type, the type of ammo selected

   Purpose:
 	To retrieve detailed information about the status of a units
	weapons. One response is generated for each weapon on the unit,
	functional or not, and a single "Done" message signals the end of
	the list. Weapons are returned in unspecified order.

   Example:
    > hudinfo we
    < #HUD:C58x2:WE:L# 0,1,3,RA,-,h,-
    < #HUD:C58x2:WE:L# 1,2,4,LL,29,H,A
    < #HUD:C58x2:WE:L# 5,2,1,CT,D,H,-
    < #HUD:C58x2:WE:D# Done


c. Weapon List

   command:
	hudinfo wl

   repsonse:
   Zero or more:
	#HUD:<key>:WL:L# WN,NM,NR,SR,MR,LR,NS,SS,MS,LS,CR,WT,DM,RT,FM,AT,DT
   Exactly one:
	#HUD:<key>:WL:D# Done

   WN: integer, weapon number
   NM: string, weapon name
   NR: range, minimum range
   SR: range, short range
   MR: range, medium range
   LR: range, long range
   NS: range, minimum range in water
   SS: range, short range in water
   MS: range, medium range in water
   LS: range, long range in water
   CR: integer, size in critslots
   WT: integer, weight in 1/100 tons
   DM: integer, maximum damage
   RT: integer, recycle time in ticks
   FM: weapon fire mode, possible fire modes
   AT: ammo type, possible ammo types
   DT: damage type
   HT: heat measure, weapon heat per salvo

   Example:
    > hudinfo wl
    < #HUD:C58x2:WL:L# 0,Odd Laser,0,2,4,6,0,1,2,3,1,100,6,30,-,E,30
    < #HUD:C58x2:WL:L# 1,Heavy Odd Pulse Flamer-Laser,0,4,6,8,0,1,2,3,2,500,12,30,h,Eph,100
    < #HUD:C58x2:WL:L# 2,LRM-6,6,14,18,24,-,-,-,-,4,700,6,30,H,1ACNMSs,Mg,50
    < #HUD:C58x2:WL:D# Done

d. Limb Status

   command:
	hudinfo li
 
   response:
   Zero or more:
	#HUD:<key>:LI:L# SC,ST
   Exactly once:
	#HUD:<key>:LI:D# Done

   SC: section, only applicable sections are Arms/Legs and Suit1.
   ST: weapon status, status for this section.

   Example:
    > hudinfo li
    < #HUD:C58x2:WL:L# LA,D
    < #HUD:C58x2:WL:L# RA,22
    < #HUD:C58x2:WL:L# LL,-
    < #HUD:C58x2:WL:L# RL,-
    < #HUD:C58x2:WL:D# Done

e. Ammo status

   command:
	hudinfo am

   response:
   Zero or more:
	#HUD:<key>:AM:L# AN,WT,AM,AR,FR
   Exactly once:
	#HUD:<key>:AM:D# Done

   AN: integer, ammo bin number
   WT: weapon type, what the ammo is for
   AM: ammo mode
   AR: integer, rounds remaining
   FR: integer, full capacity

   Example:
    > hudinfo am
    < #HUD:C58x2:AM:L# 0,2,A,20,24
    < #HUD:C58x2:AM:L# 1,2,C,12,12
    < #HUD:C58x2:AM:L# 2,2,-,48,48
    < #HUD:C58x2:AM:D# Done


f. Static General info

   command:
	hudinfo sgi

   response:
   Exactly once:
	#HUD:<key>:SGI:R# TC,RF,NM,WS,RS,BS,VS,TF,HS,AT

   TC: unit type character
   RF: string, unit referece
   NM: string, unit name
   WS: speed, unit max walking/cruise speed
   RS: speed, unit max running/flank speed
   BS: speed, unit max reverse speed
   VS: speed, unit max vertical speed
   TF: fuel, or '-' for n/a
   HS: integer, number of templated (single) heatsinks
   AT: advtech, advanced technology available or '-' if n/a

   Example:
     > hudinfo sgi
     < #HUD:C58x2:SGI:R# B,MAD-1W,Mawauder,43,64,43,0,-,12,S


g. Original Armor Status
   
   command:
   	hudinfo oas

   response:
   Zero or more:
   	#HUD:<key>:OAS:L# SC,AF,AR,IS
   Exactly once:
   	#HUD:<key>:OAS:D# Done
   	
   SC: section
   AF: integer, original front armor or '-' if n/a
   AR: integer, original rear armor or '-' if n/a
   IS: integer, original internal structure or '-' if n/a

   Example:
    > hudinfo oas
    < #HUD:C58x2:OAS:L# LA,22,-,12
    < #HUD:C58x2:OAS:L# RA,22,-,12
    < #HUD:C58x2:OAS:L# LT,17,8,16
    < #HUD:C58x2:OAS:L# RT,17,8,16
    < #HUD:C58x2:OAS:L# CT,35,10,23
    < #HUD:C58x2:OAS:L# LL,18,-,16
    < #HUD:C58x2:OAS:L# RL,18,-,16
    < #HUD:C58x2:OAS:L# H,9,-,3
    < #HUD:C58x2:OAS:D# Done

h. Armor Status

   command:
   	hudinfo as
   response:
   Zero or more:
   	#HUD:<key>:AS:L# SC,AF,AR,IS
   Exactly once:
   	#HUD:<key>:AS:D# Done
   	
   SC: section
   AF: integer, front armor or '-' if n/a
   AR: integer, rear armor or '-' if n/a
   IS: integer, internal structure or '-' if n/a, 0 if destroyed

   Example:
    > hudinfo as
    < #HUD:C58x2:AS:L# LA,0,-,0
    < #HUD:C58x2:AS:L# RA,22,-,12
    < #HUD:C58x2:AS:L# LT,9,5,16
    < #HUD:C58x2:AS:L# RT,4,1,16
    < #HUD:C58x2:AS:L# CT,0,10,5
    < #HUD:C58x2:AS:L# LL,0,-,16
    < #HUD:C58x2:AS:L# RL,18,-,16
    < #HUD:C58x2:AS:L# H,9,-,3
    < #HUD:C58x2:AS:D# Done


                           II. External Data
                           =================
                  
a. Contacts

   command:
  	hudinfo c

   response:
   Zero or more:
  	#HUD:<key>:C:L# ID,AC,SE,UT,MN,X,Y,Z,RN,BR,SP,VS,HD,JH,RTC,BTC,TN,HT,FL
   Exactly once:
  	#HUD:<key>:C:D# Done

   ID: mechid, ID of the unit
   AC: arc, weapon arc the unit is in
   SE: sensors, sensors that see the unit
   UT: unit type character
   MN: string, mechname of unit, or '-' if unknown
   X, Y, Z: coordinates of unit
   RN: range, range to unit
   BR: degree, bearing to unit
   SP: speed, speed of unit
   VS: speed, vertical speed of unit
   HD: degree, heading of unit
   JH: degree, jump heading, or '-' if not jumping
   RTC: range, range from unit to X,Y center
   BTC: degree, bearing from unit to X,Y center
   TN: integer, unit weight in tons
   HT: heatmeasure, unit's apparent heat (overheat)
   FL: unit status string

   Example:
    > hudinfo c
    < #HUD:C58x2:C:L# DV,r*,PS,B,Pheonix-Hawg,5,3,1,1.6,2,0.0,0.0,180,-,0.2,0,45,0,BSl
    < #HUD:C58x2:C:D# Done

b. Armor Scan

  command:
	hudinfo asc <mechid>

  response:
  Zero or more:
	#HUD:<key>:ASC:L# SC,AF,AR,IS
  Exactly once:
	#HUD:<key>:ASC:D# Done

  SC: section
  AF: section scan status, front armor
  AR: section scan status, rear armor
  IS: section scan status, internal structure, '*' for destroyed

  This command generates a 'You are being scanned by XX' message in the
  scanned unit.

   Example:
    > hudinfo asc DV
    < #HUD:C58x2:ASC:L# LA,O,-,O
    < #HUD:C58x2:ASC:L# RA,O,-,O
    < #HUD:C58x2:ASC:L# LT,o,O,O
    < #HUD:C58x2:ASC:L# RT,x,O,O
    < #HUD:C58x2:ASC:L# CT,*,O,o
    < #HUD:C58x2:ASC:L# LL,O,-,O
    < #HUD:C58x2:ASC:L# RL,O,-,O
    < #HUD:C58x2:ASC:L# H,*,-,*
    < #HUD:C58x2:ASC:D# Done


c. Weapon Scan

   command:
	hudinfo wsc <mechid>

   response:
   Zero or more:
	#HUD:<key>:WSC:L# WI,SC,ST
   Exactly once:
	#HUD:<key>:WSC:D# Done

   WN: integer, weapon number on this scan[*]
   WI: weapon index, type of the weapon
   SC: section
   ST: weapon scan status, apparent state of the weapon.

   [*] weapon numbers are not persistent across scans.

   This command generates a 'You are being scanned by XX' message in the
   scanned unit.

   Example:
    > hudinfo wsc DV
    < #HUD:C58x2:WSC# 0,1,LA,R
    < #HUD:C58x2:WSC# 1,2,LA,-
    < #HUD:C58x2:WSC# 2,0,LA,*
    < #HUD:C58x2:WSC# 3,1,LA,*
    < #HUD:C58x2:WSC# 4,1,RA,-
    < #HUD:C58x2:WSC# 5,2,RA,R
    < #HUD:C58x2:WSC# 6,0,RA,R
    < #HUD:C58x2:WSC# 7,1,RA,-
    < #HUD:C58x2:WSC# 8,0,CTr,R
    < #HUD:C58x2:WSC# 9,0,RTr,R
    < #HUD:C58x2:WSC# 10,0,LTr,R
    < #HID:C58x2:WSC# Done


d. Tactical

   command:
	hudinfo t [ <height> [ <range> <bearing> [ l ] ] ]

   response:
   Exactly once:
	#HUD:<key>:T:S# SX,SY,EX,EY,MI,MN,MV
   Once or more:
	#HUD:<key>:T:L# Y,TS
   Exactly once:
	#HUD:<key>:T:D# Done

   SX: coordinate, Start X
   SY: coordinate, Start Y
   EX: coordinate, End X
   EY: coordinate, End Y
   MI: map identifier
   MN: map name
   MV: map version number

   Y: coordinate, Y coordinate for line
   TS: tactical string, of length (EX-SX + 1)*2

   The terrain string (TS) is a special type of string. It is built up out of
   pairs of characters for terrain elevation and terrain type. One pair (two
   characters) per X-coordinate of the tactical string. Water hexes have
   depth instead of height (their top level is always 0).

   The MI, MN and MV items are not mandatory, and should be -1 when not
   supported or disabled by the game administrator(s).

   If the fourth argument is passed in and is 'l', a line-of-sight tactical
   (if available) will be returned, where all unknown terrain and/or height
   is '?'.

   Example:
    > hudinfo t 5
    < #HUD:C58x2:T:S# 11,10,29,14,-1,-1,-1
    < #HUD:C58x2:T:L# 10,#0"0"0"0"0"0"0"0'0~1.0.0.0.0'0'0'0'0'0
    < #HUD:C58x2:T:L# 11,"0#0"0"0"0"0"0'0'0~1'0.0.0.0.0.0.0.0.0
    < #HUD:C58x2:T:L# 12,^9#0#0"0"0"0"0'0~1'0'0'0.0.0.0.0.0.0.0
    < #HUD:C58x2:T:L# 13,^9#1^9#0"0"0'0~1'0.0'0.0.0.0.0.0.0.0.0
    < #HUD:C58x2:T:L# 14,^9#2^9^9#0'0'0'0~1'0.0.0.0.0.0.0.0.0.0
    < #HUD:C58x2:T:D# Done

e. Conditions

   command:
	hudinfo co

   response:
   Exactly once:
	#HUD:<key>:CO:R# LT,VR,GR,TP,FL

   LT: light type
   VR: range, visibility range
   GR: integer, gravity in 100th G's
   TP: heatmeasure, ambient temperature
   FL: map condition flags

   Example:
    > hudinfo co
    < #HUD:C58x2:CO:R# D,60,100,21,UD

f. Building Contacts

   command:
	hudinfo cb

   response:
   Zero or more:
	#HUD:<key>:CB:L# AC,BN,X,Y,Z,RN,BR,CF,MCF,BS
   Exactly once:
	#HUD:<KEY>:CB:D# Done

   AC: arc, weapon arc the building is in
   BN: string, name of the building, or '-' if unknown
   X, Y, Z: coordinates of building
   RN: range, range to building
   BR: degree, bearing to building
   CF: integer, current construction factor of building
   MCF: integer, maximum construction factor of building
   BS: building status string

   Example:
    > hudinfo cb
    < #HUD:C58x2:CB:L# *,Underground Hangar,55,66,7,25.1,180,1875,2000,X
    < #HUD:C58x2:CB:D# Done


                        V. Data Type Definitions
                        ========================

a. Sizes

 All integers are at most 32 bit signed ints. All floats are at most C
doubles. All strings are variable-length with a maximum length of 8192
characters, ASCII encoded.

b. Type list

  advtech: '-', or at least one of the following:
    'A': Angel ECM
    'B': Beagle Active Probe
    'C': C3I suite
    'E': Guardian ECM
    'I': Infiltrator-II Stealth
    'J': Jettisonable Backpack
    'K': Kage Stealth
    'L': AntiLeg Attack
    'M': MASC
    'N': Null Signal System
    'P': Personal ECM
    'R': Radar
    'S': Searchlight
    'T': TAG
    'W': Swarm Attack
    'a': Achileus Stealth
    'b': Bloodhound Active Probe
    'c': C3 Slave
    'f': Mount Friends
    'i': Infiltrator Stealth
    'j': Must jettison backpack before swarm/antileg attacks
    'm': C3 Master
    'p': Purifier Stealth
    's': Stealth Armor
    't': Triple Strength Myomer

  ammo type: '-', or at least one of the following characters:
    '1': Swarm1 mode
    'A': Artemis mode
    'C': Cluster mode
    'E': iNARC ECM pods
    'F': Flechette mode
    'H': iNARC Haywire mode
    'I': Inferno mode
    'L': LBX mode
    'M': Mine mode
    'N': NARC-guided mode
    'P': Precision mode
    'S': Swarm mode
    'T': Stinger mode
    'a': Armor Piercing mode
    'e': iNARC Explode mode
    'i': Incendiary mode
    's': Smoke mode
      
  arc: One or more of the following:
    '*': Forward arc
    'r': Right side arc
    'l': Left side arc 
    'v': Rear arc
    't': Turret arc
    '-': No arc

  armor scan status: One of the following:
    'O': 90% or more intact
    'o': 70% to 90% intact
    'x': 45% to 70% intact
    'X': 1% to 45% intact
    '*': completely gone
    '-': not applicable
    '?': under repair

  building status string: '-', or at least one of the following characters:
    'C': Combat safe
    'E': Hostile
    'F': Friendly
    'H': Hidden
    'X': Unbreakably locked 
    'x': Breakably locked

  coordinate: integer used for X, Y and Z coordinates, where the Z axis is
	      scaled down by a factor of five. (one X is 5 Z's)

  damage type: Exactly one of the following type chars:
    'A': Artillery
    'B': Ballistic
    'E': Energy
    'F': Flamer
    'G': Gauss
    'M': Missile
               Followed by zero of more of the following modifiers:
    'C': Caseless
    'D': Dead-Fire Missile
    'H': Heavy Gauss
    'P': Player Character weapon
    'Y': Hyper
    'a': A-Pod
    'd': Defensive only (cannot fire)
    'e': ELRM
    'g': Grouping damage (clusters of 5)
    'h': Heavy Laser
    'm': MRM
    'p': Pulse
      
  degree: integer, ranging from 0 to 359, where 0 is directly north

  fuel: integer, measure of fuel (not percentage.)

  heatmeasure: integer, degrees celcius (1 heatpoint is 10 degrees celcius)

  map condition flags: '-', or at least one of the following:
    'V': Map is Vacuum
    'U': Map is Underground (no jumping, VTOLs)
    'D': Map is Dark (no tactical beyond sensors)

  map identifier: string or -1. Non-mandatory unique identifier for an
                  instance of a map (such as its dbref.) The special value
                  -1 indicates it is not supported (or disabled.)

  map name: string or -1. Non-mandatory name of the map. No guarantees
            are made about the equivalence of two maps with the same name.
            The special value -1 indicates it is not supported (or
            disabled.)

  map version identifier: string or -1. Non-mandatory version number of the
                          map. When supported, it should be changed whenever
                          the map is modified. The special value -1
                          indicates it is not supported (or disabled.)

  mechid: string, uniquely identifies unit on current map.

  light type: One of the following:
    'D': Daylight
    'T': Twilight
    'N': Night

  offset: integer, ranging from -180 to 179, relative to unit facing

  range: float, distance measured in hexes, with two decimal places.

  section: Exactly one of the following strings:
    '-': No particular section
    'A': Aft (Aerodyne Dropship and Spheroid Dropship)
    'AS': Aft Side (Vehicle)
    'C': Cockpit (AeroFighter and Aerodyne Dropship)
    'CT': Center Torso (Quad and 'Mech)
    'CTr': Center Torso (Rear) (Quad and 'Mech)
    'E': Engine (AeroFighter)
    'F': Fuselage (AeroFighter)
    'FLLr': Front Left Leg (Rear) (Quad)
    'FLS': Front Left Side (Spheroid Dropship)
    'FRLr': Fron Right Leg (Rear) (Quad)
    'FRS': Front Right Side (Spheroid Dropship)
    'FS': Front Side (Vehicle)
    'H': Head ('Mech)
    'Hr': Head (Rear) ('Mech)
    'LA': Left Arm ('Mech)
    'LAr': Left Arm (Rear) ('Mech)
    'LL': Left Leg ('Mech)
    'LLr': Left Leg (Rear) ('Mech)
    'LRW': Left Rear Wing (Aerodyne Dropship)
    'LS': Left Side (Vehicle)
    'LT': Left Torso (Quad, 'Mech)
    'LTr': Left Torso (Rear) (Quad, 'Mech)
    'LW': Left Wing (AeroFighter and Aerodyne Dropship)
    'N': Nose (AeroFighter, Aerodyne Dropship and Spheroid Dropship)
    'R': Rotor (VTOL)
    'RA': Right Arm ('Mech)
    'RAr': Right Arm (Rear) ('Mech)
    'RL': Right Leg ('Mech)
    'RLr': Right Leg (Rear) ('Mech)
    'RLS': Rear Left Side (Spheroid Dropship)
    'RRS': Rear Right Side (Spheroid Dropship)
    'RRW': Right Rear Wing (Aerodyne Dropship)
    'RS': Right Side (Vehicle)
    'RT': Right Torso (Quad, 'Mech)
    'RTr': Right Torso (Rear) (Quad, 'Mech)
    'RW': Right Wing (AeroFighter and Aerodyne Dropship)
    'S1': Suit 1 (BattleSuit)
    'S2': Suit 2 (BattleSuit)
    'S3': Suit 3 (BattleSuit)
    'S4': Suit 4 (BattleSuit)
    'S5': Suit 5 (BattleSuit)
    'S6': Suit 6 (BattleSuit)
    'S7': Suit 7 (BattleSuit)
    'S8': Suit 8 (BattleSuit)
    'T': Turret (turreted Vehicle)
      Strings not in this list should be considered '-', for forward
      compatibility.

  sensors: '-' or at least one of the following characters:
    'P': Primary sensor
    'S': Secondary sensor

  speed: float, in kph, with two decimal places.

  tactical string: One or more times each of the following two:
                   One Type char:
    '"': Heavy woods
    '#': Road
    '%': Rough
    '&': Fire
    '+': Snow
    '-': Ice
    '.': Plains
    '/': Bridge
    ':': Smoke
    '=': Pavement/Wall
    '?': Unknown terrain
    '@': Building
    '^': Mountain
    '`': Light woods
    '~': Water
                   One Elevation char:
    '0' through '9': Terrain height
    '?': Unknown height

  unit status string: '-', or at least one of the following characters:
    'B': Burning
    'C': Carrying a club
    'd': Dug in
    'D': Destroyed
    'e': Affected by ECM
    'E': Emitting ECM bubble
    'f': In the process of standing up
    'F': Fallen
    'h': In the process of going hull-down
    'H': Hull-down
    'I': Jellied by Inferno
    'J': Jumping
    'l': Illuminated
    'L': Illuminating
    'n': Enemy NARC attached (to friendly unit)
    'N': Friendly NARC attached (to enemy unit)
    '+': Overheating
    'O': Orbital-Dropping
    'p': Protected by ECCM
    'P': Emitting ECCM bubble
    's': Starting up
    'S': Shutdown
    't': Being towed
    'T': Towing
    'W': Swarming (bsuit)
    'X': Spinning (Aero)
      Position in the string is not significant and should not be relied
      upon. Any characters not in this list should be ignored, for forward
      compatibility.

  unit type character: Exactly one of the following strings:
    'B': Bipedal BattleMech ('Mech)
    'Q': Quad BattleMech (Quad)
    'H': Hover Vehicle (Vehicle)
    'T': Tracked Vehicle (Vehicle)
    'W': Wheeled Vehicle (Vehicle)
    'N': Naval Surface Displacement Vehicle (Vehicle)
    'Y': Naval Hydrofoil Vehicle (Vehicle)
    'U': Naval Submarine Vehicle (Vehicle)
    'V': VTOL
    'F': AeroFighter
    'A': Aerodyne Dropship
    'D': Spheroid Dropship
    'S': BattleSuit
    'I': Infantry
    'i': Installation (non moving unit)
    '-': Unknown
      Any other characters should be considered '-', for forward
      compatibility.

  weapon fire mode: '-', or at least one of the following characters:
    '2': RAC 2-shot mode
    '4': RAC 4-shot mode
    '6': RAC 6-shot mode
    'A': AMS (on)
    'B': Backpack mounted
    'C': Damaged: Crystal damaged
    'F': Damaged: Focus misaligned
    'G': Gattling MG mode
    'H': Hotloaded
    'I': iNARC Beacon launcher
    'L': Laser AMS (on)
    'M': Modular: part of Omni pod
    'N': NARC Beacon launcher
    'O': One-shot (unused)
    'R': RapidFire mode
    'S': Streak mode (usually not toggleable)
    'T': Linked to Targetting Computer
    'U': Ultra mode
    'a': AMS (off)
    'f': Damaged: Ammo feed damaged
    'b': Damaged: Barrel damaged
    'd': Damaged: Moderate damage
    'h': Heat mode
    'i': IDF capable
    'l': Laser AMS (off)
    'm': Modular: hand-held
    'o': One-shot (used)
    'r': Damaged: Ranging system hit

  weapon quality: integer, from 1 to 5, where 1 means low and 5 means
                  excellent.

  weapon scan status: One of the following:
    '-': Recycling/unusable
    '*': Destroyed/unusable
    'R': Ready

  weapon status: Either a non-zero integer for recycle time left, or one of
                 the following characters:
    '*': Destroyed
    'A': Ammo jammed
    'D': Disabled
    'E': Empty
    'J': Jammed
    'R': Weapon is ready
    'S': Shorted
    'a': Ammo crit-jammed
    'd': Dud
    'j': Jettisoned
      Characters not in this list should be considered 'D', for forward
      compatibility.

  weapon type: integer, identifier for type of weapon; index in the list of
               weapons retrievable by the 'wl' subcommand.

