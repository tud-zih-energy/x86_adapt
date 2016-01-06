# x86_adapt
A Linux kernel module, that allows changing/toggling system parameters stored in MSR and PCI registers of x86 processors

Its usage was initially presented at Ena-HPC 2013:

Integrating performance analysis and energy efficiency optimizations in a unified environment, R Schöne, D Molka - Computer Science-Research and Development, 2014, [DOI:10.1007/s00450-013-0243-7](http://dx.doi.org/10.1007/s00450-013-0243-7)


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
    # This module provides symbols used by the x86_adapt kernel module
    $ insmod kernel_module/definition_driver/x86_adapt_defs.ko
    $ insmod kernel_module/driver/x86_adapt_driver.ko
```
3. (optional) install
```
    $ sudo make install
```
This will install the libraries, binaries and kernel modules to the standard system pathes (e.g. /usr/include and /lib/modules/(kernel version) ).

Remarks:
You can also build a debian package with generate-deb.sh.
This kernel module has to be rebuild with every kernel update.

## CPU Hotplug and Resetting values
x86_adapt allows users to reset all values of all available knobs to default settings.
The corresponding default values to which the single registers are reset are read when the driver (not the definition) kernel module is loaded.
If a CPU is taken offline while the module is loaded, the defaults are loaded when the CPU is taken online.
Node-defaults are captured when the driver kernel module is loaded. If all CPUs of a node are taken offline, the kernel module will fail to load since node-wide MSR registers cannot be read under these circumstances.

To reset all knobs of a device (cpu or node), write any 8 bytes to offset 0 of the corresponding device file.

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
 SB_PCU2, HSW_PCU0, HSW_PCU1, HSW_PCU2, MSRNODE. This reflects the PCI Device/Function or MSR.
MSRNODE should be used for MSRs that are available once per package/NUMA node.
//#register_index
value: hex number. This defines the MSR/PCI register number.
//#bit_mask
value: bitmask in C syntax. This is used to make only parts of the register available.
//#restricted_settings (optional)
value: allowed settings separated by comas, or "readonly"
//#reserved_settings (optional)
value: unallowed settings separated by comas
//#processor_groups
value: one or more processor groups delimited by ','
//#CPUID (optional)
operation,check in C sytax : a cpuid check, where you provide an operation (e.g."0a") and a check (e.g, "(eax&0xF) == 2" ). The whole would look like "0a,(eax&0xF) == 2"
//#NDA (optional) defines the knob as NDA
//#name (optional)
override_name: overrides the name defined by the text file. This variable can be used when the same knob has different register locations on different processors.

```


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
