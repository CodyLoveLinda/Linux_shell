import re
import matplotlib.pyplot as plt
import numpy as np
import argparse

def is_number(s):
    try:
        float(s)
        return True
    except ValueError:
        pass
 
    try:
        import unicodedata
        unicodedata.numeric(s)
        return True
    except (TypeError, ValueError):
        pass
 
    return False

def main(filename):
    print(filename)
    file = open(filename,'r')
    list = []
    # search the line including accuracy
    for line in file:
        #print(len(line))
        if len(line) == 85 :
            #print(len(line))
            line = ' '.join(line.split())
            #print(line)
            #str_line = line.replace(' ', '')
            str_list = line.split(" ")
            #print(str_list[3])
            if is_number(str_list[3]) :
                list.append(float(str_list[3]))
            #str_list_2 = str_list[0].split(":")
            #if str_list_2[0] == "%Cpu0" :
            #    str_list_3 = str_list_2[1].split("u")
            #    list.append(float(str_list_3[0]))
        
    file.close()
    #plt.plot(list, 'go')
    plt.plot(list, 'r')
    #plt.legend()
    plt.xlabel('count')
    plt.ylabel('usage %')
    plt.title('cpu usage')
    plt.show()

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('filename', type=str)
    args = parser.parse_args()
    print(args.filename)
    main(args.filename)
