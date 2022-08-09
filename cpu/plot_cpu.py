import re
import matplotlib.pyplot as plt
import numpy as np


def main():
    file = open('cpunum.txt','r')
    list = []
    list1 = []
    list2 = []
    list3 = []
    list4 = []
    list5 = []
    list6 = []
    list7 = []
    list8 = []
    list9 = []
    list10 = []
    list11 = []
    list12 = []
    list13 = []
    list14 = []
    list15 = []
    # search the line including accuracy
    for line in file:
        str_line = line.replace(' ', '')
        str_list = str_line.split(",")
        str_list_2 = str_list[0].split(":")
        if str_list_2[0] == "%Cpu0" :
            str_list_3 = str_list_2[1].split("u")
            list.append(float(str_list_3[0]))
        if str_list_2[0] == "%Cpu1" :
            str_list_3 = str_list_2[1].split("u")
            list1.append(float(str_list_3[0]))
        if str_list_2[0] == "%Cpu2" :
            str_list_3 = str_list_2[1].split("u")
            list2.append(float(str_list_3[0]))
        if str_list_2[0] == "%Cpu3" :
            str_list_3 = str_list_2[1].split("u")
            list3.append(float(str_list_3[0]))
        if str_list_2[0] == "%Cpu4" :
            str_list_3 = str_list_2[1].split("u")
            list4.append(float(str_list_3[0]))
        if str_list_2[0] == "%Cpu5" :
            str_list_3 = str_list_2[1].split("u")
            list5.append(float(str_list_3[0]))
        if str_list_2[0] == "%Cpu6" :
            str_list_3 = str_list_2[1].split("u")
            list6.append(float(str_list_3[0]))
        if str_list_2[0] == "%Cpu7" :
            str_list_3 = str_list_2[1].split("u")
            list7.append(float(str_list_3[0]))
        if str_list_2[0] == "%Cpu8" :
            str_list_3 = str_list_2[1].split("u")
            list8.append(float(str_list_3[0]))
        if str_list_2[0] == "%Cpu9" :
            str_list_3 = str_list_2[1].split("u")
            list9.append(float(str_list_3[0]))
        if str_list_2[0] == "%Cpu10" :
            str_list_3 = str_list_2[1].split("u")
            list10.append(float(str_list_3[0]))
        if str_list_2[0] == "%Cpu11" :
            str_list_3 = str_list_2[1].split("u")
            list11.append(float(str_list_3[0]))
        if str_list_2[0] == "%Cpu12" :
            str_list_3 = str_list_2[1].split("u")
            list12.append(float(str_list_3[0]))
        if str_list_2[0] == "%Cpu13" :
            str_list_3 = str_list_2[1].split("u")
            list13.append(float(str_list_3[0]))
        if str_list_2[0] == "%Cpu14" :
            str_list_3 = str_list_2[1].split("u")
            list14.append(float(str_list_3[0]))
        if str_list_2[0] == "%Cpu15" :
            str_list_3 = str_list_2[1].split("u")
            list15.append(float(str_list_3[0]))
        
    file.close()
    #plt.plot(list, 'go')
    #plt.plot(list, 'r')
    plt.plot(list1, 'k', label='cpu1')
    plt.plot(list2, 'y', label='cpu2')
    plt.plot(list3, 'g', label='cpu3')
    plt.plot(list4, 'c', label='cpu4')
    plt.plot(list5, 'b', label='cpu5')
    plt.plot(list6, 'm', label='cpu6')
    plt.plot(list7, 'darkgoldenrod', label='cpu7')
    plt.plot(list8, 'khaki', label='cpu8')
    plt.plot(list9, 'saddlebrown', label='cpu9')
    plt.plot(list10, 'chartreuse', label='cpu10')
    plt.plot(list11, 'deepskyblue', label='cpu11')
    plt.plot(list12, 'purple', label='cpu12')
    plt.plot(list13, 'olive', label='cpu13')
    plt.plot(list14, 'darkslategray', label='cpu14')
    plt.plot(list15, 'royalblue', label='cpu15')
    plt.legend()
    plt.xlabel('count')
    plt.ylabel('usage %')
    plt.title('cpu usage')
    plt.show()

if __name__ == '__main__':
    main()
