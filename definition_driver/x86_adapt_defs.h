
/* everything I could think of :P */
enum{
    MSR = 0,
    NB_F0, /* Device 15, Function 0 */
    NB_F1, /* Device 15, Function 1 */
    NB_F2, /* Device 15, Function 2 */
    NB_F3, /* Device 15, Function 3 */
    NB_F4, /* Device 15, Function 4 */
    NB_F5, /* Device 15, Function 5 */
    SB_PCU0, /* Device 10, Function 0 */
    SB_PCU1, /* Device 10, Function 1 */
    SB_PCU2, /* Device 10, Function 2 */
    HSW_PCU0, /* Device 30, Function 0 */
    HSW_PCU1, /* Device 30, Function 1 */
    HSW_PCU2, /* Device 30, Function 2 */
    UNCORE,
};

/* 4 byte id + 1 byte length + 4 byte name length + 4 byte description length */
#define ENTRY_HEADER_SIZE 13 

struct knob_entry {
    /* user */
    char * name;
    char * description;
    u32 id;
    u8 length;
    /* internal */
    u8 device;
    u8 readonly;
    u64 register_index;
    u64 bitmask;
    u32 restricted_settings_length;
    u32 reserved_settings_length;
    u64 * restricted_settings;
    u64 * reserved_settings;
};

struct fam_struct {
    u16 family;
    u8 model_length;
    u16 * models; 
};

struct knob_vendor {
    u8 vendor;
    u8 features_length;
    u64 * features;
    u8 fam_length;
    struct fam_struct * fams;
};

struct knob_entry_definition {
    struct knob_entry knob;
    u16 av_length;
    struct knob_vendor ** av_vendors;
    int blocked_by_cpuid;
};

struct knob_entry_definition * x86_adapt_get_all_knobs(void);

u32 x86_adapt_get_all_knobs_length(void);
