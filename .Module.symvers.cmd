cmd_/home/div/Codes/signal_module/Module.symvers :=  sed 's/ko$$/o/'  /home/div/Codes/signal_module/modules.order | scripts/mod/modpost -m -a    -o /home/div/Codes/signal_module/Module.symvers -e -i Module.symvers -T - 
