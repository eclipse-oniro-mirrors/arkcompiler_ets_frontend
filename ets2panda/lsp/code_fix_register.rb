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

module CodeFixRegister
  @codefix_map = Hash.new { |h, k| h[k] = [] }

  class DiagnosticCode
    attr_reader :type, :id

    def initialize(type, id)
      @type = type
      @id = id
    end
  end

  class << self
    def codefix_map
      @codefix_map
    end

    def collect_code_fix(diagnostic)
      diagnostic.code_fix_ids.each do |code_fix_id|
        @codefix_map[code_fix_id] << DiagnosticCode.new(diagnostic.type, diagnostic.id)
      end
    end

    def wrap_data(data)
      data.each_pair do |diagnostic_type, diagnostics|
        diagnostics.each do |diagnostic|
          if diagnostic.respond_to?(:code_fix_ids)
            diagnostic.type = diagnostic_type
            collect_code_fix(diagnostic)
          end
        end
      end
    end
  end
end

class String
  def snakecase
    self.gsub(/::/, '/').
    gsub(/([A-Z]+)([A-Z][a-z])/,'\1_\2').
    gsub(/([a-z\d])([A-Z])/,'\1_\2').
    tr("-", "_").
    downcase
  end
end

def Gen.on_require(data)
  CodeFixRegister.wrap_data(data)
end