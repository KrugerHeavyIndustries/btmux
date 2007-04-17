/*
 * Module implementing the vocabulary table data structure required by X.891.
 */

#ifndef BTECH_FI_VOCAB_H
#define BTECH_FI_VOCAB_H

#include "common.h"
#include "encalg.h"

/* Sections 6.5 and 6.10: Vocabulary table indexes range from 1 to 2^20.  */
typedef unsigned long FI_VocabIndex;

#define FI_VOCAB_INDEX_NULL 0
#define FI_VOCAB_INDEX_MAX FI_ONE_MEG

/* Convenience constants for built-in restricted alphabets.  */
typedef enum {
	FI_RA_NUMERIC = 1,		/* numeric (section 9.1) */
	FI_RA_DATE_AND_TIME		/* date and time (section 9.2) */
} FI_RA_VocabIndex;

/* Convenience constants for built-in encoding algorithms.  */
typedef enum {
	FI_EA_HEXADECIMAL = 1,		/* hexadecimal (section 10.2) */
	FI_EA_BASE64,			/* Base64 (section 10.3) */
	FI_EA_SHORT,			/* 16-bit integer (section 10.4) */
	FI_EA_INT,			/* 32-bit integer (section 10.5) */
	FI_EA_LONG,			/* 64-bit integer (section 10.6) */
	FI_EA_BOOLEAN,			/* boolean (section 10.7) */
	FI_EA_FLOAT,			/* single precision (section 10.8) */
	FI_EA_DOUBLE,			/* double precision (section 10.9) */
	FI_EA_UUID,			/* UUID (section 10.10) */
	FI_EA_CDATA			/* CDATA (section 10.11) */
} FI_EA_VocabIndex;

/*
 * Restricted alphabet table implementation.
 */
typedef struct FI_tag_RA_VocabTable FI_RA_VocabTable;

FI_RA_VocabTable *fi_create_ra_table(void);
void fi_destroy_ra_table(FI_RA_VocabTable *tbl);

FI_VocabIndex fi_add_ra(FI_RA_VocabTable *tbl, const FI_Char *alphabet);
const FI_Char *fi_get_ra(const FI_RA_VocabTable *tbl, FI_VocabIndex idx);

/*
 * Encoding algorithm table implementation.
 */
typedef struct FI_tag_EA_VocabTable FI_EA_VocabTable;

FI_EA_VocabTable *fi_create_ea_table(void);
void fi_destroy_ea_table(FI_EA_VocabTable *tbl);

FI_VocabIndex fi_add_ea(FI_EA_VocabTable *tbl,
                        const FI_EncodingAlgorithm *alg);
const FI_EncodingAlgorithm *fi_get_ea(const FI_EA_VocabTable *tbl,
                                      FI_VocabIndex idx);

/*
 * Dynamic string table implementation.
 */
typedef struct FI_tag_DS_VocabTable FI_DS_VocabTable;

FI_DS_VocabTable *fi_create_ds_table(void);
void fi_destroy_ds_table(FI_DS_VocabTable *tbl);

FI_VocabIndex fi_add_ds(FI_DS_VocabTable *tbl, const FI_Char *str);
const FI_Char *fi_get_ds(const FI_DS_VocabTable *tbl, FI_VocabIndex idx);

/*
 * Dynamic name table implementation.
 */
typedef struct FI_tag_DN_VocabTable FI_DN_VocabTable;

typedef struct {
	FI_VocabIndex prefix_idx;	/* optional */
	FI_VocabIndex namespace_idx;	/* optional, required by prefix_idx */
	FI_VocabIndex local_idx;	/* required */
} FI_NameSurrogate;

FI_DN_VocabTable *fi_create_dn_table(void);
void fi_destroy_dn_table(FI_DN_VocabTable *table);

FI_VocabIndex fi_add_dn(FI_DN_VocabTable *tbl, const FI_NameSurrogate *name);
const FI_NameSurrogate *fi_get_dn(const FI_DN_VocabTable *tbl,
                                  FI_VocabIndex idx);

#endif /* !BTECH_FI_VOCAB_H */
