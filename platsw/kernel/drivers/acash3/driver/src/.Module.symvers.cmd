cmd_/home/autox/platsw/kernel/drivers/acash3/driver/src/Module.symvers := sed 's/ko$$/o/' /home/autox/platsw/kernel/drivers/acash3/driver/src/modules.order | scripts/mod/modpost -m -a   -o /home/autox/platsw/kernel/drivers/acash3/driver/src/Module.symvers -e -i Module.symvers   -T -
