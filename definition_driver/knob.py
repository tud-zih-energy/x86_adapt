class CpuIdCondition():
     operation=None
     check=None

     def __init__(self, operation, check):
         self.operation=operation.strip()
         self.check=check.strip()

class Knob():
    name=""
    description=""
    device=None
    register_index=None
    bit_mask=None
    restricted_settings=[]
    reserved_settings=[]
    processor_groups=[]
    processor_group_reuse=None
    nda=False
    cpuid=None
    filename=None
    override_name=None
    def __init__(self, inputfilename):
        self.name=inputfilename[inputfilename.rfind('/')+1:inputfilename.rfind('.')]

        self.description=""
        self.device=None
        self.register_index=None
        self.bit_mask=None
        self.restricted_settings=[]
        self.reserved_settings=[]
        self.processor_groups=[]
        self.processor_group_reuse=None
        self.readonly=False
        self.nda=False
        self.cpuid=None
        self.override_name=self.name

        inputfile=open(inputfilename, encoding='utf-8')
        data = list(inputfile)
        for line_nr in range(0,len(data)-1):
            if (data[line_nr].strip()=='//#name'):
                self.override_name=data[line_nr+1].strip()
            if (data[line_nr].strip()=='//#description'):
                self.description=data[line_nr+1].strip()
                self.description=self.description.replace('\\n', '\\n"\n"')
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
            if (data[line_nr+1].strip().upper() == '//#NDA'):
                self.nda=True
            if (data[line_nr].strip()=='//#processor_groups'):
                self.processor_groups=data[line_nr+1].strip().split(',')
            if (data[line_nr].strip()=='//#CPUID'):
                items=data[line_nr+1].split(',')
                if len(items) != 2:
                    print("Malformed CPUID entry in knob definition "+inputfilename)
                self.cpuid=CpuIdCondition(items[0],items[1])
        inputfile.close()
