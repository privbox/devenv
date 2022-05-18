## BASEDIR
IMAGE_PREFIX	= piot-
IMAGES_DIR		= $(CURDIR)
DEVENV_ROOT		= $(CURDIR)/..
BUILD			= $(CURDIR)/build
PROJ_ROOT		= $(abspath $(DEVENV_ROOT)/..)
KERNEL_DIR		= $(DEVENV_ROOT)/../kernel

KERNEL = $(KERNEL_DIR)/arch/x86/boot/bzImage
INITRD = $(BUILD)/initrd.img

IMAGE_DEP_PREFIX = $(BUILD)/podman-images/

image_name = $(IMAGE_PREFIX)$(1)

cont_cmd = \
	docker run --rm \
	    $(2) \
	    --volume $(PROJ_ROOT):$(PROJ_ROOT):rw,z \
	    --volume privbox-build:/build:rw,z \
	    $(1)

ROOTFS_IMAGE     = $(call image_name,rootfs)
ROOTFS_IMAGE_DEP = $(IMAGE_DEP_PREFIX)$(ROOTFS_IMAGE)
DRACUT_IMAGE     = $(call image_name,dracut)
DRACUT_IMAGE_DEP = $(IMAGE_DEP_PREFIX)$(DRACUT_IMAGE)
DRACUT_CMD       = $(call cont_cmd,$(DRACUT_IMAGE))

GLIBC_DIST  =   $(CURDIR)/../../glibc/dist/opt/piot

## TOOLS
TOOLS_SRC   = $(DEVENV_ROOT)/tools
TOOLS_BUILD = $(IMAGES_DIR)/build/tools
TOOLS_DIST  = $(IMAGES_DIR)/dist/tools
TOOLS_DEP   = $(IMAGES_DIR)/deps/tools

# BASE IMAGE
BASE_IMAGE_TAG  = piot-rootfs
CONT_NAME       = piot-build

## IMAGE
BASE_TAR      = $(IMAGES_DIR)/build/base.tar
BASE_IMG      = $(IMAGES_DIR)/build/base.img

KERNEL_HEADERS = $(BUILD)/kernel-headers

PAR             = $(shell nproc)

.PHONY: all tools clean kernel initrd

all: rootfs kernel initrd

build dist deps:
	mkdir -p $(CURDIR)/$@

quick-all: $(TWIN_IMG)

rootfs: $(BASE_IMG)
dracut: $(DRACUT_IMAGE_DEP)
initrd: $(INITRD)

$(IMAGE_DEP_PREFIX)$(IMAGE_PREFIX)%: $(CURDIR)/Dockerfile.%
	docker build -f $< -t $(IMAGE_PREFIX)$* --build-arg FILES=files/$* .
	mkdir -p $(dir $@) && touch $@

$(ROOTFS_IMAGE_DEP): $(shell find $(CURDIR)/files/rootfs -type f)

$(INITRD): $(DRACUT_IMAGE_DEP)
	mkdir -p $(BUILD)
	$(DRACUT_CMD) \
		bash -c " \
			dracut \
				--no-kernel \
				--force \
				--no-hostonly \
				--verbose \
				/build/initrd.img &> /build/dracut.log && \
			chmod a+r /build/initrd.img \
		"
	cp /build/initrd.img $@

kernel:
	cd $(KERNEL_DIR); \
		CFLAGS=-Wno-error make bzImage -j$(PAR)
	cp $(KERNEL_DIR)/arch/x86/boot/bzImage $(CURDIR)/../scripts

# "-DCMAKE_C_FLAGS=-I$(KERNEL_HEADERS)/include -I$(GLIBC_DIST)/include -L$(GLIBC_DIST)/lib -static"

kernel-headers:
	cd $(KERNEL_DIR); \
		make INSTALL_HDR_PATH=$(KERNEL_HEADERS) headers_install -j$(PAR)

tools: kernel-headers
	echo "Build tools"
	mkdir -p $(TOOLS_BUILD)
	cd $(TOOLS_BUILD) ; cmake $(TOOLS_SRC) \
		"-DCMAKE_C_FLAGS=-I$(KERNEL_HEADERS)/include -static -I$(GLIBC_DIST)/include -L$(GLIBC_DIST)/lib -static -DHAVE_KERNCALL" \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_INSTALL_PREFIX=$(TOOLS_DIST) \
		-DCMAKE_INSTALL_SYSCONFDIR=/etc
	cmake --build $(TOOLS_BUILD) ##--verbose
	cmake --install $(TOOLS_BUILD)

$(TOOLS_DEP): deps tools
	[ -f $@ ] || touch $@
	stat --format %y $(shell find $(TOOLS_DIST) -type f | xargs ls -tr1 | tail -1) | xargs -i touch -d {} $@

$(BASE_TAR): $(ROOTFS_IMAGE_DEP)
	docker rm -f $(CONT_NAME) || true
	docker create --name $(CONT_NAME) $(BASE_IMAGE_TAG)
	docker export $(CONT_NAME) > $@
	docker rm -f $(CONT_NAME)

$(BASE_IMG): $(BASE_TAR) # $(TOOLS_DEP)
	qemu-img create -f qcow2 $@.tmp 10g
	LIBGUESTFS_BACKEND=direct guestfish -- \
		add $@.tmp : \
		run : \
		mkfs ext4 /dev/sda : \
		mount /dev/sda / : \
		tar-in $(BASE_TAR) / : \
		exit
	mv $@.tmp $@

base-tar:
	@echo $(ROOTFS_IMAGE_DEP)
	@echo $(IMAGE_DEP_PREFIX)$(IMAGE_PREFIX)rootfs
	@echo $(shell find $(CURDIR)/files/rootfs -type f)

clean :
	rm -rf $(CURDIR)/build
