class Knob():
	name=""
	description=""
	device=None
	register_index=None
	bit_mask=None
	restricted_settings=[]
	reserved_settings=[]
	processor_groups=[]
	readonly=False
	def __init__(self, inputfilename):
		self.name=inputfilename[inputfilename.rfind('/')+1:inputfilename.rfind('.')]
		inputfile=open(inputfilename)
		data = list(inputfile)
		for line_nr in range(0,len(data)-1):
			if (data[line_nr].strip()=='//#name'):
				self.name=data[line_nr+1].strip()
			if (data[line_nr].strip()=='//#description'):
				self.description=data[line_nr+1].strip()
			if (data[line_nr].strip()=='//#device'):
				self.device=data[line_nr+1].strip()
			if (data[line_nr].strip()=='//#register_index'):
				self.register_index=data[line_nr+1].strip()
			if (data[line_nr].strip()=='//#bit_mask'):
				self.bit_mask=data[line_nr+1].strip()
			if data[line_nr+1].strip()=="ro" or data[line_nr+1].strip()=="readonly":
				self.readonly=True
			else:
				if (data[line_nr].strip()=='//#restricted_settings'):
					self.restricted_settings=data[line_nr+1].strip().split(',')
				if (data[line_nr].strip()=='//#reserved_settings'):
					self.reserved_settings=data[line_nr+1].strip().split(',')
			if (data[line_nr].strip()=='//#processor_groups'):
				self.processor_groups=data[line_nr+1].strip().split(',')
		inputfile.close()
