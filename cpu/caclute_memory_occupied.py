import psutil
import pynvml
import time

class MonitorProcess:
    def __init__(self,pid_num):
        self.pid = pid_num
        self.pro = psutil.Process(pid_num)
        self.pro_rss = 0.0
        self.pro_vss = 0.0
        self.pro_cpu = 0.0
        self.pro_io = 0.0
        self.pro_gpu = []
        try:
            pynvml.nvmlInit()
            self.deviceCount = pynvml.nvmlDeviceGetCount()
        except:
            print("No GPU")

    def Memonitor(self):
        self.pro_rss = float(self.pro.memory_info().rss)/1024/1024
        self.pro_vss = float(self.pro.memory_info().vms)/1024/1024

    def CPUMonitor(self):
        self.pro_cpu = self.pro.cpu_percent(interval = 1)

    def GPUMonitor(self):
        self.pro_gpu = []
        try:
            for gup_id in xrange(0,self.deviceCount):
                handle = pynvml.nvmlDeviceGetHandleByIndex(gpu_id)
                meminfo = pynvml.nvmlDeviceGetMemoryInfo(handle)
                gpuratio_tmp = pynvml.nvmlDeviceGetUtilizationRates(handle)
                gpuratio = str(gpuratio_tmp.gpu) + " %"
                self.pro_gpu.append(gpuratio)
        except:
            pass
    def IoMonitor(self):
        self.pro_io = self.pro.io_counters()
    
    def GPUCount(self):
        return self.deviceCount
    
    def LastWork(self):
        list_return = []
        self.Memonitor()
        self.CPUMonitor()
        self.GPUMonitor()
        self.IoMonitor()
        list_return.append(self.pro_rss)
        list_return.append(self.pro_vss)
        list_return.append(self.pro_cpu)
        list_return.append(self.pro_gpu)
        return list_return

class CsvWrite:
    def __init__(self,csvname):
        self.f_out = open(csvname+".csv",'ab+')
        self.csv_write = csv.writer(self.f_out,dialect="excel")
    def CSVDataWrite(self,inlist):
        self.csv_write.writerow(inlist)
    def ClsoeCSV(self):
        self.f_out.close()

class TimeFuc:
    def __init__(self):
        pass
    def LocalTime(self):
        return time.strftime("%Y-%m-%d %H:%M:%S",time.localtime())

a = MonitorProcess(3439390)
while 1:
    print(a.LastWork())
