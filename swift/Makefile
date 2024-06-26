#
# Copyright (c) ZeroC, Inc. All rights reserved.
#

top_srcdir := ..

include $(top_srcdir)/config/Make.rules

CONFIG ?= $(if $(filter $(OPTIMIZE),no),Debug,Release)
# TODO use ice-test.xcodeproj for binary builds once carthage support XCFramework prebuild packages
PROJECT ?= ice.xcodeproj
# PROJECT ?= $(if $(filter $(ICE_BIN_DIST), swift all),ice-test.xcodeproj,ice.xcodeproj)

RAKE      ?= rake
RAKE_TASK ?= icebdistproj

# $(call make-xcodebuild-rule,$1=rule,$2=platform,$3=scheme,$4=target)
define make-xcodebuild-rule
$1:: $(PROJECT)
	xcodebuild $(if $(V),,-quiet) \
                   -project $(PROJECT) \
                   -target "$3 $(if $(filter $2,macosx),macOS,iOS)" \
                   -configuration $(CONFIG) \
                   -sdk $2 $4
endef

ice.xcodeproj: Rakefile | carthage-bootstrap
	rake icesdistproj

ice-test.xcodeproj: Rakefile | carthage-bootstrap
	rake icebdistproj

all:: tests

carthage-bootstrap:
	$(Q)cd .. && carthage bootstrap --cache-builds --use-xcframeworks --platform macOS,iOS

rake: carthage-bootstrap
	$(Q)$(RAKE) $(RAKE_TASK)

tests srcs:: rake

install::
	@echo nothing to install

project:
	@echo Project is $(PROJECT)

$(foreach p,$(platforms),$(eval $(call make-xcodebuild-rule,srcs,$p,Ice,build)))
$(foreach p,$(platforms),$(eval $(call make-xcodebuild-rule,tests,$p,TestDriver,build)))

$(foreach p,$(platforms),$(eval $(call make-xcodebuild-rule,clean,$p,Ice,clean)))
$(foreach p,$(platforms),$(eval $(call make-xcodebuild-rule,clean,$p,TestDriver,clean)))

$(foreach p,$(supported-platforms),$(eval $(call make-xcodebuild-rule,distclean,$p,Ice,clean)))
$(foreach p,$(supported-platforms),$(eval $(call make-xcodebuild-rule,distclean,$p,TestDriver,clean)))
