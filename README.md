# meta-readonly
yocto build to image for readonly 

## Setup
No poky. In top directory `../meta-readonly`
```
mkdir -p downloads sstate-cache
```

### Install
openembedded core
```
git clone -b scarthgap git://git.openembedded.org/openembedded-core
cd openembedded-core
git clone -b 2.8 git://git.openembedded.org/bitbake
```

### Activate && Forms
```
cd ~/yocto_dev/openembedded-core
source oe-init-build-env build
```

Edit in `build/conf/bblayers.conf`
```
BBLAYERS ?= " \
  ${TOPDIR}/../meta \
  ${TOPDIR}/../../meta-readonly \
  "
```

Edit in `build/conf/local.conf`
```
DISTRO ?= "nodistro"
MACHINE ?= "qemux86-64"
DL_DIR ?= "${TOPDIR}/../../downloads"
SSTATE_DIR ?= "${TOPDIR}/../../sstate-cache"
```

## Run
```
bitbake readonly-image
```

