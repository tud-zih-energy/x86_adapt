class ProcessorGroup():
	name=""
	vendor=""
	families=[]
	features=[]
	models=[]
	def __init__(self, inputfilename):
		self.name=inputfilename[inputfilename.rfind('/')+1:inputfilename.rfind('.')]
		inputfile=open(inputfilename)
		data = list(inputfile)
		self.models=[]
		self.families=[]
		self.features=[]
		for line_nr in range(0,len(data)-1):
			if (data[line_nr].strip()=='//#vendor'):
				self.vendor=data[line_nr+1].strip()
			if (data[line_nr].strip()=='//#families'):
				all_families=data[line_nr+1].strip()
				self.families=all_families.split(',')
				for family in self.families:
					self.models.append([])
			if (data[line_nr].strip()=='//#models'):
				self.models=[]
				all_models_for_all_families=data[line_nr+1].strip().split('|')
				for current_models in all_models_for_all_families:
					self.models.append(current_models.split(','))
			if (data[line_nr].strip()=='//#features'):
				all_features=data[line_nr+1].strip()
				self.features=all_features.split(',')
			
		inputfile.close()
