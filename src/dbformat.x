typedef int     int32_t;
typedef unsigned int    uint32_t;
typedef hyper       int64_t;
typedef unsigned hyper  uint64_t;
typedef opaque string<>;
const DB_MAGIC = 0x4841475A;

struct db_header {
    int32_t magic;
    int32_t version;
    int32_t dumptime_seconds; // UTC
    int32_t dumptime_useconds; // UTC
    int32_t revision;
    int32_t xid[5];
    db_attrdef<> attrs;
    db_object<> objects;
}

struct db_attrdef {
    string Name;
    int32_t number;
    int32_t flags;
}

struct db_object {
    int32_t dbref;
    string Name;
    int32_t Location;
    int32_t Zone;
    int32_t Contents;
    int32_t Exits;
    int32_t Link;
    int32_t Next;
    int32_t Owner;
    int32_t Parent;
    int32_t Pennies;
    int32_t Flags;
    int32_t Flags2;
    int32_t Flags3;
    int32_t Powers;
    int32_t Powers2;
    db_attr<> attrs;
}

struct db_attr {
    string data;
    int32_t number;
}


    

