#!/usr/bin/python

class HaswellPciBox:
	prefix=None
	processor_group=None
	name=None
	description=None
	device=None
	has_ctl=False
	nr_fixed=0
	nr_ctr=0
	nr_filter=0
	has_status=False
	ctl_bitmask=None
	fixed_ctl_bitmask=None
	fixed_ctr_bitmask=None
	eventsel_bitmask=None
	filter_bitmasks=None
	status_bitmask=None
	ctr_bitmask=None
	config_bitmask=None

	def __init__(self,prefix,processor_group,name,description, device, has_ctl=True, nr_fixed=0, nr_ctr=0, nr_filter=0, has_status=True, ctl_bitmask=None, fixed_ctl_bitmask=None, fixed_ctr_bitmask=None, eventsel_bitmask=None, filter_bitmasks=None, status_bitmask=None, ctr_bitmask=None, config_bitmask=None):
		self.prefix=prefix
		self.processor_group=processor_group
		self.name=name
		self.description=description
		self.device=device
		self.has_ctl=has_ctl
		self.nr_fixed=nr_fixed
		self.nr_ctr=nr_ctr
		self.nr_filter=nr_filter
		self.has_status=has_status
		self.ctl_bitmask=ctl_bitmask
		self.fixed_ctl_bitmask=fixed_ctl_bitmask
		self.fixed_ctr_bitmask=fixed_ctr_bitmask
		self.filter_bitmasks=filter_bitmasks
		self.eventsel_bitmask=eventsel_bitmask
		self.status_bitmask=status_bitmask
		self.ctr_bitmask=ctr_bitmask
		self.config_bitmask=config_bitmask

	def print_one(self,register,name,descr,bitmask):
		file_name = "{0}{1}_PMON_{2}.txt".format(self.prefix, self.name, name)
		print("write file", file_name)
		f=open(file_name,"w")
		text=""
		text=text+"//#name\n"+self.name+"_PMON_"+name+"\n"
		text=text+"//#description\n"+self.description+" "+descr
		count_shifted_bits=0
		bitmask_int=int(bitmask,16)
		while bitmask_int%2 == 0:
			bitmask_int=bitmask_int>>1
			count_shifted_bits=count_shifted_bits+1
		if count_shifted_bits > 0:
			text=text+"(The register content is shifted by "+str(count_shifted_bits)+" bits to prevent invalid access. I.e. a x86_adapt setting of 1 is equal to a register setting of "+hex(1<<count_shifted_bits)+")"
		text=text+"\n"
		text=text+"//#device\n"+self.device+"\n"
		text=text+"//#register_index\n"+str(register)+"\n"
		text=text+"//#bit_mask\n"+bitmask+"\n"
		text=text+"//#processor_groups\n"+self.processor_group+"\n"
		f.write(text)
		f.close()

	def print_all(self):
		# counters
		for i in range(self.nr_ctr):
			self.print_one(160+8*i,"CTR"+str(i), "Event Counter "+str(i),self.ctr_bitmask)
		for i in range(self.nr_ctr):
			self.print_one(216+4*i, "CTL"+str(i), "Control for Counter "+str(i),self.eventsel_bitmask)
		if self.nr_fixed == 1:
			self.print_one(208, "FIXED_CTR", "Fixed Counter",self.fixed_ctr_bitmask)
			self.print_one(240, "FIXED_CTL", "Fixed Counter Control",self.fixed_ctl_bitmask)
			
		if self.has_ctl:
			self.print_one(244, "BOX_CTL", "Control",self.ctl_bitmask)
		if self.has_status:
			self.print_one(248, "STATUS", "Status",self.status_bitmask)

class Box:
	prefix=None
	processor_group=None
	name=None
	description=None
	start_adress=0
	device=None
	has_ctl=False
	skip_before_fixed=0
	nr_fixed=0
	nr_ctr=0
	skip_before_eventsel=0
	skip_before_bitmask=0
	nr_filter=0
	skip_before_status=0
	skip_before_ctr=0
	has_config=False
	has_status=False
	ctl_bitmask=None
	fixed_ctl_bitmask=None
	fixed_ctr_bitmask=None
	eventsel_bitmask=None
	filter_bitmasks=None
	status_bitmask=None
	ctr_bitmask=None
	config_bitmask=None

	def __init__(self,prefix,processor_group,name,description,start_adress, device="MSRNODE", has_ctl=True, skip_before_fixed=0, nr_fixed=0, nr_ctr=0, skip_before_eventsel=0, skip_before_bitmask=0, nr_filter=0, skip_before_status=0, skip_before_ctr=0, has_config=False, has_status=False, ctl_bitmask=None, fixed_ctl_bitmask=None, fixed_ctr_bitmask=None, eventsel_bitmask=None, filter_bitmasks=None, status_bitmask=None, ctr_bitmask=None, config_bitmask=None):
		self.prefix=prefix
		self.processor_group=processor_group
		self.name=name
		self.description=description
		self.start_adress=start_adress
		self.device=device
		self.has_ctl=has_ctl
		self.skip_before_fixed=skip_before_fixed
		self.nr_fixed=nr_fixed
		self.nr_ctr=nr_ctr
		self.skip_before_eventsel=skip_before_eventsel
		self.skip_before_bitmask=skip_before_bitmask
		self.nr_filter=nr_filter
		self.skip_before_status=skip_before_status
		self.skip_before_ctr=skip_before_ctr
		self.has_config=has_config
		self.has_status=has_status
		self.ctl_bitmask=ctl_bitmask
		self.fixed_ctl_bitmask=fixed_ctl_bitmask
		self.fixed_ctr_bitmask=fixed_ctr_bitmask
		self.filter_bitmasks=filter_bitmasks
		self.eventsel_bitmask=eventsel_bitmask
		self.status_bitmask=status_bitmask
		self.ctr_bitmask=ctr_bitmask
		self.config_bitmask=config_bitmask

	def print_one(self,register,name,descr,bitmask):
		file_name = "{0}{1}_PMON_{2}.txt".format(self.prefix, self.name, name)
		print("write file", file_name)
		f=open(file_name,"w")
		text=""
		text=text+"//#name\n"+self.name+"_PMON_"+name+"\n"
		text=text+"//#description\n"+self.description+" "+descr+"\n"
		text=text+"//#device\n"+self.device+"\n"
		text=text+"//#register_index\n"+str(register)+"\n"
		text=text+"//#bit_mask\n"+bitmask+"\n"
		text=text+"//#processor_groups\n"+self.processor_group+"\n"
		f.write(text)
		f.close()


	def print_all(self):
		count=self.start_adress
		if self.has_ctl:
			self.print_one(count, "BOX_CTL", "Control",self.ctl_bitmask)
			count = count + 1

		if self.nr_fixed == 1:
			self.print_one(count, "FIXED_CTL", "Fixed Counter Control",self.fixed_ctl_bitmask)
			count = count + 1
		elif self.nr_fixed > 1:
			for i in range(nr_fixed):
				self.print_one(count, "FIXED_CTL"+str(i), "Fixed Counter "+str(i)+" Control",self.fixed_ctl_bitmask)
				count = count + 1

		count= count+self.skip_before_fixed
		if self.nr_fixed == 1:
			self.print_one(count, "FIXED_CTR", "Fixed Counter",self.fixed_ctr_bitmask)
			count = count + 1
		elif self.nr_fixed > 1:
			for i in range(nr_fixed):
				self.print_one(count, "FIXED_CTR"+str(i), "Fixed Counter "+str(i),self.fixed_ctr_bitmask)
				count = count + 1

		count= count+self.skip_before_eventsel
		if self.nr_ctr == 1:
			self.print_one(count, "CTL", "Control for Counter",self.eventsel_bitmask)
			count = count + 1
		elif self.nr_ctr > 1:
			for i in range(self.nr_ctr):
				self.print_one(count, "CTL"+str(i), "Control for Counter "+str(i),self.eventsel_bitmask)
				count = count + 1

		
		count= count+self.skip_before_bitmask
		if self.nr_filter == 1:
			self.print_one(count, "FILTER", "Event Filter",self.filter_bitmasks)
			count = count + 1
		elif self.nr_filter > 1:
			for i in range(self.nr_filter):
				self.print_one(count, "FILTER"+str(i), "Event Filter "+str(i),self.filter_bitmasks[i])
				count = count + 1

		if self.has_status:
			count= count+self.skip_before_status
			self.print_one(count, "STATUS", "Status",self.status_bitmask)
			count = count + 1

		count= count+self.skip_before_ctr
		if self.nr_ctr == 1:
			self.print_one(count, "CTR", "Event Counter",self.ctr_bitmask)
			count = count + 1
		elif self.nr_ctr > 1:
			for i in range(self.nr_ctr):
				self.print_one(count, "CTR"+str(i), "Event Counter "+str(i),self.ctr_bitmask)
				count = count + 1

		if self.has_config:
			self.print_one(count, "CONFIG", "Configuration",self.config_bitmask)
			count = count + 1


# Haswell EP
boxes=[]
boxes.append(Box("Intel_Haswell_","haswell_ep","GLOBAL", "Global",0x700,has_config=True,has_status=True,ctl_bitmask="0xE003FFFF",status_bitmask="0xC3FF00007",config_bitmask="0x1F"))
# U Box
boxes.append(Box("Intel_Haswell_","haswell_ep","U", "U-box",0x703,has_ctl=False,nr_fixed=1,nr_ctr=2,skip_before_status=1,has_status=True, status_bitmask="0x3",eventsel_bitmask="0x1FD6FFFF",ctr_bitmask="0xFFFFFFFFFFFF",fixed_ctl_bitmask="0x500000",fixed_ctr_bitmask="0xFFFFFFFFFFFF"))
# PCU
boxes.append(Box("Intel_Haswell_","haswell_ep","PCU", "Power control Unit", 0x710, nr_ctr=4, nr_filter=1, has_status=True, ctl_bitmask="0x103", status_bitmask="0xF", eventsel_bitmask="0xDFF6C0FF", ctr_bitmask="0xFFFFFFFFFFFF",filter_bitmasks="0xFFFFFFFF"))
# s boxes
for i in range(4):
	boxes.append(Box("Intel_Haswell_","haswell_ep","S"+str(i), "S-box "+str(i),0x720+i*0xa,nr_ctr=4,has_status=True,ctl_bitmask="0x103",status_bitmask="0xF",eventsel_bitmask="0xFFCEFFFF", ctr_bitmask="0xFFFFFFFFFFFF"))
# c boxes
for i in range(18):
	boxes.append(Box("Intel_Haswell_","haswell_ep","C"+str(i), "C-box "+str(i), 0xe00+i*0x10, nr_ctr=4, nr_filter=2, has_status=True, ctl_bitmask="0x103", status_bitmask="0xF", eventsel_bitmask="0xFFCEFFFF", ctr_bitmask="0xFFFFFFFFFFFF", filter_bitmasks=["0xFE003F","0xDFF0FFFF"]))

# Home Agent (todo: filters :( )
for i in range(2):
	boxes.append(HaswellPciBox("Intel_Haswell_","haswell_ep","HA"+str(i),"Home Agent "+str(i),"HSW_PMON_HA"+str(i), nr_ctr=4, has_status=True, has_ctl=True, ctl_bitmask="0x103", status_bitmask="0xF",eventsel_bitmask="0xFFD6FFFF",ctr_bitmask="0xFFFFFFFFFFFF" ))

# Memory Controller
for channel in range(4):
	for mc in range(2):
		boxes.append(HaswellPciBox("Intel_Haswell_","haswell_ep","IMC"+str(mc)+"_CHAN"+str(channel),"Memory Controller "+str(mc)+" Channel "+str(channel),"HSW_PMON_MC"+str(mc)+"_CHAN"+str(channel), nr_ctr=4, has_status=True, has_ctl=True, nr_fixed=1, ctl_bitmask="0x103", status_bitmask="0x1F",eventsel_bitmask="0xFFD6FFFF",fixed_ctl_bitmask="0xD80000", ctr_bitmask="0xFFFFFFFFFFFF", fixed_ctr_bitmask="0xFFFFFFFFFFFF" ))

# IRP
boxes.append(HaswellPciBox("Intel_Haswell_","haswell_ep","IRP0","IRP 0","HSW_PMON_IRP", nr_ctr=2, has_status=True, has_ctl=True, ctl_bitmask="0x103", status_bitmask="0xF",eventsel_bitmask="0xFFC6FFFF",ctr_bitmask="0xFFFFFFFFFFFF" ))

# QPI (Todo: filters and stuff, QPI1 on haswell ex)
boxes.append(HaswellPciBox("Intel_Haswell_","haswell_ep","QPI0","Quick Path Interface 0","HSW_PMON_QPI_P0", nr_ctr=4, has_status=True, has_ctl=True, ctl_bitmask="0x103", status_bitmask="0xF",eventsel_bitmask="0xFFF6FFFF",ctr_bitmask="0xFFFFFFFFFFFF" ))

#R2PCI
boxes.append(HaswellPciBox("Intel_Haswell_","haswell_ep","R2PCIe","Ring to PCIe","HSW_PMON_R2PCIE", nr_ctr=4, has_status=True, has_ctl=True, ctl_bitmask="0x103", status_bitmask="0xF",eventsel_bitmask="0xFFD6FFFF",ctr_bitmask="0xFFFFFFFFFFFF" ))

# R3QPI (R3QPI L1 only on haswell ex)
boxes.append(HaswellPciBox("Intel_Haswell_","haswell_ep","R3QPI0_Link_0","Ring to QPI Interface Link 0","HSW_PMON_R3QPI_L0", nr_ctr=3, has_status=True, has_ctl=True, ctl_bitmask="0x103", status_bitmask="0x7",eventsel_bitmask="0xFFF6FFFF",ctr_bitmask="0xFFFFFFFFFFF" ))
boxes.append(HaswellPciBox("Intel_Haswell_","haswell_ep","R3QPI0_Link_1","Ring to QPI Interface Link 1","HSW_PMON_R3QPI_L1", nr_ctr=3, has_status=True, has_ctl=True, ctl_bitmask="0x103", status_bitmask="0x7",eventsel_bitmask="0xFFF6FFFF",ctr_bitmask="0xFFFFFFFFFFF" ))


#Sandy Bridge
boxes.append(Box("Intel_SandyBridge_","sandybridge_ep","U", "U-box",0xC08,has_ctl=False,nr_fixed=1,nr_ctr=2,skip_before_eventsel=6,skip_before_ctr=4,ctl_bitmask="0x1FC7FFFF",ctr_bitmask="0xFFFFFFFFFFFF", fixed_ctl_bitmask="0x400000",fixed_ctr_bitmask="0xFFFFFFFFFFFF",eventsel_bitmask="0x1FC6FFFF"))
boxes.append(Box("Intel_SandyBridge_","sandybridge_ep","PCU", "Power control Unit",0xC24,nr_ctr=4,skip_before_eventsel=11,nr_filter=1,skip_before_ctr=1, ctl_bitmask="0x10103", status_bitmask="0xF", eventsel_bitmask="0xDFC6C0FF", ctr_bitmask="0xFFFFFFFFFFFF",filter_bitmasks="0xFFFFFFFF" ))
# c boxes
for i in range(8):
	boxes.append(Box("Intel_SandyBridge_","sandybridge_ep","C"+str(i), "C-box "+str(i),0xd04+i*0x20,nr_ctr=4,skip_before_eventsel=11,nr_filter=1,skip_before_ctr=1, ctl_bitmask="10103",eventsel_bitmask="0xFFCEFFFF",ctr_bitmask="0xFFFFFFFFFFFF", filter_bitmasks="0xFFFFFC1F"))

for box in boxes:
	box.print_all()



