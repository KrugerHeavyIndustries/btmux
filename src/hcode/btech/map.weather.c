
/*
 * $Id: map.weather.c,v 1.1.1.1 2005/01/11 21:18:10 kstevens Exp $
 *
 * Author: Cord Awtry <kipsta@bs-interactive.com>
 *
 *  Copyright (c) 2002 Cord Awtry
 *       All rights reserved
 */

#include "mech.h"
#include "p.mech.utils.h"
#include "p.mech.ice.h"
#include "p.map.build.functions.h"

int validateWeatherConditions(int curConditions)
{
	int conditions = curConditions;

	if(conditions & WEATHER_PRECIP) {
		if(conditions & WEATHER_HEAVY_PRECIP)
			conditions &= ~WEATHER_PRECIP;
	}

	return conditions;
}

int calcWeatherEffects(MAP * map)
{
	int effects = 0;
	int weather = map->weather;
	char temp = map->temp;
	short windspeed = map->windspeed;

	if(windspeed > 60)
		effects = EFFECT_HIGH_WINDS;
	else if(windspeed >= 30)
		effects = EFFECT_WINDS;

	if(weather & WEATHER_HEAVY_PRECIP) {
		if(temp <= 0) {
			if(effects & EFFECT_HIGH_WINDS) {
				effects &= ~EFFECT_HIGH_WINDS;
				effects |= EFFECT_BLIZZARD;
			} else
				effects |= EFFECT_HEAVY_SNOW;
		} else
			effects |= EFFECT_HEAVY_RAIN;
	} else if(weather & WEATHER_PRECIP) {
		if(temp <= 0)
			effects |= EFFECT_SNOW;
		else
			effects |= EFFECT_RAIN;
	}

	if(weather & WEATHER_FOG)
		effects |= EFFECT_FOG;

	if(weather & WEATHER_BLOWING_SAND)
		effects |= EFFECT_BLOWING_SAND;

	return effects;
}

int calcWeatherGunEffects(MAP * map, int weapindx)
{
	int gunMods = 0;
	int weapType = MechWeapons[weapindx].type;

	if(MapEffectRain(map)) {
		gunMods += 1;
	} else if(MapEffectRain(map)) {
		gunMods += 1;
	}

	if(MapEffectBlizzard(map)) {
		gunMods += ((weapType == TAMMO) ? 2 : 1);
	} else if(MapEffectHvySnow(map)) {
		gunMods += 1;
	}

	if(MapEffectFog(map)) {
		gunMods += ((weapType == TBEAM) ? 1 : 0);
	}

	if(MapEffectSand(map)) {
		gunMods += ((weapType == TAMMO) ? 1 : 2);
	}

	if(MapEffectHighWinds(map) & !MapEffectBlizzard(map)) {
		gunMods += ((weapType == TAMMO) ? 2 : 0);
	}

	if(MapEffectWinds(map)) {
		gunMods += ((weapType == TAMMO) ? 1 : 0);
	}

	return gunMods;
}

int calcWeatherPilotEffects(MECH * mech)
{
	MAP *map = FindObjectsData(mech->mapindex);
	int onTheGround = 1;
	int mod = 0;

	if(!map)
		return 0;

	onTheGround = (!Jumping(mech) &&
				   (MechZ(mech) <= Elevation(map, MechX(mech), MechY(mech))));

	if(onTheGround) {
		if((MapEffectHvyRain(map) || MapEffectHvySnow(map) ||
			MapEffectHighWinds(map) || MapEffectBlizzard(map)))
			mod += 1;

		if(MechMove(mech) != MOVE_HOVER) {
			if(HexHasDeepSnow(map, MechX(mech), MechY(mech)))
				mod += 1;

			if(HexHasMud(map, MechX(mech), MechY(mech)))
				mod += 1;

			if(HexHasRapids(map, MechX(mech), MechY(mech)))
				mod += 2;
		}

		return mod;
	}

	return 0;
}

void setWeatherHeatEffects(MAP * map, MECH * mech)
{
	if(MapEffectHvyRain(map))
		MechMinusHeat(mech) -= 2.;
	else if(MapEffectRain(map))
		MechMinusHeat(mech) -= 1.;

	if(MapEffectBlizzard(map))
		MechMinusHeat(mech) -= 2.;
	else if(MapEffectHvySnow(map))
		MechMinusHeat(mech) -= 1.;

	if(MapTemperature(map) < -30 || MapTemperature(map) > 50) {
		if(MapTemperature(map) < -30)
			MechMinusHeat(mech) += (-30 - MapTemperature(map) + 9) / 10;
		else
			MechMinusHeat(mech) -= (MapTemperature(map) - 50 + 9) / 10;
	}

	if(HexHasDeepSnow(map, MechX(mech), MechY(mech))) {
		if(FindLegHeatSinks(mech) > 0)
			MechMinusHeat(mech) -= 1.;
	}
}

void meltSnowAndIce(MAP * map, int x, int y, int depth, int emit,
					int makeSteam)
{
	int data = 0;
	int layers = 0, oldLayers = 0;
	int snowDone = 0;
	int steamLength = 0;

	if(!map)
		return;

	if(depth <= 0)
		return;

	oldLayers = GetHexLayers(map, x, y);
	layers = oldLayers;
	data = GetHexLayerData(map, x, y);

	if((layers & HEXLAYER_SNOW) || (layers & HEXLAYER_DEEP_SNOW)) {
		data = MAX(0, data - depth);
		steamLength = abs(data - GetHexLayerData(map, x, y));

		if(data == 0)
			layers &= ~(HEXLAYER_SNOW | HEXLAYER_DEEP_SNOW);
		else if(data <= 1000) {
			layers |= HEXLAYER_SNOW;
			layers &= ~HEXLAYER_DEEP_SNOW;
		} else {
			layers |= HEXLAYER_DEEP_SNOW;
			layers &= ~HEXLAYER_SNOW;
		}

		SetHexLayers(map, x, y, layers);
		SetHexLayerData(map, x, y, data);

		if(emit) {
			if(layers & HEXLAYER_DEEP_SNOW)
				snowDone = 1;

			if(!snowDone && !((layers & HEXLAYER_SNOW) ||
							  (layers & HEXLAYER_DEEP_SNOW))) {
				HexLOSBroadcast(map, x, y,
								"%ch%cgThe snow in $h melts to nothingness!%cn");
				snowDone = 1;
			}

			if(!snowDone && ((oldLayers & HEXLAYER_DEEP_SNOW) &&
							 (!(layers & HEXLAYER_DEEP_SNOW)))) {
				HexLOSBroadcast(map, x, y,
								"%ch%cgThe snow in $h visibly melts!%cn");
				snowDone = 1;
			}
		}
	}

	if(IsIceHex(map, x, y)) {
		if(depth >= (Elevation(map, x, y) * 200)) {
			if(emit)
				HexLOSBroadcast(map, x, y, "The ice at $h breaks apart!");

			breakIceAndSplashUnits(map, NULL, x, y,
								   "goes swimming as ice breaks!");
			steamLength = (Elevation(map, x, y) * 200);
		}
	}

	if((steamLength > 0) && makeSteam) {
		if(steamLength > 90)
			steamLength = 90 + Number(0, steamLength / 20);

		add_decoration(map, x, y, TYPE_SMOKE, SMOKE, steamLength);
	}
}

void growSnow(MAP * map, int lowDepth, int highDepth)
{
	int i, j;
	char terrain;
	int layer, layerData;
	int depth = 0;
	int sign = 1;
	int low, high;

	if((lowDepth == 0) && (highDepth == 0))
		return;

	low = MIN(abs(lowDepth), abs(highDepth));
	high = MAX(abs(lowDepth), abs(highDepth));

	if((lowDepth < 0) || (highDepth < 0))
		sign = -1;

	for(i = 0; i < map->width; i++) {
		for(j = 0; j < map->height; j++) {
			terrain = GetHexTerrain(map, i, j);

			switch (terrain) {
			case BRIDGE:
			case FIRE:
			case WATER:
			case ICE:
				continue;
				break;
			}

			depth = Number(low, high) * sign;

			if(depth == 0)
				continue;

			layerData = GetHexLayerData(map, i, j);

			if(depth < 0) {
				if(!(HexHasSnow(map, i, j) || HexHasDeepSnow(map, i, j)))
					continue;
			} else {
				if(!(HexHasSnow(map, i, j) && HexHasDeepSnow(map, i, j)))
					SetHexLayers(map, i, j, HEXLAYER_SNOW);
			}

			SetHexLayerData(map, i, j, (layerData + depth));
			validateSnowDepth(map, i, j);
		}
	}

}
