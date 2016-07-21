# This file is generated by gyp; do not edit.

TOOLSET := target
TARGET := v8_external_snapshot
### Rules for action "run_mksnapshot (external)":
quiet_cmd__home_alex_Documents_projects_pxCore_pxCore_examples_pxScene2d_external_libnode_v0_12_7_deps_v8_tools_gyp_v8_gyp_v8_external_snapshot_target_run_mksnapshot__external_ = ACTION _home_alex_Documents_projects_pxCore_pxCore_examples_pxScene2d_external_libnode_v0_12_7_deps_v8_tools_gyp_v8_gyp_v8_external_snapshot_target_run_mksnapshot__external_ $@
cmd__home_alex_Documents_projects_pxCore_pxCore_examples_pxScene2d_external_libnode_v0_12_7_deps_v8_tools_gyp_v8_gyp_v8_external_snapshot_target_run_mksnapshot__external_ = LD_LIBRARY_PATH=$(builddir)/lib.host:$(builddir)/lib.target:$$LD_LIBRARY_PATH; export LD_LIBRARY_PATH; cd $(srcdir)/deps/v8/tools/gyp; mkdir -p $(builddir) $(obj).$(TOOLSET)/v8_external_snapshot/geni; "$(builddir)/mksnapshot" --log-snapshot-positions --logfile "$(obj).$(TOOLSET)/v8_external_snapshot/geni/snapshot.log" "$(obj).$(TOOLSET)/v8_external_snapshot/geni/snapshot.cc" --startup_blob "$(builddir)/snapshot_blob.bin"

$(obj).$(TOOLSET)/$(TARGET)/geni/snapshot.cc: obj := $(abs_obj)
$(obj).$(TOOLSET)/$(TARGET)/geni/snapshot.cc: builddir := $(abs_builddir)
$(obj).$(TOOLSET)/$(TARGET)/geni/snapshot.cc: TOOLSET := $(TOOLSET)
$(obj).$(TOOLSET)/$(TARGET)/geni/snapshot.cc: $(builddir)/mksnapshot FORCE_DO_CMD
	$(call do_cmd,_home_alex_Documents_projects_pxCore_pxCore_examples_pxScene2d_external_libnode_v0_12_7_deps_v8_tools_gyp_v8_gyp_v8_external_snapshot_target_run_mksnapshot__external_)
$(builddir)/snapshot_blob.bin: $(obj).$(TOOLSET)/$(TARGET)/geni/snapshot.cc
$(builddir)/snapshot_blob.bin: ;

all_deps += $(obj).$(TOOLSET)/$(TARGET)/geni/snapshot.cc $(builddir)/snapshot_blob.bin
action__home_alex_Documents_projects_pxCore_pxCore_examples_pxScene2d_external_libnode_v0_12_7_deps_v8_tools_gyp_v8_gyp_v8_external_snapshot_target_run_mksnapshot__external__outputs := $(obj).$(TOOLSET)/$(TARGET)/geni/snapshot.cc $(builddir)/snapshot_blob.bin


DEFS_Debug := \
	'-DV8_TARGET_ARCH_X64' \
	'-DENABLE_DISASSEMBLER' \
	'-DV8_ENABLE_CHECKS' \
	'-DOBJECT_PRINT' \
	'-DVERIFY_HEAP' \
	'-DDEBUG' \
	'-DENABLE_EXTRA_CHECKS' \
	'-DENABLE_HANDLE_ZAPPING' \
	'-D_DEBUG'

# Flags passed to all source files.
CFLAGS_Debug := \
	-pthread \
	-Wall \
	-Wextra \
	-Wno-unused-parameter \
	-m64 \
	-fno-strict-aliasing \
	-m64 \
	-Woverloaded-virtual \
	 \
	-fdata-sections \
	-ffunction-sections \
	-g \
	-fPIC

# Flags passed to only C files.
CFLAGS_C_Debug :=

# Flags passed to only C++ files.
CFLAGS_CC_Debug := \
	-fno-rtti \
	-fno-exceptions

INCS_Debug := \
	-I$(srcdir)/deps/v8

DEFS_Release := \
	'-DV8_TARGET_ARCH_X64' \
	'-DENABLE_DISASSEMBLER'

# Flags passed to all source files.
CFLAGS_Release := \
	-pthread \
	-Wall \
	-Wextra \
	-Wno-unused-parameter \
	-m64 \
	-fno-strict-aliasing \
	-m64 \
	-O3 \
	-ffunction-sections \
	-fdata-sections \
	-fPIC \
	-fno-tree-vrp \
	-fno-omit-frame-pointer \
	-fdata-sections \
	-ffunction-sections \
	 \
	-O3

# Flags passed to only C files.
CFLAGS_C_Release :=

# Flags passed to only C++ files.
CFLAGS_CC_Release := \
	-fno-rtti \
	-fno-exceptions

INCS_Release := \
	-I$(srcdir)/deps/v8

OBJS := \
	$(obj).target/$(TARGET)/deps/v8/src/natives-external.o \
	$(obj).target/$(TARGET)/deps/v8/src/snapshot-external.o

# Add to the list of files we specially track dependencies for.
all_deps += $(OBJS)

# Make sure our dependencies are built before any of us.
$(OBJS): | $(builddir)/mksnapshot $(obj).target/deps/v8/tools/gyp/js2c.stamp $(obj).target/deps/v8/tools/gyp/natives_blob.stamp

# Make sure our actions/rules run before any of us.
$(OBJS): | $(action__home_alex_Documents_projects_pxCore_pxCore_examples_pxScene2d_external_libnode_v0_12_7_deps_v8_tools_gyp_v8_gyp_v8_external_snapshot_target_run_mksnapshot__external__outputs)

# CFLAGS et al overrides must be target-local.
# See "Target-specific Variable Values" in the GNU Make manual.
$(OBJS): TOOLSET := $(TOOLSET)
$(OBJS): GYP_CFLAGS := $(DEFS_$(BUILDTYPE)) $(INCS_$(BUILDTYPE))  $(CFLAGS_$(BUILDTYPE)) $(CFLAGS_C_$(BUILDTYPE))
$(OBJS): GYP_CXXFLAGS := $(DEFS_$(BUILDTYPE)) $(INCS_$(BUILDTYPE))  $(CFLAGS_$(BUILDTYPE)) $(CFLAGS_CC_$(BUILDTYPE))

# Suffix rules, putting all outputs into $(obj).

$(obj).$(TOOLSET)/$(TARGET)/%.o: $(srcdir)/%.cc FORCE_DO_CMD
	@$(call do_cmd,cxx,1)

# Try building from generated source, too.

$(obj).$(TOOLSET)/$(TARGET)/%.o: $(obj).$(TOOLSET)/%.cc FORCE_DO_CMD
	@$(call do_cmd,cxx,1)

$(obj).$(TOOLSET)/$(TARGET)/%.o: $(obj)/%.cc FORCE_DO_CMD
	@$(call do_cmd,cxx,1)

# End of this set of suffix rules
### Rules for final target.
# Build our special outputs first.
$(obj).target/deps/v8/tools/gyp/libv8_external_snapshot.a: | $(action__home_alex_Documents_projects_pxCore_pxCore_examples_pxScene2d_external_libnode_v0_12_7_deps_v8_tools_gyp_v8_gyp_v8_external_snapshot_target_run_mksnapshot__external__outputs)

# Preserve order dependency of special output on deps.
$(action__home_alex_Documents_projects_pxCore_pxCore_examples_pxScene2d_external_libnode_v0_12_7_deps_v8_tools_gyp_v8_gyp_v8_external_snapshot_target_run_mksnapshot__external__outputs): | $(builddir)/mksnapshot $(obj).target/deps/v8/tools/gyp/js2c.stamp $(obj).target/deps/v8/tools/gyp/natives_blob.stamp

LDFLAGS_Debug := \
	-pthread \
	-rdynamic \
	-m64 \
	-m64

LDFLAGS_Release := \
	-pthread \
	-rdynamic \
	-m64 \
	-m64

LIBS :=

$(obj).target/deps/v8/tools/gyp/libv8_external_snapshot.a: GYP_LDFLAGS := $(LDFLAGS_$(BUILDTYPE))
$(obj).target/deps/v8/tools/gyp/libv8_external_snapshot.a: LIBS := $(LIBS)
$(obj).target/deps/v8/tools/gyp/libv8_external_snapshot.a: TOOLSET := $(TOOLSET)
$(obj).target/deps/v8/tools/gyp/libv8_external_snapshot.a: $(OBJS) FORCE_DO_CMD
	$(call do_cmd,alink)

all_deps += $(obj).target/deps/v8/tools/gyp/libv8_external_snapshot.a
# Add target alias
.PHONY: v8_external_snapshot
v8_external_snapshot: $(obj).target/deps/v8/tools/gyp/libv8_external_snapshot.a

# Add target alias
.PHONY: v8_external_snapshot
v8_external_snapshot: $(builddir)/libv8_external_snapshot.a

# Copy this to the static library output path.
$(builddir)/libv8_external_snapshot.a: TOOLSET := $(TOOLSET)
$(builddir)/libv8_external_snapshot.a: $(obj).target/deps/v8/tools/gyp/libv8_external_snapshot.a FORCE_DO_CMD
	$(call do_cmd,copy)

all_deps += $(builddir)/libv8_external_snapshot.a
# Short alias for building this static library.
.PHONY: libv8_external_snapshot.a
libv8_external_snapshot.a: $(obj).target/deps/v8/tools/gyp/libv8_external_snapshot.a $(builddir)/libv8_external_snapshot.a

