
/*
   p.btechstats.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Mon Mar 22 10:40:18 CET 1999 from btechstats.c */

#include "config.h"

#ifndef _P_BTECHSTATS_H
#define _P_BTECHSTATS_H

/* btechstats.c */
char *silly_get_uptime_to_string(int i);
void list_charvaluestuff(dbref player, int flag);
int char_getvaluecode(char *name);
int char_rollsaving(void);
int char_rollunskilled(void);
int char_rollskilled(void);
int char_rolld6(int num);
int char_getvalue(dbref player, char *name);
void char_setvalue(dbref player, char *name, int value);
int char_getskilltargetbycode(dbref player, int code, int modifier);
int char_getskilltarget(dbref player, char *name, int modifier);
int char_getxpbycode(dbref player, int code);
int char_gainxpbycode(dbref player, int code, int amount);
int char_gainxp(dbref player, char *skill, int amount);
int char_getskillsuccess(dbref player, char *name, int modifier, int loud);
int char_getskillmargsucc(dbref player, char *name, int modifier);
int char_getopposedskill(dbref first, char *skill1, dbref second,
    char *skill2);
int char_getattrsave(dbref player, char *name);
int char_getattrsavesucc(dbref player, char *name);
void zap_unneccessary_stats(void);
void init_btechstats(void);
void do_charstatus(dbref player, dbref cause, int key, char *arg1);
void do_charclear(dbref player, dbref cause, int key, char *arg1);
dbref char_lookupplayer(dbref player, dbref cause, int key, char *arg1);
void initialize_pc(dbref player, MECH * mech);
void fix_pilotdamage(MECH * mech, dbref player);
int mw_ic_bth(MECH * mech);
int handlemwconc(MECH * mech, int initial);
void headhitmwdamage(MECH * mech, int dam);
void mwlethaldam(MECH * mech, int dam);
void lower_xp(dbref player, int promillage);
void AccumulateTechXP(dbref pilot, MECH * mech, int reason);
void AccumulateTechWeaponsXP(dbref pilot, MECH * mech, int reason);
void AccumulateCommXP(dbref pilot, MECH * mech);
void AccumulatePilXP(dbref pilot, MECH * mech, int reason, int addanyway);
void AccumulateSpotXP(dbref pilot, MECH * attacker, MECH * wounded);
int MadePerceptionRoll(MECH * mech, int modifier);
void AccumulateArtyXP(dbref pilot, MECH * attacker, MECH * wounded);
void AccumulateComputerXP(dbref pilot, MECH * mech, int reason);
int HasBoolAdvantage(dbref player, const char *name);
void AccumulateGunXP(dbref pilot, MECH * attacker, MECH * wounded,
    int numOccurences, int multiplier, int weapindx, int bth);
void AccumulateGunXPold(dbref pilot, MECH * attacker, MECH * wounded,
    int numOccurences, int multiplier, int weapindx, int bth);
void fun_btgetcharvalue(char *buff, char **bufc, dbref player, dbref cause,
    char *fargs[], int nfargs, char *cargs[], int ncargs);
void fun_btsetcharvalue(char *buff, char **bufc, dbref player, dbref cause,
    char *fargs[], int nfargs, char *cargs[], int ncargs);
void fun_btcharlist(char *buff, char **bufc, dbref player, dbref cause,
    char *fargs[], int nfargs, char *cargs[], int ncargs);
void debug_xptop(dbref player, void *data, char *buffer);
void debug_setxplevel(dbref player, void *data, char *buffer);
int btthreshold_func(char *skillname);
struct chargen_struct *retrieve_chargen_struct(dbref player);
int lowest_bit(int num);
int recursive_add(int lev);
int can_proceed(dbref player, struct chargen_struct *st);
void cm_a_add(dbref player, void *data, char *buffer);
void cm_a_minus(dbref player, void *data, char *buffer);
void cm_a_toggle(dbref player, void *data, char *buffer);
void cm_a_set(dbref player, void *data, char *buffer);
void cm_b_add(dbref player, void *data, char *buffer);
void cm_b_minus(dbref player, void *data, char *buffer);
void cm_b_toggle(dbref player, void *data, char *buffer);
void cm_b_set(dbref player, void *data, char *buffer);
void cm_c_add(dbref player, void *data, char *buffer);
void cm_c_minus(dbref player, void *data, char *buffer);
void cm_c_toggle(dbref player, void *data, char *buffer);
void cm_c_set(dbref player, void *data, char *buffer);
void cm_d_add(dbref player, void *data, char *buffer);
void cm_d_minus(dbref player, void *data, char *buffer);
void cm_d_toggle(dbref player, void *data, char *buffer);
void cm_d_set(dbref player, void *data, char *buffer);
void cm_e_add(dbref player, void *data, char *buffer);
void cm_e_minus(dbref player, void *data, char *buffer);
void cm_e_toggle(dbref player, void *data, char *buffer);
void cm_e_set(dbref player, void *data, char *buffer);
void cm_f_add(dbref player, void *data, char *buffer);
void cm_f_minus(dbref player, void *data, char *buffer);
void cm_f_toggle(dbref player, void *data, char *buffer);
void cm_f_set(dbref player, void *data, char *buffer);
void cm_g_add(dbref player, void *data, char *buffer);
void cm_g_minus(dbref player, void *data, char *buffer);
void cm_g_toggle(dbref player, void *data, char *buffer);
void cm_g_set(dbref player, void *data, char *buffer);
void cm_h_add(dbref player, void *data, char *buffer);
void cm_h_minus(dbref player, void *data, char *buffer);
void cm_h_toggle(dbref player, void *data, char *buffer);
void cm_h_set(dbref player, void *data, char *buffer);
void cm_i_add(dbref player, void *data, char *buffer);
void cm_i_minus(dbref player, void *data, char *buffer);
void cm_i_toggle(dbref player, void *data, char *buffer);
void cm_i_set(dbref player, void *data, char *buffer);
void cm_j_add(dbref player, void *data, char *buffer);
void cm_j_minus(dbref player, void *data, char *buffer);
void cm_j_toggle(dbref player, void *data, char *buffer);
void cm_j_set(dbref player, void *data, char *buffer);
void cm_k_add(dbref player, void *data, char *buffer);
void cm_k_minus(dbref player, void *data, char *buffer);
void cm_k_toggle(dbref player, void *data, char *buffer);
void cm_k_set(dbref player, void *data, char *buffer);
void cm_l_add(dbref player, void *data, char *buffer);
void cm_l_minus(dbref player, void *data, char *buffer);
void cm_l_toggle(dbref player, void *data, char *buffer);
void cm_l_set(dbref player, void *data, char *buffer);
void cm_m_add(dbref player, void *data, char *buffer);
void cm_m_minus(dbref player, void *data, char *buffer);
void cm_m_toggle(dbref player, void *data, char *buffer);
void cm_m_set(dbref player, void *data, char *buffer);
void cm_n_add(dbref player, void *data, char *buffer);
void cm_n_minus(dbref player, void *data, char *buffer);
void cm_n_toggle(dbref player, void *data, char *buffer);
void cm_n_set(dbref player, void *data, char *buffer);
void cm_o_add(dbref player, void *data, char *buffer);
void cm_o_minus(dbref player, void *data, char *buffer);
void cm_o_toggle(dbref player, void *data, char *buffer);
void cm_o_set(dbref player, void *data, char *buffer);
void cm_p_add(dbref player, void *data, char *buffer);
void cm_p_minus(dbref player, void *data, char *buffer);
void cm_p_toggle(dbref player, void *data, char *buffer);
void cm_p_set(dbref player, void *data, char *buffer);
void cm_q_add(dbref player, void *data, char *buffer);
void cm_q_minus(dbref player, void *data, char *buffer);
void cm_q_toggle(dbref player, void *data, char *buffer);
void cm_q_set(dbref player, void *data, char *buffer);
void cm_r_add(dbref player, void *data, char *buffer);
void cm_r_minus(dbref player, void *data, char *buffer);
void cm_r_toggle(dbref player, void *data, char *buffer);
void cm_r_set(dbref player, void *data, char *buffer);
void cm_s_add(dbref player, void *data, char *buffer);
void cm_s_minus(dbref player, void *data, char *buffer);
void cm_s_toggle(dbref player, void *data, char *buffer);
void cm_s_set(dbref player, void *data, char *buffer);
void cm_t_add(dbref player, void *data, char *buffer);
void cm_t_minus(dbref player, void *data, char *buffer);
void cm_t_toggle(dbref player, void *data, char *buffer);
void cm_t_set(dbref player, void *data, char *buffer);
void cm_u_add(dbref player, void *data, char *buffer);
void cm_u_minus(dbref player, void *data, char *buffer);
void cm_u_toggle(dbref player, void *data, char *buffer);
void cm_u_set(dbref player, void *data, char *buffer);
void cm_v_add(dbref player, void *data, char *buffer);
void cm_v_minus(dbref player, void *data, char *buffer);
void cm_v_toggle(dbref player, void *data, char *buffer);
void cm_v_set(dbref player, void *data, char *buffer);
void cm_w_add(dbref player, void *data, char *buffer);
void cm_w_minus(dbref player, void *data, char *buffer);
void cm_w_toggle(dbref player, void *data, char *buffer);
void cm_w_set(dbref player, void *data, char *buffer);
void cm_x_add(dbref player, void *data, char *buffer);
void cm_x_minus(dbref player, void *data, char *buffer);
void cm_x_toggle(dbref player, void *data, char *buffer);
void cm_x_set(dbref player, void *data, char *buffer);
void cm_y_add(dbref player, void *data, char *buffer);
void cm_y_minus(dbref player, void *data, char *buffer);
void cm_y_toggle(dbref player, void *data, char *buffer);
void cm_y_set(dbref player, void *data, char *buffer);
void cm_z_add(dbref player, void *data, char *buffer);
void cm_z_minus(dbref player, void *data, char *buffer);
void cm_z_toggle(dbref player, void *data, char *buffer);
void cm_z_set(dbref player, void *data, char *buffer);
int can_advance_state(struct chargen_struct *st);
int can_go_back_state(struct chargen_struct *st);
void recalculate_skillpoints(struct chargen_struct *st);
void go_back_state(dbref player, struct chargen_struct *st);
void chargen_look(dbref player, void *data, char *buffer);
void chargen_begin(dbref player, void *data, char *buffer);
void chargen_apply(dbref player, void *data, char *buffer);
void chargen_done(dbref player, void *data, char *buffer);
void chargen_next(dbref player, void *data, char *buffer);
void chargen_prev(dbref player, void *data, char *buffer);
void chargen_reset(dbref player, void *data, char *buffer);
void chargen_help(dbref player, void *data, char *buffer);

#endif				/* _P_BTECHSTATS_H */
