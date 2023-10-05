# build kernel module

## Within CMake project (opt.A)
```bash
cmake [path-to-main-cmake-project]
make module
```

## make (opt.B)

```bash
make
```

# load kernel module

```bash
insmod fgpadma.ko
```


# petalinux kernel options
 
#### cma=256 parameter to be passed as bootargs. 

re-configure petalinux
```
petalinux-config
```

DTG Settings → Kernel Bootargs → unset generate boot args automatically
set user set kernel bootargs to `console=ttyPS0,115200 earlyprintk cma=256M`


#### remove the xilinx dma module:
```petalinux-config -c kernel```

Device Drivers → DMA Engine support → Xilinx AXI DMAS Engine   ***unset***


