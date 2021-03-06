################################################################
#
#        Copyright 2013, Big Switch Networks, Inc. 
# 
# Licensed under the Eclipse Public License, Version 1.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
# 
#        http://www.eclipse.org/legal/epl-v10.html
# 
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
# either express or implied. See the License for the specific
# language governing permissions and limitations under the
# License.
#
################################################################

#
# The root of of our repository is here:
#
ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

#
# Resolve submodule dependencies. 
#
ifndef SUBMODULE_INFRA
  ifdef SUBMODULES
    SUBMODULE_INFRA := $(SUBMODULES)/infra
  else
    SUBMODULE_INFRA := $(ROOT)/submodules/infra
    SUBMODULES_LOCAL += infra
  endif
endif

ifndef SUBMODULE_BIGCODE
  ifdef SUBMODULES
    SUBMODULE_BIGCODE := $(SUBMODULES)/bigcode
  else
    SUBMODULE_BIGCODE := $(ROOT)/submodules/bigcode
    SUBMODULES_LOCAL += bigcode
  endif
endif

ifndef SUBMODULE_INDIGO
  ifdef SUBMODULES
    SUBMODULE_INDIGO := $(SUBMODULES)/indigo
  else
    SUBMODULE_INDIGO := $(ROOT)/submodules/indigo
    SUBMODULES_LOCAL += indigo
  endif
endif


ifdef SUBMODULES_LOCAL
  SUBMODULES_LOCAL_UPDATE := $(shell python $(ROOT)/submodules/init.py --update $(SUBMODULES_LOCAL))
  ifneq ($(lastword $(SUBMODULES_LOCAL_UPDATE)),submodules:ok.)
    $(info Local submodule update failed.)
    $(info Result:)
    $(info $(SUBMODULES_LOCAL_UPDATE))
    $(error Abort)
  endif
endif

export SUBMODULE_INFRA
export SUBMODULE_BIGCODE
export SUBMODULE_INDIGO
export BUILDER := $(SUBMODULE_INFRA)/builder/unix

MODULE_DIRS := $(ROOT)/modules $(SUBMODULE_INFRA)/modules $(SUBMODULE_BIGCODE)/modules $(SUBMODULE_INDIGO)/modules

.show-submodules:
	@echo infra @ $(SUBMODULE_INFRA)
	@echo bigcode @ $(SUBMODULE_BIGCODE)
	@echo indigo @ $(SUBMODULE_INDIGO)














