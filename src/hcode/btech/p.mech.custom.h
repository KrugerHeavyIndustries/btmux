
/*
   p.mech.custom.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:47 CET 1999 from mech.custom.c */

#ifndef _P_MECH_CUSTOM_H
#define _P_MECH_CUSTOM_H

/* mech.custom.c */
int crit_weight(MECH * mech, int t);
int generate_change_list(MECH * from, MECH * to);
void ToggleAmmo(dbref player, MECH * mech, int loc, int pos);
void custom_edit(dbref player, void *data, char *buffer);
void custom_finish(dbref player, void *data, char *buffer);
void custom_back(dbref player, void *data, char *buffer);
void custom_look(dbref player, void *data, char *buffer);
void custom_help(dbref player, void *data, char *buffer);
void custom_status(dbref player, void *data, char *buffer);
void custom_weight1(dbref player, void *data, char *buffer);
void custom_weight2(dbref player, void *data, char *buffer);
void custom_critstatus(dbref player, void *data, char *buffer);
void custom_weaponspecs(dbref player, void *data, char *buffer);
void newfreecustom(dbref key, void **data, int selector);
void cu_a_add(dbref player, void *data, char *buffer);
void cu_a_minus(dbref player, void *data, char *buffer);
void cu_a_toggle(dbref player, void *data, char *buffer);
void cu_a_set(dbref player, void *data, char *buffer);
void cu_b_add(dbref player, void *data, char *buffer);
void cu_b_minus(dbref player, void *data, char *buffer);
void cu_b_toggle(dbref player, void *data, char *buffer);
void cu_b_set(dbref player, void *data, char *buffer);
void cu_c_add(dbref player, void *data, char *buffer);
void cu_c_minus(dbref player, void *data, char *buffer);
void cu_c_toggle(dbref player, void *data, char *buffer);
void cu_c_set(dbref player, void *data, char *buffer);
void cu_d_add(dbref player, void *data, char *buffer);
void cu_d_minus(dbref player, void *data, char *buffer);
void cu_d_toggle(dbref player, void *data, char *buffer);
void cu_d_set(dbref player, void *data, char *buffer);
void cu_e_add(dbref player, void *data, char *buffer);
void cu_e_minus(dbref player, void *data, char *buffer);
void cu_e_toggle(dbref player, void *data, char *buffer);
void cu_e_set(dbref player, void *data, char *buffer);
void cu_f_add(dbref player, void *data, char *buffer);
void cu_f_minus(dbref player, void *data, char *buffer);
void cu_f_toggle(dbref player, void *data, char *buffer);
void cu_f_set(dbref player, void *data, char *buffer);
void cu_g_add(dbref player, void *data, char *buffer);
void cu_g_minus(dbref player, void *data, char *buffer);
void cu_g_toggle(dbref player, void *data, char *buffer);
void cu_g_set(dbref player, void *data, char *buffer);
void cu_h_add(dbref player, void *data, char *buffer);
void cu_h_minus(dbref player, void *data, char *buffer);
void cu_h_toggle(dbref player, void *data, char *buffer);
void cu_h_set(dbref player, void *data, char *buffer);
void cu_i_add(dbref player, void *data, char *buffer);
void cu_i_minus(dbref player, void *data, char *buffer);
void cu_i_toggle(dbref player, void *data, char *buffer);
void cu_i_set(dbref player, void *data, char *buffer);
void cu_j_add(dbref player, void *data, char *buffer);
void cu_j_minus(dbref player, void *data, char *buffer);
void cu_j_toggle(dbref player, void *data, char *buffer);
void cu_j_set(dbref player, void *data, char *buffer);
void cu_k_add(dbref player, void *data, char *buffer);
void cu_k_minus(dbref player, void *data, char *buffer);
void cu_k_toggle(dbref player, void *data, char *buffer);
void cu_k_set(dbref player, void *data, char *buffer);
void cu_l_add(dbref player, void *data, char *buffer);
void cu_l_minus(dbref player, void *data, char *buffer);
void cu_l_toggle(dbref player, void *data, char *buffer);
void cu_l_set(dbref player, void *data, char *buffer);
void cu_m_add(dbref player, void *data, char *buffer);
void cu_m_minus(dbref player, void *data, char *buffer);
void cu_m_toggle(dbref player, void *data, char *buffer);
void cu_m_set(dbref player, void *data, char *buffer);
void cu_n_add(dbref player, void *data, char *buffer);
void cu_n_minus(dbref player, void *data, char *buffer);
void cu_n_toggle(dbref player, void *data, char *buffer);
void cu_n_set(dbref player, void *data, char *buffer);
void cu_o_add(dbref player, void *data, char *buffer);
void cu_o_minus(dbref player, void *data, char *buffer);
void cu_o_toggle(dbref player, void *data, char *buffer);
void cu_o_set(dbref player, void *data, char *buffer);
void cu_p_add(dbref player, void *data, char *buffer);
void cu_p_minus(dbref player, void *data, char *buffer);
void cu_p_toggle(dbref player, void *data, char *buffer);
void cu_p_set(dbref player, void *data, char *buffer);
void cu_q_add(dbref player, void *data, char *buffer);
void cu_q_minus(dbref player, void *data, char *buffer);
void cu_q_toggle(dbref player, void *data, char *buffer);
void cu_q_set(dbref player, void *data, char *buffer);
void cu_r_add(dbref player, void *data, char *buffer);
void cu_r_minus(dbref player, void *data, char *buffer);
void cu_r_toggle(dbref player, void *data, char *buffer);
void cu_r_set(dbref player, void *data, char *buffer);
void cu_s_add(dbref player, void *data, char *buffer);
void cu_s_minus(dbref player, void *data, char *buffer);
void cu_s_toggle(dbref player, void *data, char *buffer);
void cu_s_set(dbref player, void *data, char *buffer);
void cu_t_add(dbref player, void *data, char *buffer);
void cu_t_minus(dbref player, void *data, char *buffer);
void cu_t_toggle(dbref player, void *data, char *buffer);
void cu_t_set(dbref player, void *data, char *buffer);
void cu_u_add(dbref player, void *data, char *buffer);
void cu_u_minus(dbref player, void *data, char *buffer);
void cu_u_toggle(dbref player, void *data, char *buffer);
void cu_u_set(dbref player, void *data, char *buffer);
void cu_v_add(dbref player, void *data, char *buffer);
void cu_v_minus(dbref player, void *data, char *buffer);
void cu_v_toggle(dbref player, void *data, char *buffer);
void cu_v_set(dbref player, void *data, char *buffer);
void cu_w_add(dbref player, void *data, char *buffer);
void cu_w_minus(dbref player, void *data, char *buffer);
void cu_w_toggle(dbref player, void *data, char *buffer);
void cu_w_set(dbref player, void *data, char *buffer);
void cu_x_add(dbref player, void *data, char *buffer);
void cu_x_minus(dbref player, void *data, char *buffer);
void cu_x_toggle(dbref player, void *data, char *buffer);
void cu_x_set(dbref player, void *data, char *buffer);
void cu_y_add(dbref player, void *data, char *buffer);
void cu_y_minus(dbref player, void *data, char *buffer);
void cu_y_toggle(dbref player, void *data, char *buffer);
void cu_y_set(dbref player, void *data, char *buffer);
void cu_z_add(dbref player, void *data, char *buffer);
void cu_z_minus(dbref player, void *data, char *buffer);
void cu_z_toggle(dbref player, void *data, char *buffer);
void cu_z_set(dbref player, void *data, char *buffer);

#endif				/* _P_MECH_CUSTOM_H */
