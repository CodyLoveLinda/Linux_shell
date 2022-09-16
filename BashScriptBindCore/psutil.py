import psutil

count = psutil.cpu_count()
p = psutil.Process()

cpu_lst = p.cpu_affinity()  # [0, 1, 2, 3]
print(cpi_lst)
p.cpu_affinity([0, 1])  
