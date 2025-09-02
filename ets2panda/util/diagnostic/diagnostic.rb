#!/usr/bin/env ruby
# Copyright (c) 2025 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

require 'ostruct'
require 'set'
require 'delegate'

module Diagnostic
  module_function

  @diagnostics = []

  def diagnostics
    @diagnostics
  end

  def normalization_error
    warn "You probably need to run 'normalize_yaml', see ets_frontend/ets2panda/util/diagnostic/README.md"
    Kernel.exit 1
  end

  def wrap_data(data)
    graveyard = data.delete_field(:graveyard)
    data.freeze
    graveyard.each_cons(2) do |lhs, rhs|
      if lhs >= rhs
        warn "Graveyard is not strictly monotonically sorted, '#{lhs}' should come before '#{rhs}'"
        normalization_error
      end
    end
    graveyard = graveyard.to_set
    data.each_pair do |diagnostic_type, diagnostics|
      # Check if the YAML is in normal form according to ets_frontend/ets2panda/util/diagnostic/normalize_yaml
      diagnostics.map(&:name).each_cons(2) do |lhs, rhs|
        if lhs >= rhs
          warn "Message with name '#{lhs}' should come after '#{rhs}' for diagnostic type '#{diagnostic_type}'"
          normalization_error
        end
      end
      diagnostics.map(&:id).each do |id|
        if graveyard.member? id
          warn "'#{id}' used for diagnostic type #{diagnostic_type} is already in the graveyard, let it rest in peace"
          normalization_error
        end
      end
      diagnostics.map(&:id).group_by(&:itself).select{ |_, v| v.size > 1 }.map(&:first).each do |duplicate|
        warn "Duplicate id '#{duplicate}' for diagnostic type '#{diagnostic_type}'"
        normalization_error
      end
      diagnostics.each do |diagnostic|
        diagnostic.type = diagnostic_type
        @diagnostics.append(diagnostic)
      end
    end
  end
end

def Gen.on_require(data)
  Diagnostic.wrap_data(data)
end

