# x86_adapt
A Linux kernel module, that allows changing/toggling system parameters stored in MSR and PCI registers of x86 processors



## Dependencies
- python
- make
- gcc
- linux kernel > version //TODO: test old debian with 2.6.32
- linux kernel headers

## Install

1. generate the device driver from the template and build the kernel module

```
    $ cd <x86_adapt_folder>
    $ mkdir build
    $ cd build
    $ cmake ../
    $ make

```
2. load the kernel module

```
    # insmmod x86_adapt_driver.ko
```

Remarks:
You can also build a debian package with generate-deb.sh.
This kernel module has to be rebuild with every kernel update.

##  Adding Features/Knobs

To add a new Feature to the driver you have to create new txt file in 
the directory knobs. A knob file uses the following syntax.
Every variable starts with //# and their value is written in the next line.
Strings aren't put into quotes or single quotes and can span multiple
lines. Every variable except restricted_settings is mandatory.

```
//#description
value: String. use this to describe the new feature.
//#device
possible values: MSR, NB_F0, NB_F1, NB_F2, NB_F3, NB_F4, NB_F5, SB_PCU0, SB_PCU1, 
 SB_PCU2, HSW_PCU0, HSW_PCU1, HSW_PCU2. This reflects the PCI Device/Function or MSR.
//#register_index
value: hex number. This defines the MSR/PCI register number.
//#bit_mask
value: bitmask in C syntax. This is used to make only parts of the register available.
//#restricted_settings
value: available settings separated by comas, or "readonly"
//#reserved_settings
value: reserved settings separated by comas
//#processor_groups
value: one or more processor groups delimited by ','
```

The variable name (```//#name```) is optional and overrides the name defined by
the text file. This variable can be used when the same knob has different register
locations on different processors.

The processor groups are defined as txt files in the directory processors.
They use the same syntax. The following variables are used to describe a
processor.

```
//#vendor
possible values are defined in: arch/x86/include/asm/processor.h, examples: X86_VENDOR_INTEL, X86_VENDOR_AMD
//#families
value: hex number
//#families
value: hex number
//#models
value: one or more hex numbers delimited by ','
```

usage of libx86_adapt:
	Please have a look at library/examples.
