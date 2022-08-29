#/bin/bash
# jinyuan Li 5/26/2022

#深圳南山办公室23层打印机配置脚本
if [ $(whoami) != root ]; then
    echo Please  run this script with sudo
    exit
fi

PrinterDriver=`lpinfo -m|grep -i fujifilm|awk '{print $1}'`
if [  -z $PrinterDriver ];then
    sudo wget https://m3support-fb.fujifilm-fb.com.cn//driver_downloads/fflinuxprint_1.1.3-4_amd64.deb
    chmod +x ./fflinuxprint_1.1.3-4_amd64.deb
    dpkg -i ./fflinuxprint_1.1.3-4_amd64.deb
    PrinterDriver=`lpinfo -m|grep -i fujifilm|awk '{print $1}'`
fi

lpadmin -p 23-administrative -E -v socket://10.10.30.64:9100 -m $PrinterDriver >/dev/null 2>&1
sed -i -r '/DefaultFFColorMode/s#(.*: )(.*)#\1Black#' /etc/cups/ppd/23-administrative.ppd
echo -e "\033[32m提示: 打印机23-administrative已添加，请检查！\033[0m"
