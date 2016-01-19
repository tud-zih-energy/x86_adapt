
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

    MSRNODE, /* MSRs that are available once per package */

    HSW_PMON_HA0, /* Device 18, Function 1 */
    HSW_PMON_HA1, /* Device 18, Function 5 */
    HSW_PMON_MC0_CHAN0, /* Device 20, Function 0 */
    HSW_PMON_MC0_CHAN1, /* Device 20, Function 1 */
    HSW_PMON_MC0_CHAN2, /* Device 21, Function 0 */
    HSW_PMON_MC0_CHAN3, /* Device 21, Function 1 */
    HSW_PMON_MC1_CHAN0, /* Device 23, Function 0 */
    HSW_PMON_MC1_CHAN1, /* Device 23, Function 1 */
    HSW_PMON_MC1_CHAN2, /* Device 24, Function 0 */
    HSW_PMON_MC1_CHAN3, /* Device 24, Function 1 */
    HSW_PMON_IRP, /* Device 5, Function 6 */
    HSW_PMON_QPI_P0, /* Device 8, Function 2 */
    HSW_PMON_QPI_P1, /* Device 9, Function 2 */
    HSW_PMON_R2PCIE, /* Device 16, Function 1 */
    HSW_PMON_R3QPI_L0, /* Device 11, Function 1 */
    HSW_PMON_R3QPI_L1, /* Device 11, Function 2 */
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
