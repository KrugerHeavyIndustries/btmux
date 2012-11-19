#ifndef BTPR__INTERFACE__H
#define BTPR__INTERFACE__H

#define HUDKEYLEN 21

typedef struct mux_descriptor_data BT_DESC;
struct mux_descriptor_data {
    char *hudkey;
    dbref player;

    void *real_desc; /* opaque pointer to the real MU* DESC object */
}; /* struct mux_descriptor_data */

#endif /* undef BTPR__INTERFACE_H */
