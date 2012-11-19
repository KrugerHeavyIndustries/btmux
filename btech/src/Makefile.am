AM_CPPFLAGS = $(BTPR_INCLUDE) -I$(srcdir)/include -I$(srcdir)/btech \
	-I$(top_srcdir)/btfi/include

if BTPR_MUX1
BTPR_INCLUDE = -I$(top_srcdir)/../include -I$(top_srcdir)/../src
endif

if BTPR_PENN
AM_LDFLAGS = -Wl,-Bsymbolic
BTPR_INCLUDE = -I$(btpr_dir)/penn/include -I$(btpr_dir)/include
BTPR_LIBS = ../btpr/libbtpr.la
endif

if BTPR_MUX2
AM_LDFLAGS = -Wl,-Bsymbolic
BTPR_INCLUDE = -I$(btpr_dir)/mux2/include -I$(btpr_dir)/include
BTPR_LIBS = ../btpr/libbtpr.la
endif

lib_LTLIBRARIES = libbtech.la

libbtech_la_SOURCES = coolmenu.c debug.c events.c glue.hcode.c glue.c \
	glue.scode.c btdb.c btdb.h btdb_int.h xcode_io.c xcode_io.h \
	mech.lostracer.c mechfile.c spath.c tick.c mt19937ar.c \
	btech/aero.bomb.c btech/aero.move.c btech/artillery.c \
	btech/autopilot_ai.c btech/autopilot_autogun.c \
	btech/autopilot_commands.c btech/autopilot_core.c \
	btech/autopilot_radio.c btech/bsuit.c btech/btech.c \
	btech/btechstats.c btech/btspath.c btech/crit.c btech/ds.bay.c \
	btech/ds.turret.c btech/econ.c btech/econ_cmds.c btech/eject.c \
	btech/failures.c btech/hudinfo.c btech/map.bits.c btech/map.coding.c \
	btech/map.conditions.c btech/map.dynamic.c \
	btech/map.los.c btech/map.c btech/map.obj.c btech/mech.advanced.c \
	btech/mech.ammodump.c btech/mech.bth.c \
	btech/mech.build.c btech/mech.c3.misc.c btech/mech.c3.c \
	btech/mech.c3i.c btech/mech.combat.misc.c btech/mech.combat.missile.c \
	btech/mech.combat.c btech/mech.consistency.c btech/mech.contacts.c \
	btech/mech.damage.c btech/mech.ecm.c btech/mech.enhanced.criticals.c \
	btech/mech.events.c btech/mech.fire.c btech/mech.hitloc.c \
	btech/mech.ice.c btech/mech.lite.c btech/mech.los.c \
	btech/mech.maps.c btech/mech.mechref_ident.c btech/mech.move.c \
	btech/mech.notify.c btech/mech.ood.c btech/mech.partnames.c \
	btech/mech.physical.c btech/mech.pickup.c btech/mech.restrict.c \
	btech/mech.scan.c btech/mech.sensor.functions.c \
	btech/mech.sensor.c btech/mech.spot.c btech/mech.startup.c \
	btech/mech.stat.c btech/mech.status.c btech/mech.tag.c \
	btech/mech.tech.commands.c btech/mech.tech.damages.c \
	btech/mech.tech.do.c btech/mech.tech.events.c \
	btech/mech.tech.c btech/mech.tech.repairs.c \
	btech/mech.tech.saverepair.c btech/mech.tic.c btech/mech.update.c \
	btech/mech.utils.c btech/mechrep.c btech/mine.c btech/pcombat.c \
	btech/template.c \
	btech/aero.bomb.h btech/artillery.h btech/autopilot.h \
	btech/btechstats_global.h btech/btechstats.h btech/btmacros.h \
	btech/extern.h btech/failures.h btech/floatsim.h btech/hudinfo.h \
	btech/map.coding.h btech/map.h btech/map.los.h \
	btech/mech.combat.h btech/mech.ecm.h btech/mech.events.h btech/mech.h \
	btech/mech.ice.h btech/mech.notify.h btech/mech.partnames.h \
	btech/mech.physical.h btech/mechrep.h btech/mech.sensor.h \
	btech/mech.stat.h btech/mech.tech.damages.h btech/mech.tech.h \
	btech/mech.tic.h btech/mine.h btech/mymath.h btech/p.aero.bomb.h \
	btech/p.aero.move.h btech/p.ai.h btech/p.artillery.h \
	btech/p.autogun.h btech/p.autopilot_command.h \
	btech/p.autopilot_commands.h btech/p.autopilot.h btech/p.bsuit.h \
	btech/p.btech.h btech/p.btechstats.h btech/p.btspath.h \
	btech/pcombat.h btech/p.crit.h btech/p.ds.bay.h btech/p.ds.turret.h \
	btech/p.econ_cmds.h btech/p.econ.h btech/p.eject.h btech/p.failures.h \
	btech/p.map.bits.h btech/p.map.build.functions.h btech/p.map.coding.h \
	btech/p.map.conditions.h btech/p.map.dynamic.h btech/p.map.h \
	btech/p.map.obj.h btech/p.map.weather.h btech/p.mech.advanced.h \
	btech/p.mech.ammodump.h btech/p.mech.bth.h \
	btech/p.mech.build.h btech/p.mech.c3.h btech/p.mech.c3i.h \
	btech/p.mech.c3.misc.h btech/p.mech.combat.h \
	btech/p.mech.combat.misc.h btech/p.mech.combat.missile.h \
	btech/p.mech.consistency.h btech/p.mech.contacts.h \
	btech/p.mech.damage.h btech/p.mech.ecm.h \
	btech/p.mech.enhanced.criticals.h btech/p.mech.events.h \
	btech/p.mech.fire.h btech/p.mech.hitloc.h btech/p.mech.ice.h \
	btech/p.mech.lite.h btech/p.mech.los.h btech/p.mech.maps.h \
	btech/p.mech.mechref_ident.h btech/p.mech.move.h \
	btech/p.mech.notify.h btech/p.mech.ood.h btech/p.mech.partnames.h \
	btech/p.mech.physical.h btech/p.mech.pickup.h btech/p.mechrep.h \
	btech/p.mech.restrict.h btech/p.mech.scan.h \
	btech/p.mech.sensor.functions.h btech/p.mech.sensor.h \
	btech/p.mech.spot.h btech/p.mech.startup.h btech/p.mech.stat.h \
	btech/p.mech.status.h btech/p.mech.tag.h btech/p.mech.tic.h \
	btech/p.mech.tech.commands.h btech/p.mech.tech.damages.h \
	btech/p.mech.tech.do.h btech/p.mech.tech.events.h btech/p.mech.tech.h \
	btech/p.mech.tech.repairs.h btech/p.mech.tech.saverepair.h \
	btech/p.mech.update.h btech/p.mech.utils.h btech/p.mine.h \
	btech/p.pcombat.h btech/p.template.h btech/turret.h \
	btech/weapons.fasa.h btech/weapons.h btech/weapons.vrt.h

libbtech_la_LIBADD = $(BTPR_LIBS) ../btfi/libbtfi.la

noinst_HEADERS = include/btconfig.h include/coolmenu.h \
	include/coolmenu_interface2.h include/coolmenu_interface.h \
	include/debug.h include/glue.h include/glue_types.h include/macros.h \
	include/mycool.h include/p.coolmenu.h include/p.debug.h \
	include/p.events.h include/p.glue.h include/p.glue.hcode.h \
	include/p.glue.scode.h include/p.luck.h include/p.mechfile.h \
	include/p.mech.lostracer.h include/p.spath.h include/spath.h \
	include/mt19937ar.h
