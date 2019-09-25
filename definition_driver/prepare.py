#!/usr/bin/env python2

#avoid bytecode files, which are not tracked by dpkg
#which would cause errors when removing the deb package
import sys
sys.dont_write_bytecode = True

import cpu,os,re,knob

processor_group_folder="./processors/"
knob_folder="./knobs/"
template="x86_adapt_defs_template.c"
target="x86_adapt_defs.c"
target_path="./"

if len(sys.argv) >= 2:
    path=str(sys.argv[1])
    if path[-1] != "/":
        path=path+"/"
    target=path+target
    target_path=path

if len(sys.argv) == 3:
    path=str(sys.argv[2])
    if path[-1] != "/":
        path=path+"/"
    source=path
    processor_group_folder=source+processor_group_folder
    knob_folder=source+knob_folder
    template=source+template

def bitCount(int_type):
    count = 0
    while(int_type):
        int_type &= int_type - 1
        count += 1
    return(count)
processor_groups=[]
single_processor_groups=set()
knobs=[]

# get the processor groups
for filename in os.listdir(processor_group_folder):
    if re.match('.*\.txt$', filename) != None:
        processor_groups.append(cpu.ProcessorGroup(processor_group_folder+filename))

# read the knobs
for filename in os.listdir(knob_folder):
    if re.match('.*\.txt$', filename) != None:
        knobs.append(knob.Knob(knob_folder+filename))

# find processor groups with one element
for knob in knobs:
    if len(knob.processor_groups) == 1:
        single_processor_groups.add(knob.processor_groups[0])
    

# write the processor groups to c

text=""

for processor_group in processor_groups:
    for i in range(0,len(processor_group.families)):
        if len(processor_group.models[i])>0:
            family = processor_group.families[i]
            text = text+"static u16 "+processor_group.name+"_"+family+"_models[]={"
            for model in processor_group.models[i]:
                text = text + model+","
            text = text[0:-1]
            text = text + "};\n"
    if len(processor_group.families) > 0:
        text = text + "static struct fam_struct fam_"+processor_group.name+"[] = {\n"
        for i in range(0,len(processor_group.families)):
            family = processor_group.families[i]
            text = text + "\t{\n"
            text = text + "\t.family = "+family+",\n"
            text = text + "\t.model_length = "+str(len(processor_group.models[i]))+",\n"
            if len(processor_group.models[i])>0:
                text = text + "\t.models = "+processor_group.name+"_"+family+"_models}\n,"
            else:
                text = text + "\t}\n,"
        text = text[0:-1]
        text = text + "};\n"
    if len(processor_group.features) > 0:
        text = text + "static u64 features_"+processor_group.name+"[] = {"
        for feature in processor_group.features:
                text = text + feature+","
        text = text[0:-1]
        text = text + "};\n"
    text = text+ "static struct knob_vendor "+processor_group.name+" __attribute__((unused)) = {\n"
    text = text+ "\t.vendor="+processor_group.vendor+",\n"
    text = text+ "\t.fam_length="+str(len(processor_group.families))+",\n"
    if len(processor_group.families) > 0:
        text = text+ "\t.fams=fam_"+processor_group.name+",\n"
    text = text+ "\t.features_length="+str(len(processor_group.features))+",\n"
    if len(processor_group.features) > 0:
        text = text+ "\t.features=features_"+processor_group.name+"};\n"
    else:
        text = text+ "};\n"
    if  processor_group.name in single_processor_groups:
        text = text+ "static struct knob_vendor* "+processor_group.name+"_ref []= {&"+processor_group.name+"};\n"

# unify processor groups
unified_groups_knobs=[]
for knob in knobs:
    print knob.name,
    found_entry=None
    for existing_knob in unified_groups_knobs:
        if len(existing_knob.processor_groups)==len(knob.processor_groups):
            nr_matches=0
            for entry in existing_knob.processor_groups:
                if entry in knob.processor_groups:
                    nr_matches=nr_matches+1
            if len(existing_knob.processor_groups)==nr_matches:
                found_entry=existing_knob
                break
    if found_entry!=None:
        print knob.processor_groups, "Found in", found_entry.name, found_entry.processor_groups
        knob.processor_group_reuse=found_entry
    else:
        print "added"
        unified_groups_knobs.append(knob)
        
# write the knobs

## restricted settings per knob
for knob in knobs:
    if len(knob.restricted_settings)!=0:
        text = text + "static u64 res_setting_"+knob.name+"[]={"
        for res_setting in knob.restricted_settings:
            text = text +str(res_setting)+ ","
        text = text[0:-1]
        text = text + "};\n"
    if len(knob.reserved_settings)!=0:
        text = text + "static u64 resvd_setting_"+knob.name+"[]={"
        for resvd_setting in knob.reserved_settings:
            text = text +str(resvd_setting)+ ","
        text = text[0:-1]
        text = text + "};\n"
        
for knob in unified_groups_knobs:
    if len(knob.processor_groups) > 1:
        text = text + "static struct knob_vendor* processorgroups_"+knob.name+"[]={"
        for processor_group in knob.processor_groups:
            text = text + "&"+processor_group + ","
        text = text[0:-1]
        text = text + "};\n"
    


nda=False

text = text + "static u32 all_knobs_length="+str(len(knobs))+";\n"
text = text + "static struct knob_entry_definition all_knobs [] = {\n"

for knob in knobs:
    text = text + "\t{\n"
    text = text + "\t.knob.name=\""+knob.override_name+"\",\n"
    text = text + "\t.knob.description=\""+knob.description+"\",\n"
    text = text + "\t.knob.device="+knob.device+",\n"
    text = text + "\t.knob.register_index="+knob.register_index+",\n"
    if knob.bit_mask is None:
        knob.bit_mask = "0xFFFFFFFFFFFFFFFF"
    text = text + "\t.knob.bitmask="+knob.bit_mask+",\n"
    start=knob.bit_mask.find('x')
    if start == -1:
        start=knob.bit_mask.find('(')
    start=start+1
    end = knob.bit_mask.find('U')
    if end == -1:
        end = knob.bit_mask.find("L")
    if end == -1:
        end = knob.bit_mask.find(")")
    if end == -1:
        end=len(knob.bit_mask)
#   print knob.bit_mask
#   print start
#   print end

    if knob.readonly:
        text = text + "\t.knob.readonly=1,\n"
    else:
        text = text + "\t.knob.readonly=0,\n"

    text = text + "\t.knob.length="+str(bitCount(int(knob.bit_mask[start:end],16)))+",\n"
    text = text + "\t.knob.restricted_settings_length="+str(len(knob.restricted_settings))+",\n"
    text = text + "\t.knob.reserved_settings_length="+str(len(knob.reserved_settings))+",\n"
    if len(knob.restricted_settings)!=0:
        text = text + "\t.knob.restricted_settings=res_setting_"+knob.name+",\n"
    if len(knob.reserved_settings)!=0:
        text = text + "\t.knob.reserved_settings=resvd_setting_"+knob.name+",\n"
    text = text + "\t.av_length="+str(len(knob.processor_groups))+",\n"
    if len(knob.processor_groups) > 1:
        if knob.processor_group_reuse == None:
            text = text + "\t.av_vendors=processorgroups_"+knob.name+",\n"
        else:
            text = text + "\t.av_vendors=processorgroups_"+knob.processor_group_reuse.name+",\n"
    else:
        text = text + "\t.av_vendors="+knob.processor_groups[0]+"_ref\n,"
    text = text + "\t.blocked_by_cpuid=0}\n,"
	
    
    if knob.nda:
        nda = True

# remove trailing coma
text = text[0:-1]

text = text + "};\n"

# cpuid filter
text = text + "static inline void do_cpuid_checks(void){\n"
text = text + "  unsigned int eax,ebx,ecx,edx;\n"
index = 0
for knob in knobs:
    if knob.cpuid:
        text = text + "  cpuid("+knob.cpuid.operation+",&eax,&ebx,&ecx,&edx);\n"
        text = text + "  if (! ("+knob.cpuid.check+"))\n"
        text = text + "    all_knobs["+str(index)+"].blocked_by_cpuid=1;\n\n"
    index = index + 1
text = text + "}"

# now open template
# and replace "#template_holder" with text

if nda:
    nda_file=open(target_path+"IMPORTANT_README.txt","w")
    nda_file.write(
"This kernel module contains NDA information.\n"
"Do not distribute it, do not copy it, do not make it available to anyone!\n"
)
    nda_file.close()
templatefile=open(template)
targetfile=open(target,"w")
if nda:
    targetfile.write(
"/*****************************************************************************/\n"
"/* This kernel module contains NDA information.                              */\n"
"/* Do not distribute it, do not copy it, do not make it available to anyone! */\n"
"/*****************************************************************************/\n"
)


for line in templatefile:
    if line.strip()=="#template_holder":
        targetfile.write(text)
    else:
        targetfile.write(line)
templatefile.close()
targetfile.close()
