/* p.map.weather.h */

#ifndef _P_MAP_WEATHER_H
#define _P_MAP_WEATHER_H

/* map.weather.c */

int validateWeatherConditions(int curConditions);
int calcWeatherEffects(MAP * map);
int calcWeatherGunEffects(MAP * map, int weapindx);
int calcWeatherPilotEffects(MECH * mech);
void setWeatherHeatEffects(MAP * map, MECH * mech);
void meltSnowAndIce(MAP * map, int x, int y, int depth, int emit,
    int makeSteam);
void growSnow(MAP * map, int lowDepth, int highDepth);

#endif				/* _P_MAP_WEATHER_H */
