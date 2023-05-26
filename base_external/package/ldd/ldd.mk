LDD_VERSION = 49a498fb88a90d821e5cc4310f4e7ec8fef23e67
LDD_SITE = git@github.com:cu-ecen-aeld/assignment-7-lordnic99.git
LDD_SITE_METHOD = git
LDD_GIT_SUBMODULES = YES

LDD_MODULE_SUBDIRS = misc-modules scull
LDD_MODULE_MAKE_OPTS = KERNELRELEASE=$(LINUX_VERSION_PROBED)

$(eval $(kernel-module))
$(eval $(generic-package))
