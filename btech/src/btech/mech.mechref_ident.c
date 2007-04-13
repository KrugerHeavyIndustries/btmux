
/*
 * $Id: mech.mechref_ident.c,v 1.1.1.1 2005/01/11 21:18:18 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Tue Sep 17 18:08:13 1996 fingon
 * Last modified: Sun Jan 12 13:30:10 1997 fingon
 *
 */

#include <string.h>

static struct {
	char *mechref_start;
	char *name;
} my_mechref_pile[] = {
	{
	"ALB-", "Albatross"}, {
	"ALM-", "Fireball"}, {
	"ANH-", "Annihilator"}, {
	"ANV-", "Anvil"}, {
	"APL-", "Apollo"}, {
	"ARC-", "Archer"}, {
	"AS7-", "Atlas"}, {
	"ASN-", "Assassin"}, {
	"AWS-", "Awesome"}, {
	"BCK-", NULL}, {
	"BEN", NULL}, {
	"BEN1-", NULL}, {
	"BH-", "Battle Hawk"}, {
	"BJ-", "Blackjack"}, {
	"BL6-", "Black Knight"}, {
	"BLR-", "Battlemaster"}, {
	"BMB-", "Bombardier"}, {
	"BNC-", "Banshee"}, {
	"BNDR-", "Bandersnatch"}, {
	"BW-", NULL}, {
	"BZK-", "Hollander"}, {
	"Bab-", NULL}, {
	"Beh-", NULL}, {
	"Behemoth", "Behemoth"}, {
	"Bla-", "BlackHawk"}, {
	"BlackHawk", "BlackHawk"}, {
	"Bulldog", "Bulldog"}, {
	"CDA-", "Cicada"}, {
	"CES-", "Caesar"}, {
	"CGR-", "Charger"}, {
	"CHP-", "Champion"}, {
	"CLNT-", "Clint"}, {
	"CN9-", "Centurion"}, {
	"COM-", "Commando"}, {
	"CP10-", "Cyclops"}, {
	"CP11-", "Cyclops"}, {
	"CPLT-", "Catapult"}, {
	"CRB-", "Crab"},
		/* Note: CRK5003-1 = Crockett, 2 = Katana */
	{
	"CRD-", "Crusader"}, {
	"CRK5003-1", "Crockett"}, {
	"CRK_5003-2", "Katana"}, {
	"CTF-", "Cataphract"}, {
	"Cyrano", "Cyrano"}, {
	"DAI-", "Daikyu"}, {
	"DMO-", "Daimyo"}, {
	"DRG-", "Dragon"}, {
	"DV-", "Dervish"}, {
	"Dai-", "Daishi"}, {
	"Daishi", "Daishi"}, {
	"Das-", "Dasher"}, {
	"Dasher", "Dasher"}, {
	"Demolisher", "Demolisher"}, {
	"Dra-", "Dragonfly"}, {
	"Dragonfly", "Dragonfly"}, {
	"Drillson", "Drillson"}, {
	"ENF-", "Enforcer"}, {
	"ETAM-", NULL}, {
	"ETHS-", NULL}, {
	"ETLS-", NULL}, {
	"ETMC-", NULL}, {
	"ETRV-", NULL}, {
	"EXT-", "Exterminator"}, {
	"FF", NULL}, {
	"FFL-", "Firefly"}, {
	"FLC-", "Falcon"}, {
	"FLE-", "Flea"}, {
	"FLS-", "Flashman"}, {
	"FS9-", "Firestarter"}, {
	"Fen-", "Fenris"}, {
	"Fenris", "Fenris"}, {
	"Ferret", "Ferret"}, {
	"GAL-", "Gallowglass"}, {
	"GHR-", "Grasshopper"}, {
	"GLT-", "Guillotine"}, {
	"GOL-", "Goliath"}, {
	"GRF-", "Griffin"}, {
	"GRM-R-", "Grim Reaper"}, {
	"GT-", NULL}, {
	"GUN-", "Gunslinger"}, {
	"Gal-", NULL}, {
	"Gla-", "Gladiator"}, {
	"Gladiator", "Gladiator"}, {
	"H-", "H-7"}, {
	"HBK-", "Hunchback"}, {
	"HCT-", "Hatchetman"}, {
	"HER-", "Hermes"}, {
	"HGN-", "Highlander"}, {
	"HM-", "Hitman"}, {
	"HMR-", "Hammer"}, {
	"HNT-", "Hornet"}, {
	"HOP-", "Hoplite"}, {
	"HRC-LS-", "Hercules"}, {
	"HSR-", "Hussar"}, {
	"HSR_200-", "Hussar"}, {
	"HTM-", "Hata-Chi"}, {
	"HUR-W0-", "Huron"}, {
	"Hel-", NULL}, {
	"IMP-", "Imp"}, {
	"IMPOS-", NULL}, {
	"JA-KL-", "Jackal"}, {
	"JEdgar", "JEdgar"}, {
	"JM6-", "Jagermech"}, {
	"JR7-", "Jenner"}, {
	"JVN-", "Javelin"}, {
	"KGC-", "King Crab"}, {
	"KIM-", "Komodo"}, {
	"KOH-", NULL}, {
	"KT0-", NULL}, {
	"KTO-", "Kintaro"}, {
	"Kos-", NULL}, {
	"Kra-", "Kraken"}, {
	"LCT-", "Locust"}, {
	"LGB-OW", "Longbow"}, {
	"LNC_25-", "Lancelot"}, {
	"Lok-", "Loki"}, {
	"MAD-", "Marauder"}, {
	"MAL-", "Mauler"}, {
	"MCY-", "Mercury"}, {
	"MDG-", "Rakshasa"}, {
	"MNT-A-RY-", NULL}, {
	"MON-", "Mongoose"}, {
	"MR-", "Morpheus"}, {
	"Mad-", "MadCat"}, {
	"MadCat", "MadCat"}, {
	"Man-", "ManO'War"}, {
	"ManO'War", "ManO'War"}, {
	"Manticore", "Manticore"}, {
	"Marksman", "Marksman"}, {
	"Mas-", "Masakari"}, {
	"Masakari", "Masakari"}, {
	"Mqan-", NULL}, {
	"NG-", "Naginata"}, {
	"NGS-", "Nightsky"}, {
	"NXS-", NULL}, {
	"ON1-", "Orion"}, {
	"OSR-", "Ostroc"}, {
	"OTL-", "Ostsol"}, {
	"OTT-", "Ostscout"}, {
	"Ontos", "Ontos"}, {
	"PIT-", NULL}, {
	"PKR-", NULL}, {
	"PNT-", "Panther"}, {
	"PPR-", "Salamander"}, {
	"PTR-", "Penetrator"}, {
	"PXH-", "Phoenix Hawk"}, {
	"Partisan", "Partisan"}, {
	"Pegasus", "Pegasus"}, {
	"Per-", NULL}, {
	"Pou-", NULL}, {
	"Pum-", "Puma"}, {
	"Puma", "Puma"}, {
	"QKD-", "QuickDraw"}, {
	"RFL-", "Rifleman"}, {
	"RJN_101-", "Raijin"}, {
	"RVN-", "Raven"}, {
	"Ripper", "Ripper"}, {
	"Ryo-", "Ryoken"}, {
	"Ryoken", "Ryoken"}, {
	"SCB-", "Scarabus"}, {
	"SCP-", "Scorpion"}, {
	"SDR-", "Spider"}, {
	"SHD-", "Shadow Hawk"}, {
	"SHG-", "Shogun"}, {
	"SNK-", "Snake"}, {
	"SPT-", "Spartan"}, {
	"STG-", "Stinger"}, {
	"STH-", "Stealth"}, {
	"STK-", "Stalker"}, {
	"STN-", "Sentinel"}, {
	"Saladin", "Saladin"}, {
	"Schrek", "Schrek"}, {
	"Sniper", NULL}, {
	"Sturmfeur", "Sturmfeur"}, {
	"T-IT-", "Grand Titan"}, {
	"TBT-", "Trebuchet"}, {
	"TDR-", "Thunderbolt"}, {
	"THE-", "Thorn"}, {
	"THG-", "Thug"}, {
	"THR-", "Thunder"}, {
	"TMP-", "Tempest"}, {
	"TR1", "Wraith"}, {
	"Tho-", "Thor"}, {
	"Thor", "Thor-"}, {
	"UM-", "Urbanmech"}, {
	"Ull-", "Uller"}, {
	"Uller", "Uller"}, {
	"VL-", "Vulcan"}, {
	"VLK-", "Valkyrie"}, {
	"VND-", "Vindicator"}, {
	"VNL-", NULL}, {
	"VSD-", NULL}, {
	"VT-", "Vulcan"}, {
	"VTR-", "Victor"}, {
	"Vix-", NULL}, {
	"Vul-", "Vulture"}, {
	"Vulture", "Vulture"}, {
	"WFT-", "Wolf Trap"}, {
	"WHM-", "Warhammer"}, {
	"WLF-", "Wolfhound"}, {
	"WR-DG-", "War Dog"}, {
	"WSP-", "Wasp"}, {
	"WTC-", "Watchman"}, {
	"WTH-", "Whitworth"}, {
	"WVE-", "Wyvern"}, {
	"WVR-", "Wolverine"}, {
	"ZEU-", "Zeus"}, {
	"ZPH-", "Tarantula"},
		/* Aeros */
	{
	"CHP-W", "Chippewa"}, {
	"CNT-", "Centurion"}, {
	"CSR-V", "Corsair"}, {
	"EAG-", "Eagle"}, {
	"F-100", "Riever"}, {
	"F-10", "Cheetah"}, {
	"F-500", "Riever"}, {
	"F-90", "Stingray"}, {
	"HCT-", "Hellcat"}, {
	"LCF-", "Lucifer"}, {
	"LTG-", "Lightning"}, {
	"MechBuster", "MechBuster"}, {
	"SBR-", "Sabre"}, {
	"SL-15", "Slayer"}, {
	"SL-17", "Shilone"}, {
	"SL-21", "Sholagar"}, {
	"SPR-", "Sparrowhawk"}, {
	"STU-", "Stuka"}, {
	"SYD-", "Seydlitz"}, {
	"TR-10", "Transit"}, {
	"TR-13", "Transgressor"}, {
	"TR-14", "Transgressor"}, {
	"TR-7", "Thrush"}, {
	"TRB-1", "Thunderbird"}, {
	NULL, NULL}
};

const char *find_mechname_by_mechref(const char *ref)
{
	int i;

	for(i = 0; my_mechref_pile[i].mechref_start; i++)
		if(my_mechref_pile[i].name)
			if(!strncmp(my_mechref_pile[i].mechref_start, ref,
						strlen(my_mechref_pile[i].mechref_start)))
				return my_mechref_pile[i].name;
	return NULL;
}
