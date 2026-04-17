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
  @hardlists_map = {}
  @softlists_map = {}

  def diagnostics
    @diagnostics
  end

  def normalization_error
    warn "You probably need to run 'normalize_yaml', see ets_frontend/ets2panda/util/diagnostic/README.md"
    Kernel.exit 1
  end

  def regex_error(msg)
    warn "Regex Error: #{msg}"
    Kernel.exit 1
  end

  def configuration_error(msg)
    warn "Configuration Error: #{msg}"
    Kernel.exit 1
  end

  def create_diagnostic_pathlist(diagnostic_type, diagnostic, list, result_map)
    if diagnostic_type.to_s != 'warning'
      configuration_error("Lists are only allowed for 'warning'. Found in '#{diagnostic_type}' for '#{diagnostic.name}'")
    end

    paths = []
    list.each do |bl_name|
      if result_map.key?(bl_name)
        paths.concat(result_map[bl_name])
      else
        configuration_error("List '#{bl_name}' referenced by diagnostic '#{diagnostic.name}' is not defined")
      end
    end
    paths.uniq!
    paths.map! { |path| path.gsub('/', '[\\\\\\\\\\\\\\\\/]') } # HACK: for win path
    paths
  end

  def try_parse_lists(data)
    verify_lists(data)
    if data.respond_to?(:hardlist)
      hardlists_data = data.delete_field(:hardlist)
      if hardlists_data
        hardlists_data.each do |bl|
          @hardlists_map[bl.name] = bl.paths || []
        end
      end
      return true
    end

    if data.respond_to?(:softlist)
      softlists_data = data.delete_field(:softlist)
      if softlists_data
        softlists_data.each do |wl|
          @softlists_map[wl.name] = wl.paths || []
        end
      end
      return true
    end
    false
  end

  def validate_path_pattern!(path, context)
    allowed = 'a-zA-Z0-9_\\-./\\\\*+?\\[\\]()@'
    regex = %r{\A\.\*/[#{allowed}]+(?:\.[*]|(?:\\.[a-zA-Z]+)+)\z}

    return if path.match?(regex)

    if !path.start_with?('.*/')
      regex_error("Path '#{path}' in #{context} must start with '.*/'")
    elsif !path.end_with?('.*') && !path.match?(/(\\.[a-zA-Z]+)+$/)
      regex_error("Path '#{path}' in #{context} must end with '.*' or '\\.[a-z]+(\\.[a-z]+)*'")
    else
      regex_error("Path '#{path}' in #{context} contains forbidden characters.
                     Allowed: a-z, A-Z, 0-9, _, -, ., /, \\, *, +, ?, [, ], (, ), @")
    end
  end

  def check_if_warnings_list(data)
    data.respond_to?(:hardlist) || data.respond_to?(:softlist)
  end

  def verify_lists(data)
    return unless check_if_warnings_list(data)

    if data.respond_to?(:hardlist) && data.respond_to?(:softlist)
      configuration_error('Both hardlist and softlist defined in the same file, only one is allowed')
    end

    list = data.respond_to?(:hardlist) ? data.hardlist : data.softlist
    list_name = data.respond_to?(:hardlist) ? 'hardlist' : 'softlist'

    allowed_item_fields = Set['name', 'paths']
    names = Set.new
    list.each do |item|
      item.each_pair do |field, _value|
        unless allowed_item_fields.include?(field.to_s)
          configuration_error("Unknown field '#{field}' in #{list_name} item '#{item.name}'")
        end
      end

      configuration_error("Missing 'name' field in #{list_name} item") unless item.respond_to?(:name) && item.name
      unless item.respond_to?(:paths) && item.paths
        configuration_error("Missing 'paths' field in #{list_name} item '#{item.name}'")
      end

      configuration_error("Duplicate name '#{item.name}' in #{list_name}") if names.include?(item.name)
      names.add(item.name)

      item.paths.each do |path|
        validate_path_pattern!(path, "#{list_name} '#{item.name}'")
      end
    end
  end

  def verify_diagnostics_yamls(data)
    return if check_if_warnings_list(data)

    valid_kinds = Set[
      'warning', 'syntax', 'semantic', 'fatal',
      'declgen_ets2ts_error', 'declgen_ets2ts_warning',
      'isolated_declgen', 'arkts_config_error'
    ]
    allowed_fields = Set['name', 'id', 'message', 'strict', 'softlist', 'hardlist', 'code_fix_ids']

    diagnostic_types = data.each_pair.map { |k, _| k.to_s }.select { |k| valid_kinds.include?(k) }

    if diagnostic_types.size > 1
      configuration_error("Multiple diagnostic kinds in one file: #{diagnostic_types.join(', ')}")
    end

    data.each_pair do |key, _|
      key_str = key.to_s
      unless valid_kinds.include?(key_str) || key_str == 'graveyard'
        configuration_error("Unknown diagnostic kind '#{key_str}'")
      end
    end

    data.each_pair do |diagnostic_type, diagnostics|
      next unless valid_kinds.include?(diagnostic_type.to_s)

      diagnostics.each do |diagnostic|
        diagnostic.each_pair do |field, _|
          unless allowed_fields.include?(field.to_s)
            configuration_error("Unknown field '#{field}' in #{diagnostic_type} diagnostic '#{diagnostic.name}'")
          end
        end
      end
    end
  end

  def set_lists(diagnostic, diagnostic_type)
    diagnostic.hardlist = if diagnostic.respond_to?(:hardlist) && diagnostic.hardlist
                            create_diagnostic_pathlist(diagnostic_type, diagnostic, diagnostic.hardlist, @hardlists_map)
                          else
                            []
                          end

    diagnostic.softlist = if diagnostic.respond_to?(:softlist) && diagnostic.softlist
                            create_diagnostic_pathlist(diagnostic_type, diagnostic, diagnostic.softlist, @softlists_map)
                          else
                            []
                          end
  end

  def wrap_data(data)
    return if try_parse_lists(data)

    verify_diagnostics_yamls(data)

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
      diagnostics.map(&:id).group_by(&:itself).select { |_, v| v.size > 1 }.map(&:first).each do |duplicate|
        warn "Duplicate id '#{duplicate}' for diagnostic type '#{diagnostic_type}'"
        normalization_error
      end
      diagnostics.each do |diagnostic|
        diagnostic.type = diagnostic_type
        diagnostic.strict ||= false

        set_lists(diagnostic, diagnostic_type)

        @diagnostics.append(diagnostic)
      end
    end
  end
end

def Gen.on_require(data)
  Diagnostic.wrap_data(data)
end
